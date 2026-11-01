#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kMinDb = -60.0f;
    constexpr float kSilence = 1.0e-8f;

    float safeDb (float linear) noexcept
    {
        return juce::Decibels::gainToDecibels (juce::jmax (linear, kSilence), kMinDb);
    }

    float onePoleCoefficient (double sampleRate, float timeMs) noexcept
    {
        const auto seconds = juce::jmax (0.0001f, timeMs * 0.001f);
        return std::exp (-1.0f / (static_cast<float> (sampleRate) * seconds));
    }
}

MJOptoLevelerAudioProcessor::MJOptoLevelerAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
MJOptoLevelerAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> ("peakReduction", "Peak Reduction",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f, "%"));
    p.push_back (std::make_unique<AudioParameterFloat> ("gain", "Gain",
        NormalisableRange<float> (0.0f, 40.0f, 0.01f), 18.0f, "dB"));
    p.push_back (std::make_unique<AudioParameterBool> ("limitMode", "Limit", false));
    p.push_back (std::make_unique<AudioParameterFloat> ("mix", "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));
    p.push_back (std::make_unique<AudioParameterFloat> ("input", "Input",
        NormalisableRange<float> (-18.0f, 18.0f, 0.01f), 0.0f, "dB"));
    p.push_back (std::make_unique<AudioParameterFloat> ("output", "Output",
        NormalisableRange<float> (-18.0f, 18.0f, 0.01f), 0.0f, "dB"));
    p.push_back (std::make_unique<AudioParameterBool> ("valve", "Valve Color", true));

    return { p.begin(), p.end() };
}

void MJOptoLevelerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = samplesPerBlock;
    opticalEnvelope = 0.0f;
    slowReleaseMemory = 0.0f;
    previousOpticalGain = 1.0f;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 1 };

    for (int ch = 0; ch < 2; ++ch)
    {
        sidechainHPF[ch].reset();
        transformerHPF[ch].reset();
        transformerLPF[ch].reset();
    }

    updateFilters();

    // Polyphase IIR oversampling: efficient 4x nonlinear stage.
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampling->initProcessing (static_cast<size_t> (samplesPerBlock));
    setLatencySamples (static_cast<int> (std::ceil (oversampling->getLatencyInSamples())));

    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    grMeter.store (0.0f);
    outMeter.store (-60.0f);
}

void MJOptoLevelerAudioProcessor::releaseResources()
{
    oversampling.reset();
}

bool MJOptoLevelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

float MJOptoLevelerAudioProcessor::dbToGain (float db) const noexcept
{
    return juce::Decibels::decibelsToGain (db);
}

void MJOptoLevelerAudioProcessor::updateFilters()
{
    const auto sr = currentSampleRate;
    auto makeHP = [sr] (float hz)
    {
        return juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hz, 0.7071f);
    };
    auto makeLP = [sr] (float hz)
    {
        return juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, hz, 0.7071f);
    };

    for (int ch = 0; ch < 2; ++ch)
    {
        sidechainHPF[ch].coefficients = makeHP (100.0f);
        transformerHPF[ch].coefficients = makeHP (10.0f);
        transformerLPF[ch].coefficients = makeLP (juce::jmin (25000.0f, static_cast<float> (sr * 0.45)));
    }
}

void MJOptoLevelerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float peakReduction = get ("peakReduction");
    const float makeupDb = get ("gain");
    const float inputDb = get ("input");
    const float outputDb = get ("output");
    const float mix = get ("mix") * 0.01f;
    const bool limitMode = get ("limitMode") > 0.5f;
    const bool valveOn = get ("valve") > 0.5f;

    dryBuffer.setSize (channels, samples, false, false, true);
    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, samples);

    const float inputGain = dbToGain (inputDb);
    const float outputGain = dbToGain (outputDb);
    const float ratio = limitMode ? 10.0f : 3.0f;

    // Peak Reduction maps to an adjustable optical threshold. At 0%, threshold
    // is high (little/no compression); at 100%, threshold is low.
    const float thresholdDb = juce::jmap (peakReduction, 0.0f, 100.0f, 0.0f, -36.0f);
    const float threshold = dbToGain (thresholdDb);

    // Fixed nominal T4 attack around 10 ms. Adjust here for a faster/slower cell.
    const float attackCoeff = onePoleCoefficient (currentSampleRate, 10.0f);
    const float fastReleaseCoeff = onePoleCoefficient (currentSampleRate, 60.0f);

    float maxGR = 0.0f;
    float peakOut = 0.0f;

    for (int n = 0; n < samples; ++n)
    {
        float detector = 0.0f;

        // Stereo-linked detector: maximum absolute sidechain signal, after 100 Hz HPF.
        for (int ch = 0; ch < channels; ++ch)
        {
            const float x = buffer.getSample (ch, n) * inputGain;
            const float sc = sidechainHPF[ch].processSample (x);
            detector = juce::jmax (detector, std::abs (sc));
        }

        // T4-like envelope follower. Attack is fixed; release uses two memories:
        // a quick 60 ms recovery for the first part, followed by a slow 0.5–5 s tail.
        if (detector > opticalEnvelope)
            opticalEnvelope = attackCoeff * opticalEnvelope + (1.0f - attackCoeff) * detector;
        else
        {
            const float fast = fastReleaseCoeff * opticalEnvelope
                             + (1.0f - fastReleaseCoeff) * detector;

            // Compression history controls the long release: heavier/longer GR
            // increases the slow-memory value and lengthens the recovery.
            const float compressionActivity = juce::jlimit (0.0f, 1.0f,
                (safeDb (previousOpticalGain) * -1.0f) / 18.0f);
            const float slowTimeMs = juce::jmap (compressionActivity, 0.0f, 1.0f,
                                                  500.0f, 5000.0f);
            const float slowCoeff = onePoleCoefficient (currentSampleRate, slowTimeMs);
            slowReleaseMemory = slowCoeff * slowReleaseMemory
                              + (1.0f - slowCoeff) * fast;

            // Hysteresis: retain a portion of the previous cell state; this prevents
            // abrupt gain jumps and makes recovery dependent on recent illumination.
            const float hysteresis = juce::jmap (compressionActivity, 0.0f, 1.0f,
                                                  0.08f, 0.35f);
            opticalEnvelope = juce::jmax (fast, fast * (1.0f - hysteresis)
                                                   + slowReleaseMemory * hysteresis);
        }

        const float envDb = safeDb (opticalEnvelope);
        const float overDb = envDb - thresholdDb;
        const float compressedDb = overDb > 0.0f ? overDb * (1.0f - 1.0f / ratio) : 0.0f;
        const float targetGain = dbToGain (-compressedDb);

        // Smooth the optical cell gain as a second, program-dependent optical state.
        const float gainCoeff = targetGain < previousOpticalGain
                              ? attackCoeff : fastReleaseCoeff;
        previousOpticalGain = gainCoeff * previousOpticalGain
                            + (1.0f - gainCoeff) * targetGain;

        const float grDb = juce::jmax (0.0f, -safeDb (previousOpticalGain));
        maxGR = juce::jmax (maxGR, grDb);

        for (int ch = 0; ch < channels; ++ch)
        {
            float y = buffer.getSample (ch, n) * inputGain * previousOpticalGain;
            y *= dbToGain (makeupDb);

            // The nonlinear valve stage is performed below in the oversampled block.
            buffer.setSample (ch, n, y);
        }
    }

    // 4x oversampled tanh valve-style makeup stage.
    if (oversampling != nullptr && valveOn)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto upBlock = oversampling->processSamplesUp (block);
        const float drive = 1.35f;
        for (size_t ch = 0; ch < upBlock.getNumChannels(); ++ch)
            for (size_t n = 0; n < upBlock.getNumSamples(); ++n)
            {
                const float x = upBlock.getSample (ch, n);
                // Soft asymmetry gives a subtle valve-like even-harmonic component.
                const float shaped = std::tanh (drive * (x + 0.025f * x * x));
                upBlock.setSample (ch, n, shaped / std::tanh (drive));
            }
        oversampling->processSamplesDown (block);
    }

    for (int ch = 0; ch < channels; ++ch)
    {
        for (int n = 0; n < samples; ++n)
        {
            float wet = buffer.getSample (ch, n) * outputGain;
            wet = transformerHPF[ch].processSample (wet);
            wet = transformerLPF[ch].processSample (wet);

            const float dry = dryBuffer.getSample (ch, n);
            const float y = dry * (1.0f - mix) + wet * mix;
            buffer.setSample (ch, n, y);
            peakOut = juce::jmax (peakOut, std::abs (y));
        }
    }

    grMeter.store (maxGR);
    outMeter.store (safeDb (peakOut));
}

juce::AudioProcessorEditor* MJOptoLevelerAudioProcessor::createEditor()
{
    return new MJOptoLevelerAudioProcessorEditor (*this);
}

void MJOptoLevelerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MJOptoLevelerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

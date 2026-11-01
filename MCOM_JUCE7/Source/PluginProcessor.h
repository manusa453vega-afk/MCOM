#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class MJOptoLevelerAudioProcessor final : public juce::AudioProcessor
{
public:
    MJOptoLevelerAudioProcessor();
    ~MJOptoLevelerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    float getGainReductionDb() const noexcept { return grMeter.load(); }
    float getOutputLevelDb() const noexcept { return outMeter.load(); }

private:
    using Filter = juce::dsp::IIR::Filter<float>;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float dbToGain (float db) const noexcept;
    void updateFilters();

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> grMeter { 0.0f };
    std::atomic<float> outMeter { -60.0f };

    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    float opticalEnvelope = 0.0f;
    float slowReleaseMemory = 0.0f;
    float previousOpticalGain = 1.0f;

    // Sidechain HPF (100 Hz) and output transformer bandwidth shaping.
    juce::dsp::IIR::Filter<float> sidechainHPF[2];
    juce::dsp::IIR::Filter<float> transformerHPF[2], transformerLPF[2];

    // 4x oversampling is applied only around the nonlinear valve-style stage.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MJOptoLevelerAudioProcessor)
};

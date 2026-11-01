#include "PluginEditor.h"

MJOptoLevelerAudioProcessorEditor::MJOptoLevelerAudioProcessorEditor (
    MJOptoLevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      grBar (grProgress), outBar (outProgress)
{
    setSize (720, 430);

    styleLabel (title, "MJ OPTO LEVELER");
    title.setFont (juce::Font (27.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, juce::Colour (0xfff2f0fa));
    addAndMakeVisible (title);

    styleLabel (subtitle, "PROGRAM-DEPENDENT OPTICAL COMPRESSOR");
    subtitle.setFont (juce::Font (10.0f, juce::Font::bold));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xffa99fc8));
    addAndMakeVisible (subtitle);

    styleSlider (peakReduction, "%");
    styleSlider (gain, " dB");
    styleSlider (mix, "%");
    styleSlider (input, " dB");
    styleSlider (output, " dB");

    styleLabel (peakLabel, "PEAK REDUCTION");
    styleLabel (gainLabel, "GAIN");
    styleLabel (mixLabel, "MIX");
    styleLabel (inputLabel, "INPUT");
    styleLabel (outputLabel, "OUTPUT");

    for (auto* c : { (juce::Component*) &peakReduction, &gain, &mix, &input, &output,
                     &peakLabel, &gainLabel, &mixLabel, &inputLabel, &outputLabel })
        addAndMakeVisible (c);

    limitMode.setButtonText ("LIMIT");
    valve.setButtonText ("VALVE COLOR");
    limitMode.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    valve.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (limitMode);
    addAndMakeVisible (valve);

    styleLabel (modeLabel, "MODE");
    styleLabel (valveLabel, "ANALOG");
    addAndMakeVisible (modeLabel);
    addAndMakeVisible (valveLabel);

    styleLabel (grLabel, "GAIN REDUCTION");
    styleLabel (outLabel, "OUTPUT LEVEL");
    addAndMakeVisible (grLabel);
    addAndMakeVisible (outLabel);

    grBar.setTextToDisplay ("");
    outBar.setTextToDisplay ("");
    addAndMakeVisible (grBar);
    addAndMakeVisible (outBar);

    auto& state = processor.getAPVTS();
    peakAttachment = std::make_unique<Attachment> (state, "peakReduction", peakReduction);
    gainAttachment = std::make_unique<Attachment> (state, "gain", gain);
    mixAttachment = std::make_unique<Attachment> (state, "mix", mix);
    inputAttachment = std::make_unique<Attachment> (state, "input", input);
    outputAttachment = std::make_unique<Attachment> (state, "output", output);
    limitAttachment = std::make_unique<ButtonAttachment> (state, "limitMode", limitMode);
    valveAttachment = std::make_unique<ButtonAttachment> (state, "valve", valve);

    startTimerHz (20);
}

void MJOptoLevelerAudioProcessorEditor::styleSlider (juce::Slider& s,
                                                      const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 22);
    s.setTextValueSuffix (suffix);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffa56bff));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe6d8ff));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff24202e));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff514568));
}

void MJOptoLevelerAudioProcessorEditor::styleLabel (juce::Label& l,
                                                     const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (11.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, juce::Colour (0xffc7bddb));
}

void MJOptoLevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111016));
    g.setColour (juce::Colour (0xff211b2b));
    g.fillRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f,
                            (float) getHeight() - 24.0f, 15.0f);

    g.setColour (juce::Colour (0xffa56bff));
    g.fillRoundedRectangle (24.0f, 24.0f, 4.0f, 52.0f, 2.0f);

    g.setColour (juce::Colour (0xff40364f));
    g.drawRoundedRectangle (26.0f, 98.0f, (float) getWidth() - 52.0f, 202.0f, 10.0f, 1.0f);
    g.drawRoundedRectangle (26.0f, 315.0f, (float) getWidth() - 52.0f, 88.0f, 10.0f, 1.0f);

    g.setColour (juce::Colour (0xffa56bff));
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText ("T4 OPTICAL CELL  /  TUBE-STYLE MAKEUP  /  4X SOFT CLIP",
                36, 286, getWidth() - 72, 16, juce::Justification::centred);
}

void MJOptoLevelerAudioProcessorEditor::resized()
{
    title.setBounds (38, 25, 350, 34);
    subtitle.setBounds (40, 60, 350, 18);

    peakLabel.setBounds (48, 108, 125, 20);
    gainLabel.setBounds (190, 108, 110, 20);
    mixLabel.setBounds (330, 108, 110, 20);
    inputLabel.setBounds (470, 108, 95, 20);
    outputLabel.setBounds (570, 108, 95, 20);

    peakReduction.setBounds (48, 130, 125, 145);
    gain.setBounds (190, 130, 110, 145);
    mix.setBounds (330, 130, 110, 145);
    input.setBounds (470, 130, 95, 145);
    output.setBounds (570, 130, 95, 145);

    modeLabel.setBounds (55, 330, 80, 18);
    limitMode.setBounds (55, 350, 105, 30);
    valveLabel.setBounds (190, 330, 120, 18);
    valve.setBounds (190, 350, 130, 30);

    grLabel.setBounds (380, 328, 130, 18);
    grBar.setBounds (380, 350, 125, 18);
    outLabel.setBounds (535, 328, 130, 18);
    outBar.setBounds (535, 350, 125, 18);
}

void MJOptoLevelerAudioProcessorEditor::timerCallback()
{
    const auto gr = processor.getGainReductionDb();
    const auto out = processor.getOutputLevelDb();
    grProgress = juce::jlimit (0.0, 1.0, static_cast<double> (gr / 24.0));
    outProgress = juce::jlimit (0.0, 1.0,
        static_cast<double> ((out + 60.0f) / 60.0f));
    grBar.setTextToDisplay (juce::String (gr, 1) + " dB");
    outBar.setTextToDisplay (juce::String (out, 1) + " dBFS");
    grBar.repaint();
    outBar.repaint();
}

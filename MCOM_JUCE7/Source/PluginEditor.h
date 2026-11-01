#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MJOptoLevelerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit MJOptoLevelerAudioProcessorEditor (MJOptoLevelerAudioProcessor&);
    ~MJOptoLevelerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void styleSlider (juce::Slider&, const juce::String& suffix);
    void styleLabel (juce::Label&, const juce::String& text);

    MJOptoLevelerAudioProcessor& processor;

    juce::Slider peakReduction, gain, mix, input, output;
    juce::ToggleButton limitMode, valve;
    juce::Label title, subtitle, peakLabel, gainLabel, mixLabel, inputLabel, outputLabel;
    juce::Label modeLabel, valveLabel, grLabel, outLabel;
    juce::ProgressBar grBar, outBar;
    double grProgress = 0.0, outProgress = 0.0;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<Attachment> peakAttachment, gainAttachment, mixAttachment,
                                inputAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> limitAttachment, valveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MJOptoLevelerAudioProcessorEditor)
};

#pragma once
#include "PluginProcessor.h"

class MelodikLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class MelodikAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit MelodikAudioProcessorEditor (MelodikAudioProcessor&);
    ~MelodikAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepPreset (int delta);

    MelodikAudioProcessor& proc;
    MelodikLookAndFeel lnf;

    juce::Slider knobs[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[6];
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton randomButton { "ALEATORIO" };
    juce::TextButton freezeButton { "CONGELAR" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    int shownPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelodikAudioProcessorEditor)
};

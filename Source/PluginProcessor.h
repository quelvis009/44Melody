#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include "Effects.h"

struct Preset
{
    const char* name;
    // espacio, textura, movimiento, suciedad, tono, mezcla
    float v[6];
    // flanger, phaser, vibrato, wow, tapestop, granular, pitch, shimmer, ring, reverse, stutter
    float fx[11];
    int pitchSt;
    bool freeze;
};

class MelodikAudioProcessor : public juce::AudioProcessor
{
public:
    MelodikAudioProcessor();
    ~MelodikAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "44melodik"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static const std::vector<Preset>& getPresets();
    void applyPreset (int index);
    void randomize();
    int getCurrentPresetIndex() const { return currentPreset.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const char* id, float realValue);

    std::atomic<int> currentPreset { 0 };

    double sampleRateHz = 44100.0;
    float lfoPhase = 0.0f;
    float ringPhase = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb shimmerReverb;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DryWetMixer<float> dryWet;

    mfx::Flanger flanger;
    mfx::Vibrato vibrato;
    mfx::WowFlutter wowFlutter;
    mfx::TapeStop tapeStop;
    mfx::Granular granular;
    mfx::PitchShifter pitcher;
    mfx::PitchShifter shimmerShifter;
    mfx::Reverser reverser;
    mfx::Stutter stutter;

    juce::AudioBuffer<float> shimBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MelodikAudioProcessor)
};

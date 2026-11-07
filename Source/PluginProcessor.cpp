#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <array>

static const char* kIds[6]   = { "espacio", "textura", "movimiento", "suciedad", "tono", "mezcla" };
static const char* kNames[6] = { "Espacio", "Textura", "Movimiento", "Suciedad", "Tono", "Mezcla" };

static const char* kFxIds[11] = { "fx_flanger", "fx_phaser", "fx_vibrato", "fx_wow", "fx_tapestop",
                                  "fx_granular", "fx_pitch", "fx_shimmer", "fx_ring", "fx_reverse", "fx_stutter" };
static const char* kFxNames[11] = { "Flanger", "Phaser", "Vibrato", "Wow y flutter", "Tape stop",
                                    "Granular", "Pitch shifter", "Shimmer", "Ring modulator", "Reverse", "Stutter" };

const std::vector<Preset>& MelodikAudioProcessor::getPresets()
{
    static const std::vector<Preset> presets =
    {
        { "Limpio",           { 0.25f, 0.10f, 0.00f, 0.00f, 0.90f, 0.40f }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Cueva",            { 0.90f, 0.10f, 0.15f, 0.05f, 0.25f, 0.70f }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Radio vieja",      { 0.15f, 0.20f, 0.10f, 0.70f, 0.35f, 0.90f }, { 0, 0, 0, 0.3f, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Submarino",        { 0.60f, 0.80f, 0.50f, 0.10f, 0.30f, 0.80f }, { 0, 0, 0.3f, 0, 0, 0, 0.5f, 0, 0, 0, 0 }, -12, false },
        { "Catedral",         { 1.00f, 0.20f, 0.05f, 0.00f, 0.75f, 0.65f }, { 0, 0, 0, 0, 0, 0, 0, 0.6f, 0, 0, 0 }, 0, false },
        { "Cinta rota",       { 0.35f, 0.60f, 0.60f, 0.55f, 0.40f, 0.85f }, { 0, 0, 0, 0.7f, 0.6f, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Cajita de musica", { 0.50f, 0.30f, 0.10f, 0.15f, 1.00f, 0.60f }, { 0, 0, 0, 0, 0, 0, 0.6f, 0, 0, 0, 0 }, 12, false },
        { "Nostalgia VHS",    { 0.30f, 0.70f, 0.35f, 0.50f, 0.45f, 0.80f }, { 0, 0.2f, 0, 0.6f, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Espacio profundo", { 1.00f, 0.50f, 0.70f, 0.10f, 0.50f, 0.90f }, { 0, 0, 0, 0, 0, 0.7f, 0, 0.5f, 0, 0, 0 }, 0, true },
        { "Destrozado",       { 0.20f, 0.40f, 0.80f, 1.00f, 0.60f, 1.00f }, { 0, 0, 0, 0, 0, 0, 0, 0, 0.3f, 0, 0.7f }, 0, false },
        { "Robot",            { 0.15f, 0.10f, 0.20f, 0.30f, 0.70f, 0.90f }, { 0, 0, 0, 0, 0, 0, 0, 0, 0.7f, 0, 0 }, 0, false },
        { "Nubes",            { 0.70f, 0.30f, 0.40f, 0.05f, 0.60f, 0.85f }, { 0, 0, 0, 0, 0, 0.8f, 0, 0, 0, 0, 0 }, 0, false },
        { "Reversa onirica",  { 0.80f, 0.30f, 0.30f, 0.10f, 0.55f, 0.80f }, { 0, 0, 0, 0, 0, 0, 0, 0.3f, 0, 0.8f, 0 }, 0, false },
        { "Jet",              { 0.30f, 0.20f, 0.40f, 0.00f, 0.85f, 0.70f }, { 0.8f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Psicodelia",       { 0.50f, 0.50f, 0.60f, 0.15f, 0.70f, 0.85f }, { 0.3f, 0.8f, 0.3f, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Frenado",          { 0.30f, 0.30f, 0.10f, 0.30f, 0.50f, 1.00f }, { 0, 0, 0, 0, 1.0f, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Glitch",           { 0.25f, 0.10f, 0.30f, 0.40f, 0.80f, 0.90f }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0f }, 0, false },
        { "Armonia dulce",    { 0.55f, 0.30f, 0.10f, 0.00f, 0.85f, 0.70f }, { 0, 0, 0, 0, 0, 0, 0.6f, 0.2f, 0, 0, 0 }, 7, false },
        { "Octava abajo",     { 0.40f, 0.20f, 0.10f, 0.20f, 0.60f, 0.80f }, { 0, 0, 0, 0, 0, 0, 0.8f, 0, 0, 0, 0 }, -12, false },
        { "Mundo alien",      { 0.60f, 0.60f, 0.70f, 0.30f, 0.80f, 0.90f }, { 0, 0, 0.6f, 0, 0, 0.4f, 0, 0, 0.5f, 0, 0 }, 0, false },
        { "Ensueno",          { 0.80f, 0.80f, 0.30f, 0.00f, 0.65f, 0.80f }, { 0.2f, 0, 0, 0, 0, 0, 0, 0.7f, 0, 0, 0 }, 0, false },
        { "Sotano",           { 0.50f, 0.20f, 0.20f, 0.60f, 0.20f, 0.90f }, { 0, 0, 0, 0.4f, 0.3f, 0, 0, 0, 0, 0, 0 }, 0, false },
        { "Tormenta",         { 0.90f, 0.40f, 0.90f, 0.50f, 0.45f, 0.90f }, { 0, 0, 0, 0, 0, 0.5f, 0, 0, 0.2f, 0.5f, 0 }, 0, false },
        { "Caos",             { 0.60f, 0.70f, 0.90f, 0.80f, 0.70f, 1.00f }, { 0, 0, 0, 0, 0.4f, 0.5f, 0, 0, 0.4f, 0.6f, 0.6f }, 0, false }
    };
    return presets;
}

juce::AudioProcessorValueTreeState::ParameterLayout MelodikAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    const float defaults[6] = { 0.40f, 0.20f, 0.10f, 0.00f, 0.85f, 0.50f };

    for (int i = 0; i < 6; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kIds[i], 1 }, kNames[i],
            juce::NormalisableRange<float> (0.0f, 1.0f), defaults[i]));

    for (int i = 0; i < 11; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { kFxIds[i], 1 }, kFxNames[i],
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "fx_pitchst", 1 }, "Pitch semitonos", -12, 12, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "fx_freeze", 1 }, "Congelar", false));

    return { params.begin(), params.end() };
}

MelodikAudioProcessor::MelodikAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MELODIK", createLayout())
{
}

void MelodikAudioProcessor::setParam (const char* id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

void MelodikAudioProcessor::applyPreset (int index)
{
    auto& presets = getPresets();
    index = juce::jlimit (0, (int) presets.size() - 1, index);
    const auto& pr = presets[(size_t) index];

    for (int i = 0; i < 6; ++i)
        setParam (kIds[i], pr.v[i]);
    for (int i = 0; i < 11; ++i)
        setParam (kFxIds[i], pr.fx[i]);

    setParam ("fx_pitchst", (float) pr.pitchSt);
    setParam ("fx_freeze", pr.freeze ? 1.0f : 0.0f);

    currentPreset.store (index);
    apvts.state.setProperty ("presetIndex", index, nullptr);
}

void MelodikAudioProcessor::randomize()
{
    juce::Random rng;

    setParam ("espacio",    0.10f + 0.90f * rng.nextFloat());
    setParam ("textura",    rng.nextFloat());
    setParam ("movimiento", 0.90f * rng.nextFloat());
    setParam ("suciedad",   0.70f * rng.nextFloat());
    setParam ("tono",       0.30f + 0.70f * rng.nextFloat());
    setParam ("mezcla",     0.60f + 0.40f * rng.nextFloat());

    std::array<int, 11> idx;
    for (int i = 0; i < 11; ++i) idx[(size_t) i] = i;
    for (int i = 10; i > 0; --i)
        std::swap (idx[(size_t) i], idx[(size_t) rng.nextInt (i + 1)]);

    const int count = 3 + rng.nextInt (3);
    std::array<float, 11> amounts;
    amounts.fill (0.0f);
    for (int k = 0; k < count; ++k)
        amounts[(size_t) idx[(size_t) k]] = 0.40f + 0.60f * rng.nextFloat();

    for (int i = 0; i < 11; ++i)
        setParam (kFxIds[i], amounts[(size_t) i]);

    static const int steps[6] = { -12, -7, -5, 5, 7, 12 };
    setParam ("fx_pitchst", (float) steps[rng.nextInt (6)]);
    setParam ("fx_freeze", (amounts[5] > 0.0f && rng.nextFloat() < 0.3f) ? 1.0f : 0.0f);

    currentPreset.store (-1);
    apvts.state.setProperty ("presetIndex", -1, nullptr);
}

void MelodikAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    lfoPhase = 0.0f;
    ringPhase = 0.0f;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);

    phaser.prepare (spec);
    phaser.setCentreFrequency (700.0f);

    delay.prepare (spec);
    delay.setMaximumDelayInSamples ((int) (sampleRate * 1.5));
    delay.reset();

    reverb.prepare (spec);
    shimmerReverb.prepare (spec);

    limiter.prepare (spec);
    limiter.setThreshold (-1.0f);
    limiter.setRelease (60.0f);

    dryWet.prepare (spec);
    dryWet.setMixingRule (juce::dsp::DryWetMixingRule::linear);

    flanger.prepare (sampleRate);
    vibrato.prepare (sampleRate);
    wowFlutter.prepare (sampleRate);
    tapeStop.prepare (sampleRate);
    granular.prepare (sampleRate);
    pitcher.prepare (sampleRate);
    shimmerShifter.prepare (sampleRate);
    reverser.prepare (sampleRate);
    stutter.prepare (sampleRate);

    shimBuf.setSize (2, juce::jmax (1, samplesPerBlock));
    shimBuf.clear();
}

bool MelodikAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void MelodikAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float esp = get ("espacio");
    const float tex = get ("textura");
    const float mov = get ("movimiento");
    const float suc = get ("suciedad");
    const float ton = get ("tono");
    const float mez = get ("mezcla");

    float fx[11];
    for (int i = 0; i < 11; ++i)
        fx[i] = get (kFxIds[i]);

    const int pitchSt = (int) std::lround (get ("fx_pitchst"));
    const bool freeze = get ("fx_freeze") > 0.5f;

    const float nyquistLimit = (float) (0.45 * sampleRateHz);
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    juce::dsp::AudioBlock<float> block (buffer);
    dryWet.setWetMixProportion (mez);
    dryWet.pushDrySamples (block);

    // 1) Filtro de tono con LFO (movimiento) + tremolo suave
    const float baseCut = juce::jmin (300.0f * std::pow (2.0f, ton * 6.0f), nyquistLimit);
    const float lfoRate = 0.3f + mov * 4.0f;
    const int chunk = 32;

    for (int start = 0; start < numSamples; start += chunk)
    {
        const int n = juce::jmin (chunk, numSamples - start);

        lfoPhase += juce::MathConstants<float>::twoPi * lfoRate * (float) n / (float) sampleRateHz;
        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const float lfo = std::sin (lfoPhase);
        const float cutoff = juce::jlimit (60.0f, nyquistLimit, baseCut * std::pow (2.0f, lfo * mov * 2.0f));
        filter.setCutoffFrequency (cutoff);

        auto sub = block.getSubBlock ((size_t) start, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        filter.process (ctx);
        sub.multiplyBy (1.0f - 0.35f * mov * (0.5f + 0.5f * lfo));
    }

    // 2) Suciedad: saturacion + reduccion de bits
    if (suc > 0.01f)
    {
        const float drive = 1.0f + suc * 14.0f;
        const float norm = 1.0f / std::tanh (drive);
        const bool crush = suc > 0.6f;
        const float levels = std::pow (2.0f, 16.0f - ((suc - 0.6f) / 0.4f) * 10.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float x = std::tanh (drive * data[i]) * norm;
                if (crush)
                    x = std::round (x * levels) / levels;
                data[i] = x;
            }
        }
    }

    // 3) Ring modulator
    if (fx[8] > 0.01f)
    {
        const float a = fx[8];
        const float inc = juce::MathConstants<float>::twoPi * (80.0f + 700.0f * a) / (float) sampleRateHz;
        for (int i = 0; i < numSamples; ++i)
        {
            ringPhase += inc;
            if (ringPhase > juce::MathConstants<float>::twoPi)
                ringPhase -= juce::MathConstants<float>::twoPi;
            const float c = std::sin (ringPhase);
            L[i] = L[i] * (1.0f - a) + L[i] * c * a;
            R[i] = R[i] * (1.0f - a) + R[i] * c * a;
        }
    }

    // 4) Pitch shifter
    if (fx[6] > 0.01f)
    {
        const float a = fx[6];
        const float ratio = std::pow (2.0f, (float) pitchSt / 12.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            float y0 = 0.0f, y1 = 0.0f;
            pitcher.processSample (L[i], R[i], ratio, y0, y1);
            L[i] = L[i] * (1.0f - 0.4f * a) + y0 * a * 0.9f;
            R[i] = R[i] * (1.0f - 0.4f * a) + y1 * a * 0.9f;
        }
    }

    // 5) Vibrato, wow & flutter, flanger
    if (fx[2] > 0.01f) vibrato.process (L, R, numSamples, fx[2]);
    if (fx[3] > 0.01f) wowFlutter.process (L, R, numSamples, fx[3]);
    if (fx[0] > 0.01f) flanger.process (L, R, numSamples, fx[0]);

    // 6) Phaser
    if (fx[1] > 0.01f)
    {
        const float a = fx[1];
        phaser.setRate (0.2f + a * 1.5f);
        phaser.setDepth (0.4f + 0.5f * a);
        phaser.setFeedback (0.4f * a);
        phaser.setMix (0.3f + 0.6f * a);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        phaser.process (ctx);
    }

    // 7) Textura: chorus
    if (tex > 0.01f)
    {
        chorus.setRate (0.3f + tex * 2.2f);
        chorus.setDepth (0.2f + tex * 0.6f);
        chorus.setFeedback (0.1f * tex);
        chorus.setMix (0.25f + tex * 0.5f);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    // 8) Granular, reverse, stutter, tape stop
    if (fx[5] > 0.01f) granular.process (L, R, numSamples, fx[5], freeze);
    if (fx[9] > 0.01f) reverser.process (L, R, numSamples, fx[9]);

    if (fx[10] > 0.01f) stutter.process (L, R, numSamples, fx[10]);
    else stutter.reset();

    if (fx[4] > 0.01f) tapeStop.process (L, R, numSamples, fx[4]);
    else tapeStop.reset();

    // 9) Espacio: delay con realimentacion + reverb
    if (esp > 0.01f)
    {
        delay.setDelay ((0.22f + 0.20f * esp) * (float) sampleRateHz);
        const float fb = 0.15f + 0.50f * esp;
        const float dm = 0.40f * esp;
        float* chans[2] = { L, R };

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const float in = chans[ch][i];
                const float d = delay.popSample (ch);
                delay.pushSample (ch, in + d * fb);
                chans[ch][i] = in + d * dm;
            }
        }

        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = 0.30f + esp * 0.69f;
        rp.damping = 0.60f - ton * 0.30f;
        rp.wetLevel = esp * 0.5f;
        rp.dryLevel = 1.0f;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        juce::dsp::ProcessContextReplacing<float> ctx (block);
        reverb.process (ctx);
    }

    // 10) Shimmer: octava arriba dentro de una reverb grande
    if (fx[7] > 0.01f)
    {
        const float a = fx[7];
        if (shimBuf.getNumSamples() < numSamples)
            shimBuf.setSize (2, numSamples, false, false, true);

        float* S0 = shimBuf.getWritePointer (0);
        float* S1 = shimBuf.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
            shimmerShifter.processSample (L[i], R[i], 2.0f, S0[i], S1[i]);

        juce::dsp::Reverb::Parameters sp;
        sp.roomSize = 0.95f;
        sp.damping = 0.35f;
        sp.wetLevel = 1.0f;
        sp.dryLevel = 0.0f;
        sp.width = 1.0f;
        sp.freezeMode = 0.0f;
        shimmerReverb.setParameters (sp);

        juce::dsp::AudioBlock<float> sb (shimBuf);
        auto sbSub = sb.getSubBlock (0, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> sctx (sbSub);
        shimmerReverb.process (sctx);

        for (int i = 0; i < numSamples; ++i)
        {
            L[i] += S0[i] * a * 0.9f;
            R[i] += S1[i] * a * 0.9f;
        }
    }

    // 11) Mezcla seco/humedo y limitador final
    dryWet.mixWetSamples (block);

    juce::dsp::ProcessContextReplacing<float> limCtx (block);
    limiter.process (limCtx);
}

juce::AudioProcessorEditor* MelodikAudioProcessor::createEditor()
{
    return new MelodikAudioProcessorEditor (*this);
}

void MelodikAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MelodikAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        currentPreset.store ((int) apvts.state.getProperty ("presetIndex", 0));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MelodikAudioProcessor();
}

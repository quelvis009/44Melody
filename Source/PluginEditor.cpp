#include "PluginEditor.h"

static const char* kIds[6]    = { "espacio", "textura", "movimiento", "suciedad", "tono", "mezcla" };
static const char* kLabels[6] = { "ESPACIO", "TEXTURA", "MOVIMIENTO", "SUCIEDAD", "TONO", "MEZCLA" };
static const juce::Colour kAmber (0xfff4a63c);

void MelodikLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    const float cx = (float) x + (float) width * 0.5f;
    const float cy = (float) y + (float) height * 0.5f;
    const float r = juce::jmin (width, height) * 0.5f - 11.0f;

    for (int k = 0; k < 6; ++k)
    {
        g.setColour (juce::Colours::black.withAlpha (0.07f));
        const float e = (float) k;
        g.fillEllipse (cx - r - e, cy - r + 4.0f + e, 2.0f * (r + e), 2.0f * (r + e));
    }

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = rotaryStartAngle + ((float) i / 10.0f) * (rotaryEndAngle - rotaryStartAngle);
        const float px = cx + (r + 10.0f) * std::sin (a);
        const float py = cy - (r + 10.0f) * std::cos (a);
        g.fillEllipse (px - 1.5f, py - 1.5f, 3.0f, 3.0f);
    }

    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    g.setColour (juce::Colour (0xff2c2c2c));
    g.drawEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r, 3.0f);

    const float a = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Line<float> line (cx + 8.0f * std::sin (a), cy - 8.0f * std::cos (a),
                            cx + (r - 6.0f) * std::sin (a), cy - (r - 6.0f) * std::cos (a));
    g.setColour (kAmber);
    g.drawLine (line, 3.5f);
}

MelodikAudioProcessorEditor::MelodikAudioProcessorEditor (MelodikAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (680, 400);

    for (int i = 0; i < 6; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        k.setLookAndFeel (&lnf);
        addAndMakeVisible (k);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kIds[i], k);
    }

    for (auto* b : { &prevButton, &nextButton, &randomButton, &freezeButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0d0d0d));
        b->setColour (juce::TextButton::textColourOffId, kAmber);
        addAndMakeVisible (*b);
    }
    freezeButton.setClickingTogglesState (true);
    freezeButton.setColour (juce::TextButton::buttonOnColourId, kAmber);
    freezeButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff0d0d0d));
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "fx_freeze", freezeButton);

    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    randomButton.onClick = [this] { proc.randomize(); };
    shownPreset = proc.getCurrentPresetIndex();

    startTimerHz (20);
}

MelodikAudioProcessorEditor::~MelodikAudioProcessorEditor()
{
    stopTimer();
    for (auto& k : knobs)
        k.setLookAndFeel (nullptr);
}

void MelodikAudioProcessorEditor::stepPreset (int delta)
{
    const int n = (int) MelodikAudioProcessor::getPresets().size();
    int cur = proc.getCurrentPresetIndex();
    if (cur < 0) cur = (delta > 0) ? -1 : 0;
    proc.applyPreset ((cur + delta + n) % n);
}

void MelodikAudioProcessorEditor::timerCallback()
{
    const int idx = proc.getCurrentPresetIndex();
    if (idx != shownPreset)
    {
        shownPreset = idx;
        repaint();
    }
}

void MelodikAudioProcessorEditor::resized()
{
    for (int i = 0; i < 6; ++i)
        knobs[i].setBounds (80 + i * 104 - 44, 240 - 44, 88, 88);

    prevButton.setBounds (446, 86, 34, 28);
    nextButton.setBounds (610, 86, 34, 28);
    randomButton.setBounds (205, 338, 130, 34);
    freezeButton.setBounds (345, 338, 130, 34);
}

void MelodikAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    const int w = getWidth(), h = getHeight();

    juce::ColourGradient grad (juce::Colour (0xfff4a63c), 0.0f, 0.0f,
                               juce::Colour (0xffd9821a), 0.0f, (float) h, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 16.0f);

    juce::Path clip;
    clip.addRoundedRectangle (bounds, 16.0f);
    g.saveState();
    g.reduceClipRegion (clip);
    for (int x = 4; x < w; x += 7)
    {
        g.setColour (juce::Colours::black.withAlpha (0.09f));
        g.fillRect (x, 4, 2, h - 8);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRect (x + 4, 4, 1, h - 8);
    }
    g.restoreState();

    g.setColour (juce::Colour (0xff111111));
    g.drawRoundedRectangle (bounds, 16.0f, 4.0f);

    // Grafiti
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    {
        juce::Path p1;
        p1.startNewSubPath (470, 150);
        p1.cubicTo (500, 120, 530, 175, 560, 140);
        p1.cubicTo (590, 105, 610, 120, 640, 150);
        g.strokePath (p1, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path p2;
        p2.startNewSubPath (30, 355);
        p2.cubicTo (60, 330, 80, 380, 110, 350);
        p2.cubicTo (140, 320, 150, 335, 175, 360);
        g.strokePath (p2, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path p3;
        p3.startNewSubPath (600, 205);
        p3.lineTo (625, 180);
        p3.lineTo (618, 215);
        p3.lineTo (645, 200);
        g.strokePath (p3, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path p4;
        p4.startNewSubPath (545, 365);
        p4.cubicTo (560, 345, 585, 345, 595, 365);
        p4.cubicTo (605, 385, 630, 385, 645, 360);
        g.strokePath (p4, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.drawEllipse (44.0f, 151.0f, 28.0f, 28.0f, 5.0f);
        g.drawLine (45.0f, 178.0f, 72.0f, 152.0f, 4.0f);
    }

    // Titulo con sombra ancha y difuminada
    g.setFont (juce::Font (juce::FontOptions ("Impact", 62.0f, juce::Font::plain)));
    for (int k = 1; k <= 10; ++k)
    {
        g.setColour (juce::Colours::black.withAlpha (0.05f));
        g.drawFittedText ("44MELODIK", juce::Rectangle<int> (40 + 2 + k, 28 + 4 + k, 360, 90),
                          juce::Justification::centredLeft, 1, 0.6f);
    }
    g.setColour (juce::Colours::black);
    g.drawFittedText ("44MELODIK", juce::Rectangle<int> (40, 28, 360, 90),
                      juce::Justification::centredLeft, 1, 0.6f);

    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Propiedad de Keywyll", 190, 112, 200, 16, juce::Justification::centredRight);

    // Pantalla de presets
    g.setColour (juce::Colour (0xf0141414));
    g.fillRoundedRectangle (440.0f, 38.0f, 204.0f, 82.0f, 10.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (440.0f, 38.0f, 204.0f, 82.0f, 10.0f, 2.0f);

    const auto& presets = MelodikAudioProcessor::getPresets();
    const bool isRandom = shownPreset < 0;
    const int idx = juce::jlimit (0, (int) presets.size() - 1, isRandom ? 0 : shownPreset);

    g.setColour (juce::Colour (0xffc98a2e));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("PRESET", 456, 44, 172, 14, juce::Justification::centred);
    g.drawText ((isRandom ? juce::String ("--") : juce::String (idx + 1)) + " / " + juce::String ((int) presets.size()),
                486, 94, 114, 14, juce::Justification::centred);

    g.setColour (kAmber);
    g.setFont (juce::Font (juce::FontOptions ("Impact", 20.0f, juce::Font::plain)));
    g.drawText (isRandom ? juce::String ("ALEATORIO") : juce::String (presets[(size_t) idx].name),
                456, 58, 172, 28, juce::Justification::centred);

    // Etiquetas de perillas
    g.setColour (juce::Colours::black);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    for (int i = 0; i < 6; ++i)
        g.drawText (kLabels[i], 80 + i * 104 - 52, 296, 104, 16, juce::Justification::centred);
}

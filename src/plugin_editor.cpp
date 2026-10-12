#include "plugin_editor.h"
#include "plugin_processor.h"
#include "preset_library_full.h"
#include "preset_data.h"
#include "user_preset_manager.h"
#include "fx_engine.h"
#include "synth_engine.h"

namespace opensynth {

// Forward declaration — defined in preset_data.cpp
juce::File getSampleManifestPath(const char* id);

// ── SynthKnob ───────────────────────────────────────────────────────────────

SynthKnob::SynthKnob(const juce::String& name, juce::Colour accent)
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
      name_(name), accent_(accent)
{
    setRange(0.0, 1.0, 0.01);
    setPopupDisplayEnabled(true, true, nullptr);
}

void SynthKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 10.0f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = size * 0.5f;

    // Glow effect behind knob
    juce::Path glow;
    glow.addCentredArc(cx, cy, radius + 6, radius + 6, 0.0f, 0.0f, 6.283f, true);
    g.setColour(accent_.withAlpha(0.08f));
    g.fillPath(glow);

    g.setColour(SynthColors::surface());
    g.fillEllipse(cx - radius, cy - radius, size, size);

    // Outer ring
    g.setColour(accent_.withAlpha(0.4f));
    g.drawEllipse(cx - radius, cy - radius, size, size, 1.5f);

    float value = (float)getValue();
    float startAngle = 2.356f;
    float endAngle = startAngle + value * 4.712f;

    juce::Path arc;
    arc.addCentredArc(cx, cy, radius - 3, radius - 3, 0.0f, startAngle, endAngle, true);
    g.setColour(accent_);
    g.strokePath(arc, juce::PathStrokeType(4.0f));

    juce::Path arcDim;
    arcDim.addCentredArc(cx, cy, radius - 3, radius - 3, 0.0f, endAngle, startAngle + 4.712f, true);
    g.setColour(SynthColors::gridLine());
    g.strokePath(arcDim, juce::PathStrokeType(2.0f));

    // Value indicator dot
    float dotAngle = endAngle;
    float dotX = cx + (radius - 8) * std::cos(dotAngle);
    float dotY = cy + (radius - 8) * std::sin(dotAngle);
    g.setColour(accent_.brighter());
    g.fillEllipse(dotX - 3, dotY - 3, 6, 6);

    g.setColour(SynthColors::textDim());
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(name_, cx - radius, cy + radius + 4, size, 14, juce::Justification::centred);

    // Value display centered inside the knob
    float val = static_cast<float>(getValue());
    float range = static_cast<float>(getMaximum() - getMinimum());
    juce::String valText;
    if (range <= 0.0f)
        valText = "";
    else if (getInterval() >= 1.0f)
        valText = juce::String(static_cast<int>(std::round(val)));
    else if (range > 100.0f)
        valText = juce::String(val, 0);
    else if (range > 10.0f)
        valText = juce::String(val, 1);
    else
        valText = juce::String(val, 2);

    g.setColour(SynthColors::text().withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(valText, cx - radius, cy - 4, size, 10, juce::Justification::centred);
}

void SynthKnob::resized() {}

// ── OscPanel ────────────────────────────────────────────────────────────────

OscPanel::OscPanel(juce::AudioProcessorValueTreeState& apvts, int oscIndex)
    : apvts_(apvts), oscIndex_(oscIndex),
      waveformKnob_("Wave", SynthColors::neonPurple()),
      octaveKnob_("Octave", SynthColors::neonPurple()),
      detuneKnob_("Detune", SynthColors::hotPink()),
      volumeKnob_("Volume", SynthColors::neonYellow())
{
    addAndMakeVisible(waveformKnob_);
    addAndMakeVisible(octaveKnob_);
    addAndMakeVisible(detuneKnob_);
    addAndMakeVisible(volumeKnob_);

    waveformSelector_.addItemList({"Sine", "Triangle", "Saw", "Square", "Pulse",
        "Noise", "Sub", "FM", "Wavetable", "Physical Model"}, 1);
    addAndMakeVisible(waveformSelector_);

    juce::String prefix = oscIndex == 1 ? "osc1" : "osc2";
    waveformAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, prefix + "Waveform", waveformSelector_);
    octaveAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Octave", octaveKnob_);
    detuneAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Detune", detuneKnob_);
    volumeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Volume", volumeKnob_);
}

void OscPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 8.0f);

    // Neon border glow
    g.setColour(SynthColors::neonPurple().withAlpha(0.3f));
    g.drawRoundedRectangle(b.reduced(1.0f), 8.0f, 1.5f);

    g.setColour(SynthColors::neonPurple());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    juce::String title = oscIndex_ == 1 ? "OSCILLATOR 1" : "OSCILLATOR 2";
    g.drawText(title, 12, 8, 120, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void OscPanel::resized()
{
    int knobSize = 56, gap = 8;
    waveformSelector_.setBounds(12, 36, 100, 24);
    int x = 12, y = 68;
    waveformKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    octaveKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    detuneKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    volumeKnob_.setBounds(x, y, knobSize, knobSize + 16);
}

// ── FilterPanel ─────────────────────────────────────────────────────────────

FilterPanel::FilterPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts),
      cutoffKnob_("Cutoff", SynthColors::cyan()),
      resonanceKnob_("Resonance", SynthColors::cyan()),
      envAmtKnob_("Env Amt", SynthColors::hotPink()),
      driveKnob_("Drive", SynthColors::neonYellow())
{
    addAndMakeVisible(cutoffKnob_);
    addAndMakeVisible(resonanceKnob_);
    addAndMakeVisible(envAmtKnob_);
    addAndMakeVisible(driveKnob_);

    cutoffAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "filterCutoff", cutoffKnob_);
    resonanceAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "filterResonance", resonanceKnob_);
    envAmtAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "filterEnvAmt", envAmtKnob_);
    driveAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "filterDrive", driveKnob_);
}

void FilterPanel::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(SynthColors::cyan());
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("FILTER", 12, 8, 100, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void FilterPanel::resized()
{
    int knobSize = 56, gap = 8, x = 12, y = 68;
    cutoffKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    resonanceKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    envAmtKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    driveKnob_.setBounds(x, y, knobSize, knobSize + 16);
}

// ── EnvelopePanel ───────────────────────────────────────────────────────────

EnvelopePanel::EnvelopePanel(juce::AudioProcessorValueTreeState& apvts, const juce::String& name)
    : apvts_(apvts), name_(name),
      attackKnob_("Attack", SynthColors::neonYellow()),
      decayKnob_("Decay", SynthColors::neonYellow()),
      sustainKnob_("Sustain", SynthColors::neonYellow()),
      releaseKnob_("Release", SynthColors::neonYellow())
{
    addAndMakeVisible(attackKnob_);
    addAndMakeVisible(decayKnob_);
    addAndMakeVisible(sustainKnob_);
    addAndMakeVisible(releaseKnob_);

    juce::String prefix = name == "AMP" ? "amp" : "filter";
    attackAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Attack", attackKnob_);
    decayAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Decay", decayKnob_);
    sustainAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Sustain", sustainKnob_);
    releaseAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, prefix + "Release", releaseKnob_);
}

void EnvelopePanel::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(SynthColors::neonYellow());
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText(name_ + " ENVELOPE", 12, 8, 200, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void EnvelopePanel::resized()
{
    int knobSize = 56, gap = 8, x = 12, y = 68;
    attackKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    decayKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    sustainKnob_.setBounds(x, y, knobSize, knobSize + 16); x += knobSize + gap;
    releaseKnob_.setBounds(x, y, knobSize, knobSize + 16);
}

// ── FxSlotPanel ─────────────────────────────────────────────────────────────

FxSlotPanel::FxSlotPanel(juce::AudioProcessorValueTreeState& apvts, int slotIndex)
    : apvts_(apvts), slotIndex_(slotIndex),
      paramKnobs_{{"P1", SynthColors::neonPurple()}, {"P2", SynthColors::hotPink()},
                   {"P3", SynthColors::cyan()}, {"P4", SynthColors::neonYellow()}}
{
    enabledButton_.setButtonText("ON");
    enabledButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::neonPurple());
    addAndMakeVisible(enabledButton_);

    populateFxTypes();
    addAndMakeVisible(typeSelector_);

    for (auto& knob : paramKnobs_)
        addAndMakeVisible(knob);

    juce::String prefix = "fx" + juce::String(slotIndex);
    enabledAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, prefix + "Enabled", enabledButton_);
    typeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, prefix + "Type", typeSelector_);

    for (int i = 0; i < 4; ++i)
    {
        paramAttaches_[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "Param" + juce::String(i), paramKnobs_[i]);
    }

    // Update knob labels when FX type changes
    typeSelector_.onChange = [this]() {
        updateParamLabels(typeSelector_.getSelectedItemIndex());
    };

    // Initialize knob labels from current type
    updateParamLabels(typeSelector_.getSelectedItemIndex());
}

void FxSlotPanel::populateFxTypes()
{
    typeSelector_.addItem("None", 1);

    typeSelector_.addSectionHeading("── Modulation ──");
    typeSelector_.addItem("Chorus", 2);
    typeSelector_.addItem("Chorus Ensemble", 35);
    typeSelector_.addItem("Dimension D", 36);
    typeSelector_.addItem("Flanger", 6);
    typeSelector_.addItem("Phaser", 5);
    typeSelector_.addItem("Uni-Vibe", 34);
    typeSelector_.addItem("Tremolo", 12);
    typeSelector_.addItem("Vibrato", 32);
    typeSelector_.addItem("Auto-Pan", 33);
    typeSelector_.addItem("Rotary", 11);

    typeSelector_.addSectionHeading("── Delay ──");
    typeSelector_.addItem("Delay", 3);
    typeSelector_.addItem("Multi-tap Delay", 17);
    typeSelector_.addItem("Ping-Pong Delay", 18);
    typeSelector_.addItem("Tape Delay", 38);
    typeSelector_.addItem("Analog Delay", 39);
    typeSelector_.addItem("Diffusion Delay", 40);
    typeSelector_.addItem("Reverse Delay", 37);
    typeSelector_.addItem("Grain Delay", 65);

    typeSelector_.addSectionHeading("── Reverb ──");
    typeSelector_.addItem("Reverb", 4);
    typeSelector_.addItem("Room Reverb", 41);
    typeSelector_.addItem("Hall Reverb", 42);
    typeSelector_.addItem("Plate Reverb", 43);
    typeSelector_.addItem("Spring Reverb", 19);
    typeSelector_.addItem("Shimmer Reverb", 44);
    typeSelector_.addItem("Gated Reverb", 20);
    typeSelector_.addItem("Non-Linear Reverb", 45);

    typeSelector_.addSectionHeading("── Distortion ──");
    typeSelector_.addItem("Drive", 8);
    typeSelector_.addItem("Distortion", 24);
    typeSelector_.addItem("Overdrive", 25);
    typeSelector_.addItem("Fuzz", 26);
    typeSelector_.addItem("Tube Drive", 27);

    typeSelector_.addSectionHeading("── Filter ──");
    typeSelector_.addItem("EQ", 9);
    typeSelector_.addItem("Graphic EQ", 58);
    typeSelector_.addItem("Parametric EQ", 59);
    typeSelector_.addItem("Resonant Filter", 28);
    typeSelector_.addItem("Formant Filter", 29);
    typeSelector_.addItem("Comb Filter", 30);
    typeSelector_.addItem("Wah-Wah", 60);
    typeSelector_.addItem("Auto-Wah", 13);

    typeSelector_.addSectionHeading("── Dynamics ──");
    typeSelector_.addItem("Compressor", 7);
    typeSelector_.addItem("Multiband Comp", 52);
    typeSelector_.addItem("Limiter", 10);
    typeSelector_.addItem("Maximizer", 61);
    typeSelector_.addItem("Noise Gate", 49);
    typeSelector_.addItem("De-Esser", 50);
    typeSelector_.addItem("Transient Shaper", 51);

    typeSelector_.addSectionHeading("── Pitch ──");
    typeSelector_.addItem("Pitch Shift", 16);
    typeSelector_.addItem("Harmonizer", 46);
    typeSelector_.addItem("Octaver", 47);
    typeSelector_.addItem("Detune", 48);
    typeSelector_.addItem("Ring Mod", 15);
    typeSelector_.addItem("Talk Box", 31);
    typeSelector_.addItem("Vocoder", 23);

    typeSelector_.addSectionHeading("── Amp & Cabinet ──");
    typeSelector_.addItem("Amp Sim", 21);
    typeSelector_.addItem("Cabinet Sim", 57);

    typeSelector_.addSectionHeading("── Lo-Fi & Simulation ──");
    typeSelector_.addItem("Bitcrusher", 14);
    typeSelector_.addItem("Lo-Fi", 53);
    typeSelector_.addItem("Vinyl Sim", 54);
    typeSelector_.addItem("Radio Sim", 55);
    typeSelector_.addItem("Telephone Sim", 56);

    typeSelector_.addSectionHeading("── Spatial ──");
    typeSelector_.addItem("Stereo Widener", 22);
    typeSelector_.addItem("Stereo Imager", 63);
    typeSelector_.addItem("Exciter", 62);
    typeSelector_.addItem("Resonator", 64);
    typeSelector_.addItem("Spectral Freeze", 66);
}

void FxSlotPanel::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    juce::Colour accent = slotIndex_ == 1 ? SynthColors::neonPurple()
                         : slotIndex_ == 2 ? SynthColors::hotPink()
                         : SynthColors::cyan();
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("FX SLOT " + juce::String(slotIndex_), 12, 8, 120, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void FxSlotPanel::resized()
{
    enabledButton_.setBounds(12, 36, 50, 24);
    typeSelector_.setBounds(68, 36, 120, 24);

    int knobSize = 48, gap = 6, x = 12, y = 68;
    for (auto& knob : paramKnobs_)
    {
        knob.setBounds(x, y, knobSize, knobSize + 14);
        x += knobSize + gap;
    }
}

void FxSlotPanel::updateParamLabels(int fxTypeId)
{
    auto desc = opensynth::FxProcessor::getDescriptor(fxTypeId);
    for (int i = 0; i < 4; ++i)
    {
        if (i < desc.numParams)
        {
            paramKnobs_[i].setName(juce::String(desc.params[i].name));
            paramKnobs_[i].setVisible(true);
        }
        else
        {
            paramKnobs_[i].setName("");
            paramKnobs_[i].setVisible(fxTypeId == 0 ? false : true);
        }
    }
    repaint();
}

// ── ArpPanel ────────────────────────────────────────────────────────────────

ArpPanel::ArpPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts),
      tempoKnob_("Tempo", SynthColors::neonPurple()),
      gateKnob_("Gate", SynthColors::hotPink()),
      swingKnob_("Swing", SynthColors::cyan()),
      octaveKnob_("Octave", SynthColors::neonYellow())
{
    enabledButton_.setButtonText("ARP ON");
    enabledButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::neonPurple());
    addAndMakeVisible(enabledButton_);

    patternSelector_.addItemList({"Up", "Down", "Up/Down", "Random", "Chord",
        "Down/Up", "Played Order", "Ping Pong", "Ping Pong Rev",
        "2-Oct Up", "2-Oct Down", "2-Oct Up/Down",
        "3-Oct Up", "3-Oct Down", "3-Oct Up/Down",
        "Oct Jump Up", "Oct Jump Down",
        "Fifth Up", "Fifth Down", "Fifth Up/Down"}, 1);
    addAndMakeVisible(patternSelector_);

    addAndMakeVisible(tempoKnob_);
    addAndMakeVisible(gateKnob_);
    addAndMakeVisible(swingKnob_);
    addAndMakeVisible(octaveKnob_);

    enabledAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, "arpEnabled", enabledButton_);
    patternAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, "arpPattern", patternSelector_);
    tempoAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "arpTempo", tempoKnob_);
    gateAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "arpGate", gateKnob_);
    swingAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "arpSwing", swingKnob_);
    octaveAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "arpOctave", octaveKnob_);
}

void ArpPanel::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(SynthColors::magenta());
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("ARPEGGIATOR", 12, 8, 120, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void ArpPanel::resized()
{
    enabledButton_.setBounds(12, 36, 70, 24);
    patternSelector_.setBounds(88, 36, 100, 24);

    int knobSize = 48, gap = 6, x = 12, y = 68;
    tempoKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    gateKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    swingKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    octaveKnob_.setBounds(x, y, knobSize, knobSize + 14);
}

// ── RealismPanel ────────────────────────────────────────────────────────────

RealismPanel::RealismPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts),
      bodyMixKnob_("Body Mix", SynthColors::neonPurple()),
      clickMixKnob_("Click", SynthColors::hotPink()),
      sympatheticKnob_("Sympathetic", SynthColors::cyan()),
      brightnessKnob_("Brightness", SynthColors::neonYellow())
{
    bodyTypeSelector_.addItemList({"None", "Piano", "Guitar", "Violin", "Organ", "Brass", "Plucked", "Mallet"}, 1);
    addAndMakeVisible(bodyTypeSelector_);

    addAndMakeVisible(bodyMixKnob_);
    addAndMakeVisible(clickMixKnob_);
    addAndMakeVisible(sympatheticKnob_);
    addAndMakeVisible(brightnessKnob_);

    attackCurveSelector_.addItemList({"Linear", "Exponential", "Logarithmic", "Double-Exp"}, 1);
    addAndMakeVisible(attackCurveSelector_);

    bodyTypeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, "realismBodyType", bodyTypeSelector_);
    bodyMixAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "realismBodyMix", bodyMixKnob_);
    clickMixAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "realismClickMix", clickMixKnob_);
    sympatheticAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "realismSympatheticMix", sympatheticKnob_);
    brightnessAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "realismBrightnessSens", brightnessKnob_);
    attackCurveAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, "realismAttackCurve", attackCurveSelector_);
}

void RealismPanel::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(SynthColors::neonYellow());
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("INSTRUMENT REALISM", 12, 8, 160, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void RealismPanel::resized()
{
    bodyTypeSelector_.setBounds(12, 36, 110, 24);
    attackCurveSelector_.setBounds(128, 36, 120, 24);

    int knobSize = 48, gap = 6, x = 12, y = 68;
    bodyMixKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    clickMixKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    sympatheticKnob_.setBounds(x, y, knobSize, knobSize + 14); x += knobSize + gap;
    brightnessKnob_.setBounds(x, y, knobSize, knobSize + 14);
}

// ── MpePanel ────────────────────────────────────────────────────────────────

MpePanel::MpePanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
    , bendRangeKnob_("Bend", SynthColors::cyan())
{
    enableButton_.setButtonText("MPE");
    enableButton_.setColour(juce::TextButton::buttonOnColourId, SynthColors::cyan());
    enableButton_.setColour(juce::TextButton::textColourOnId, SynthColors::text());
    addAndMakeVisible(enableButton_);

    zoneSelector_.addItem("Lower (Ch 1)", 1);
    zoneSelector_.addItem("Upper (Ch 16)", 2);
    addAndMakeVisible(zoneSelector_);

    addAndMakeVisible(bendRangeKnob_);

    enableAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts_, "mpeEnabled", enableButton_);
    zoneAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts_, "mpeZone", zoneSelector_);
    bendRangeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts_, "mpeBendRange", bendRangeKnob_);
}

void MpePanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 6.0f);

    g.setColour(SynthColors::cyan());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("MPE", b.removeFromTop(28).reduced(8, 0), juce::Justification::centredLeft);
}

void MpePanel::resized()
{
    enableButton_.setBounds(12, 36, 60, 24);
    zoneSelector_.setBounds(78, 36, 110, 24);
    bendRangeKnob_.setBounds(196, 28, 48, 48 + 14);
}

// ── PerformanceMeter ────────────────────────────────────────────────────────

PerformanceMeter::PerformanceMeter()
{
    startTimerHz(10);
}

void PerformanceMeter::setVoiceCount(int count) { voiceCount_ = count; }
void PerformanceMeter::setCpuLoad(float load) { cpuLoad_ = load; }

void PerformanceMeter::timerCallback() { repaint(); }

void PerformanceMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 6.0f);

    // CPU bar
    float cpuNorm = juce::jlimit(0.0f, 1.0f, cpuLoad_ / 100.0f);
    juce::Colour cpuColor = cpuNorm < 0.5f ? SynthColors::neonPurple()
                          : cpuNorm < 0.8f ? SynthColors::neonYellow()
                          : SynthColors::danger();

    g.setColour(cpuColor.withAlpha(0.3f));
    g.fillRoundedRectangle(b.reduced(4), 4.0f);

    g.setColour(cpuColor);
    float barWidth = (b.getWidth() - 8) * cpuNorm;
    g.fillRoundedRectangle(b.getX() + 4, b.getY() + 4, barWidth, b.getHeight() - 8, 4.0f);

    // Text
    g.setColour(SynthColors::text());
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("CPU: " + juce::String(cpuLoad_, 1) + "% | Voices: " + juce::String(voiceCount_),
               b.toNearestInt(), juce::Justification::centred, false);
}

void PerformanceMeter::resized() {}

// ── ScopeComponent ──────────────────────────────────────────────────────────

ScopeComponent::ScopeComponent()
{
    startTimerHz(30);
}

void ScopeComponent::pushBuffer(const std::vector<float>& interleavedBuffer, int numChannels)
{
    if (numChannels < 1 || interleavedBuffer.empty()) return;

    juce::ScopedLock lock(bufferLock_);
    size_t numFrames = interleavedBuffer.size() / numChannels;
    displayBuffer_.resize(numFrames);

    // Mix down to mono for display
    for (size_t i = 0; i < numFrames; ++i)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sum += interleavedBuffer[i * numChannels + ch];
        displayBuffer_[i] = sum / numChannels;
    }
}

void ScopeComponent::timerCallback()
{
    repaint();
}

void ScopeComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 6.0f);

    g.setColour(SynthColors::gridLine().withAlpha(0.3f));
    g.drawHorizontalLine(getHeight() / 2, 0, getWidth());

    juce::ScopedLock lock(bufferLock_);
    if (displayBuffer_.size() < 2) return;

    g.setColour(SynthColors::neonYellow());
    juce::Path path;
    float xStep = b.getWidth() / (float)(displayBuffer_.size() - 1);
    float centerY = b.getCentreY();
    float scale = b.getHeight() * 0.45f;

    path.startNewSubPath(b.getX(), centerY - displayBuffer_[0] * scale);
    for (size_t i = 1; i < displayBuffer_.size(); ++i)
        path.lineTo(b.getX() + i * xStep, centerY - displayBuffer_[i] * scale);

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void ScopeComponent::resized() {}

// ── PresetBrowser ───────────────────────────────────────────────────────────

// Library categories are snake_case ("synth_pad"); filters match whole words
// against the normalized form ("synth pad"), so "Pad" covers every pad variant.
static bool categoryMatchesFilter(const juce::String& presetCategory, const juce::String& filter)
{
    if (filter.isEmpty()) return true;
    juce::String cat = presetCategory.replaceCharacter('_', ' ').toLowerCase();
    if (cat == filter.toLowerCase()) return true;
    return (" " + cat + " ").contains(" " + filter.toLowerCase().trim() + " ");
}

static juce::String prettifyCategory(const juce::String& snakeCase)
{
    juce::StringArray words;
    words.addTokens(snakeCase.replaceCharacter('_', ' '), " ", "");
    for (auto& w : words)
        w = w.substring(0, 1).toUpperCase() + w.substring(1);
    return words.joinIntoString(" ");
}

PresetBrowser::PresetBrowser()
{
    setOpaque(true);

    titleLabel_.setText("PRESET BROWSER", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::neonPurple());
    addAndMakeVisible(titleLabel_);

    nowPlayingLabel_.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    nowPlayingLabel_.setColour(juce::Label::textColourId, SynthColors::neonYellow());
    nowPlayingLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(nowPlayingLabel_);

    searchBox_.setTextToShowWhenEmpty("Search presets...", SynthColors::textDim());
    searchBox_.setColour(juce::TextEditor::backgroundColourId, SynthColors::surface());
    searchBox_.setColour(juce::TextEditor::textColourId, SynthColors::text());
    searchBox_.addListener(this);
    searchBox_.addKeyListener(this);
    addAndMakeVisible(searchBox_);

    closeButton_.setButtonText("X");
    closeButton_.setColour(juce::TextButton::buttonColourId, SynthColors::danger());
    closeButton_.onClick = [this]() { setVisible(false); };
    addAndMakeVisible(closeButton_);

    savePresetButton_.setButtonText("Save Preset");
    savePresetButton_.setColour(juce::TextButton::buttonColourId, SynthColors::neonPurple());
    savePresetButton_.setColour(juce::TextButton::textColourOffId, SynthColors::text());
    savePresetButton_.onClick = [this]() {
        if (onSavePresetRequested)
            onSavePresetRequested();
    };
    addAndMakeVisible(savePresetButton_);

    importPresetsButton_.setButtonText("Import presets");
    importPresetsButton_.setColour(juce::TextButton::buttonColourId, SynthColors::surface());
    importPresetsButton_.setColour(juce::TextButton::textColourOffId, SynthColors::text());
    importPresetsButton_.setTooltip("Merge presets from a JSON file into your user presets");
    importPresetsButton_.onClick = [this]() {
        importChooser_ = std::make_unique<juce::FileChooser>(
            "Import presets", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.json");
        importChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file == juce::File())
                    return;
                int n = importPresetsFromFile(file);
                if (onPresetsImported)
                    onPresetsImported(n);
            });
    };
    addAndMakeVisible(importPresetsButton_);

    showUserPresetsButton_.setButtonText("User Presets");
    showUserPresetsButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::neonYellow());
    showUserPresetsButton_.setColour(juce::ToggleButton::tickDisabledColourId, SynthColors::textDim());
    showUserPresetsButton_.onClick = [this]() {
        showingUserPresets_ = showUserPresetsButton_.getToggleState();
        rebuildFilter();
    };
    addAndMakeVisible(showUserPresetsButton_);

    // Category filter — items derived from the actual preset library
    categoryFilter_.addItem("All Categories", 1);
    {
        juce::StringArray cats;
        for (int i = 0; i < kNumFullPresets; ++i)
            cats.addIfNotAlreadyThere(juce::String(kFullPresets[i].category));
        cats.sort(true);
        int id = 2;
        for (const auto& c : cats)
            categoryFilter_.addItem(prettifyCategory(c), id++);
        categoryFilter_.addItem("Custom", id);
    }
    categoryFilter_.setSelectedItemIndex(0, juce::dontSendNotification);
    categoryFilter_.onChange = [this]() {
        int idx = categoryFilter_.getSelectedItemIndex();
        currentCategory_ = (idx <= 0) ? "" : categoryFilter_.getItemText(idx);
        updateCategoryButtonStates();
        rebuildFilter();
    };
    addAndMakeVisible(categoryFilter_);

    static const char* quickLabels[kNumQuickCategories] = {
        "All", "Piano", "Organ", "Drums", "Bass", "Pads"
    };
    static const char* quickFilters[kNumQuickCategories] = {
        "", "piano", "organ", "drums", "bass", "pad"
    };
    for (int i = 0; i < kNumQuickCategories; ++i)
    {
        categoryButtons_[i].setButtonText(quickLabels[i]);
        categoryButtons_[i].getProperties().set("filter", quickFilters[i]);
        categoryButtons_[i].setColour(juce::TextButton::textColourOffId, SynthColors::text());
        categoryButtons_[i].onClick = [this, i]() {
            setCategoryFilter(categoryButtons_[i].getProperties()["filter"].toString());
        };
        addAndMakeVisible(categoryButtons_[i]);
    }
    updateCategoryButtonStates();

    presetList_.setModel(this);
    presetList_.setColour(juce::ListBox::backgroundColourId, SynthColors::surface());
    presetList_.setWantsKeyboardFocus(true);
    presetList_.addKeyListener(this);
    addAndMakeVisible(presetList_);

    rebuildFilter();
}

int PresetBrowser::importPresetsFromFile(const juce::File& file)
{
    UserPresetManager manager;
    int n = manager.importPresetsFromFile(file);
    if (n > 0)
    {
        showingUserPresets_ = true;
        showUserPresetsButton_.setToggleState(true, juce::dontSendNotification);
    }
    refreshUserPresets();
    return n;
}

void PresetBrowser::refreshUserPresets()
{
    UserPresetManager manager;
    userPresets_ = manager.getUserPresets();
    rebuildFilter();
}

void PresetBrowser::rebuildFilter()
{
    displayList_.clear();
    currentSearch_ = searchBox_.getText().toLowerCase();

    if (showingUserPresets_)
    {
        for (int i = 0; i < (int)userPresets_.size(); ++i)
        {
            const auto& preset = userPresets_[i];
            bool matchesSearch = currentSearch_.isEmpty() ||
                                 preset.name.toLowerCase().contains(currentSearch_);
            bool matchesCategory = categoryMatchesFilter(preset.category, currentCategory_);
            if (matchesSearch && matchesCategory)
            {
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                obj->setProperty("isUser", true);
                obj->setProperty("name", preset.name);
                obj->setProperty("category", preset.category);
                obj->setProperty("index", i);
                displayList_.emplace_back(obj);
            }
        }
    }
    else
    {
        filteredFactoryPresetIndices_.clear();
        for (int i = 0; i < kNumFullPresets; ++i) {
            const auto& preset = kFullPresets[i];
            bool matchesSearch = currentSearch_.isEmpty() ||
                                 juce::String(preset.name).toLowerCase().contains(currentSearch_) ||
                                 juce::String(preset.id).toLowerCase().contains(currentSearch_);
            bool matchesCategory = categoryMatchesFilter(juce::String(preset.category), currentCategory_);
            if (matchesSearch && matchesCategory)
            {
                filteredFactoryPresetIndices_.push_back(i);
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                obj->setProperty("isUser", false);
                obj->setProperty("name", juce::String(preset.name));
                obj->setProperty("category", juce::String(preset.category));
                obj->setProperty("index", (int)(filteredFactoryPresetIndices_.size() - 1));
                displayList_.emplace_back(obj);
            }
        }
    }

    presetList_.updateContent();
    presetList_.repaint();
}

void PresetBrowser::setCurrentPreset(int factoryIndex, const juce::String& name)
{
    currentFactoryIndex_ = factoryIndex;
    nowPlayingLabel_.setText("NOW PLAYING: " + name, juce::dontSendNotification);
    presetList_.repaint();
}

void PresetBrowser::setCategoryFilter(const juce::String& category)
{
    currentCategory_ = category;

    int comboIndex = -1;
    for (int i = 0; i < categoryFilter_.getNumItems(); ++i)
    {
        if (categoryFilter_.getItemText(i).equalsIgnoreCase(category))
        {
            comboIndex = i;
            break;
        }
    }
    // Group filters ("pad") have no single combo item — show the group name instead
    categoryFilter_.setTextWhenNothingSelected(prettifyCategory(category));
    categoryFilter_.setSelectedItemIndex(comboIndex, juce::dontSendNotification);
    updateCategoryButtonStates();
    rebuildFilter();
}

void PresetBrowser::cycleCategory(int delta)
{
    int idx = categoryFilter_.getSelectedItemIndex() + delta;
    int count = categoryFilter_.getNumItems();
    if (idx < 0) idx = count - 1;
    if (idx >= count) idx = 0;
    categoryFilter_.setSelectedItemIndex(idx, juce::sendNotificationSync);  // onChange updates filter + buttons
}

void PresetBrowser::updateCategoryButtonStates()
{
    for (int i = 0; i < kNumQuickCategories; ++i)
    {
        juce::String filter = categoryButtons_[i].getProperties()["filter"].toString();
        bool active = filter.equalsIgnoreCase(currentCategory_);
        categoryButtons_[i].setColour(juce::TextButton::buttonColourId,
                                      active ? SynthColors::neonPurple() : SynthColors::card());
        categoryButtons_[i].repaint();
    }
}

void PresetBrowser::moveSelection(int delta)
{
    if (displayList_.empty()) return;

    int row = presetList_.getSelectedRow() + delta;
    int count = (int)displayList_.size();
    if (row < 0) row = count - 1;
    if (row >= count) row = 0;

    presetList_.selectRow(row);  // selectedRowsChanged auditions the preset
    presetList_.scrollToEnsureRowIsOnscreen(row);
}

void PresetBrowser::loadRow(int row)
{
    if (row < 0)
        row = (displayList_.empty() ? -1 : 0);
    if (row < 0 || row >= (int)displayList_.size()) return;

    presetList_.selectRow(row);
    if (row == presetList_.getSelectedRow())
        selectedRowsChanged(row);  // re-selecting the same row still confirms the load
}

void PresetBrowser::closeBrowser()
{
    setVisible(false);
}

void PresetBrowser::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)displayList_.size()) return;

    auto* obj = displayList_[rowNumber].getDynamicObject();
    if (obj == nullptr) return;

    juce::String name = obj->getProperty("name");
    juce::String category = obj->getProperty("category");
    bool isUser = obj->getProperty("isUser");
    int index = obj->getProperty("index");

    bool isLoaded = !isUser && index >= 0 && index < (int)filteredFactoryPresetIndices_.size()
                    && filteredFactoryPresetIndices_[index] == currentFactoryIndex_;

    auto b = juce::Rectangle<int>(0, 0, width, height);

    if (rowIsSelected) {
        g.setColour(SynthColors::neonPurple().withAlpha(0.4f));
        g.fillRect(b);
    } else if (rowNumber % 2 == 0) {
        g.setColour(SynthColors::card().withAlpha(0.3f));
        g.fillRect(b);
    }

    if (isLoaded)
    {
        g.setColour(SynthColors::neonYellow().withAlpha(0.15f));
        g.fillRect(b);
        g.setColour(SynthColors::neonYellow());
        g.drawRect(b.reduced(1), 1);
    }

    if (isUser)
    {
        g.setColour(SynthColors::neonYellow());
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText("[USER]", b.reduced(8, 2).removeFromLeft(50), juce::Justification::centredLeft, true);
    }

    if (isLoaded)
    {
        g.setColour(SynthColors::neonYellow());
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(juce::String::fromUTF8("\xe2\x96\xb6"), b.reduced(4, 2).removeFromLeft(16),
                   juce::Justification::centred, true);
    }

    g.setColour(isLoaded ? SynthColors::neonYellow() : SynthColors::text());
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText(name, b.reduced(isUser ? 60 : (isLoaded ? 22 : 8), 2), juce::Justification::centredLeft, true);

    g.setColour(SynthColors::textDim());
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(category, b.reduced(8, 2), juce::Justification::centredRight, true);
}

void PresetBrowser::selectedRowsChanged(int lastRowSelected)
{
    if (suppressSelectionCallback_) return;

    if (lastRowSelected >= 0 && lastRowSelected < (int)displayList_.size()) {
        auto* obj = displayList_[lastRowSelected].getDynamicObject();
        if (obj == nullptr) return;

        bool isUser = obj->getProperty("isUser");
        if (isUser)
        {
            int userIndex = obj->getProperty("index");
            if (userIndex >= 0 && userIndex < (int)userPresets_.size() && onUserPresetSelected)
                onUserPresetSelected(userPresets_[userIndex]);
        }
        else
        {
            int factoryIndex = (int)obj->getProperty("index");
            if (factoryIndex >= 0 && factoryIndex < (int)filteredFactoryPresetIndices_.size()) {
                if (onPresetSelected)
                    onPresetSelected(filteredFactoryPresetIndices_[factoryIndex]);
            }
        }
    }
}

void PresetBrowser::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (row >= 0)
        closeBrowser();
}

bool PresetBrowser::handleKey(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::downKey)
    {
        moveSelection(1);
        return true;
    }
    if (key == juce::KeyPress::upKey)
    {
        moveSelection(-1);
        return true;
    }
    if (key == juce::KeyPress::returnKey)
    {
        loadRow(presetList_.getSelectedRow());
        closeBrowser();
        return true;
    }
    if (key == juce::KeyPress::escapeKey)
    {
        closeBrowser();
        return true;
    }
    if (key == juce::KeyPress::leftKey)
    {
        cycleCategory(-1);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        cycleCategory(1);
        return true;
    }

    return false;
}

bool PresetBrowser::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    // Keep caret movement when typing in the search field
    if (originatingComponent == &searchBox_ &&
        (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey))
        return false;

    return handleKey(key);
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(SynthColors::background().withAlpha(0.95f));

    g.setColour(SynthColors::gridLine());
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0, getHeight());
    for (int y = 0; y < getHeight(); y += 40)
        g.drawHorizontalLine(y, 0, getWidth());
}

void PresetBrowser::resized()
{
    auto b = getLocalBounds().reduced(20);
    auto titleRow = b.removeFromTop(30);
    titleLabel_.setBounds(titleRow.removeFromLeft(300));
    nowPlayingLabel_.setBounds(titleRow);
    closeButton_.setBounds(getWidth() - 50, 20, 30, 30);

    auto filterRow = b.removeFromTop(30);
    savePresetButton_.setBounds(filterRow.removeFromLeft(100));
    filterRow.removeFromLeft(8);
    importPresetsButton_.setBounds(filterRow.removeFromLeft(120));
    filterRow.removeFromLeft(8);
    showUserPresetsButton_.setBounds(filterRow.removeFromLeft(110));
    filterRow.removeFromLeft(8);
    categoryFilter_.setBounds(filterRow.removeFromLeft(180));
    filterRow.removeFromLeft(8);
    searchBox_.setBounds(filterRow);

    b.removeFromTop(6);
    auto categoryRow = b.removeFromTop(28);
    for (int i = 0; i < kNumQuickCategories; ++i)
    {
        if (i > 0) categoryRow.removeFromLeft(6);
        categoryButtons_[i].setBounds(categoryRow.removeFromLeft(84));
    }

    b.removeFromTop(8);
    presetList_.setBounds(b);
}

void PresetBrowser::setVisible(bool shouldBeVisible)
{
    juce::Component::setVisible(shouldBeVisible);
    if (shouldBeVisible) {
        searchBox_.grabKeyboardFocus();
        refreshUserPresets();
        rebuildFilter();

        if (currentFactoryIndex_ >= 0)
        {
            for (int row = 0; row < (int)filteredFactoryPresetIndices_.size(); ++row)
            {
                if (filteredFactoryPresetIndices_[row] == currentFactoryIndex_)
                {
                    suppressSelectionCallback_ = true;
                    presetList_.selectRow(row);
                    suppressSelectionCallback_ = false;
                    presetList_.scrollToEnsureRowIsOnscreen(row);
                    break;
                }
            }
        }
    }
}

// ── FavoritesBar ────────────────────────────────────────────────────────────

FavoritesBar::FavoritesBar()
{
    for (int i = 0; i < 8; ++i)
    {
        favButtons_[i].setButtonText(juce::String(i + 1));
        favButtons_[i].setColour(juce::TextButton::buttonColourId, SynthColors::card());
        favButtons_[i].setColour(juce::TextButton::textColourOffId, SynthColors::neonYellow());
        favButtons_[i].onClick = [this, i]() { buttonClicked(i); };
        addAndMakeVisible(favButtons_[i]);
    }
}

void FavoritesBar::paint(juce::Graphics& g)
{
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
}

void FavoritesBar::resized()
{
    auto b = getLocalBounds().reduced(4);
    int btnWidth = b.getWidth() / 8 - 4;
    for (int i = 0; i < 8; ++i)
    {
        favButtons_[i].setBounds(b.getX() + i * (btnWidth + 4), b.getY(), btnWidth, b.getHeight());
    }
}

void FavoritesBar::buttonClicked(int index)
{
    if (onPresetSelected)
        onPresetSelected(presetIndices_[index]);
}

void FavoritesBar::assignPresetIndex(int slot, int presetIndex)
{
    if (slot >= 0 && slot < 8)
    {
        presetIndices_[slot] = presetIndex;
        if (presetIndex >= 0 && presetIndex < kNumFullPresets)
        {
            favButtons_[slot].setButtonText(juce::String(slot + 1) + ": " + kFullPresets[presetIndex].name);
            favButtons_[slot].setColour(juce::TextButton::textColourOffId, SynthColors::neonYellow());
        }
        else
        {
            favButtons_[slot].setButtonText(juce::String(slot + 1));
            favButtons_[slot].setColour(juce::TextButton::textColourOffId, SynthColors::textDim());
        }
    }
}

int FavoritesBar::getPresetIndex(int slot) const
{
    if (slot >= 0 && slot < 8)
        return presetIndices_[slot];
    return -1;
}

// ── SplitKeyboardOverlay ────────────────────────────────────────────────────

SplitKeyboardOverlay::SplitKeyboardOverlay()
{
    setInterceptsMouseClicks(true, false);
}

void SplitKeyboardOverlay::paint(juce::Graphics& g)
{
    if (!isVisible()) return;

    auto b = getLocalBounds().toFloat();
    float splitX = b.getX() + (splitPoint_ - 21) * b.getWidth() / (108 - 21);

    // Split line
    g.setColour(SynthColors::neonYellow());
    g.drawVerticalLine((int)splitX, b.getY(), b.getBottom());

    // Zone labels
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.setColour(SynthColors::hotPink());
    g.drawText("LOWER", (int)b.getX() + 4, (int)b.getY() + 4, 60, 20, juce::Justification::left);
    g.setColour(SynthColors::cyan());
    g.drawText("UPPER", (int)splitX + 4, (int)b.getY() + 4, 60, 20, juce::Justification::left);

    // Note name at split
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int note = splitPoint_ % 12;
    int octave = splitPoint_ / 12 - 1;
    g.setColour(SynthColors::neonYellow());
    g.drawText(noteNames[note] + juce::String(octave), (int)splitX - 20, (int)b.getBottom() - 24, 40, 20, juce::Justification::centred);
}

void SplitKeyboardOverlay::resized() {}

void SplitKeyboardOverlay::setSplitPoint(int note)
{
    splitPoint_ = juce::jlimit(21, 108, note);
    repaint();
}

void SplitKeyboardOverlay::mouseDown(const juce::MouseEvent& e)
{
    dragging_ = true;
    setSplitPoint(noteAtX(e.getPosition().getX()));
    if (onSplitChanged)
        onSplitChanged(splitPoint_);
}

void SplitKeyboardOverlay::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging_)
    {
        setSplitPoint(noteAtX(e.getPosition().getX()));
        if (onSplitChanged)
            onSplitChanged(splitPoint_);
    }
}

void SplitKeyboardOverlay::mouseUp(const juce::MouseEvent&)
{
    dragging_ = false;
}

int SplitKeyboardOverlay::noteAtX(int x) const
{
    auto b = getLocalBounds().toFloat();
    float norm = juce::jlimit(0.0f, 1.0f, (float)(x - b.getX()) / b.getWidth());
    return 21 + (int)(norm * (108 - 21));
}

// ── PianoKeyboard ───────────────────────────────────────────────────────────

PianoKeyboard::PianoKeyboard(OpenSynthProcessor& processor)
    : processor_(processor)
{
}

void PianoKeyboard::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::surface());
    g.fillRect(b);

    // White keys
    for (int note = 21; note <= 108; ++note)
    {
        if (!isBlackKey(note))
        {
            auto keyRect = getWhiteKeyRect(note);
            bool pressed = pressedNote_ == note;
            bool isSplit = showSplit_ && note >= splitPoint_;

            g.setColour(pressed ? SynthColors::neonPurple() :
                        isSplit ? SynthColors::cyan().withAlpha(0.3f) :
                        juce::Colours::white);
            g.fillRect(keyRect);
            g.setColour(SynthColors::surface());
            g.drawRect(keyRect, 1.0f);
        }
    }

    // Black keys
    for (int note = 21; note <= 108; ++note)
    {
        if (isBlackKey(note))
        {
            auto keyRect = getBlackKeyRect(note);
            bool pressed = pressedNote_ == note;
            bool isSplit = showSplit_ && note >= splitPoint_;

            juce::Colour keyColour = pressed ? SynthColors::hotPink() :
                                     isSplit ? SynthColors::cyan().withAlpha(0.7f) :
                                     juce::Colour(0xFF1a1a1a);
            g.setColour(keyColour);
            g.fillRect(keyRect);

            // Subtle highlight on left edge for 3D depth
            g.setColour(keyColour.brighter(0.15f));
            g.drawVerticalLine((int)keyRect.getX(), keyRect.getY(), keyRect.getBottom());

            // Shadow on right edge
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.drawVerticalLine((int)keyRect.getRight() - 1, keyRect.getY(), keyRect.getBottom());

            // Bottom shadow
            g.drawHorizontalLine((int)keyRect.getBottom() - 1, keyRect.getX(), keyRect.getRight());
        }
    }

    // Split indicator line
    if (showSplit_)
    {
        auto splitRect = getWhiteKeyRect(splitPoint_);
        g.setColour(SynthColors::neonYellow());
        g.drawVerticalLine((int)splitRect.getX(), b.getY(), b.getBottom());
    }
}

void PianoKeyboard::resized() {}

void PianoKeyboard::mouseDown(const juce::MouseEvent& e)
{
    int note = getNoteAtPosition(e.getPosition());
    if (note >= 0)
    {
        pressedNote_ = note;
        processor_.injectMidiMessage(juce::MidiMessage::noteOn(1, note, 0.8f));
        repaint();
    }
}

void PianoKeyboard::mouseUp(const juce::MouseEvent&)
{
    if (pressedNote_ >= 0)
    {
        processor_.injectMidiMessage(juce::MidiMessage::noteOff(1, pressedNote_, 0.0f));
        pressedNote_ = -1;
        repaint();
    }
}

void PianoKeyboard::mouseDrag(const juce::MouseEvent& e)
{
    int note = getNoteAtPosition(e.getPosition());
    if (note >= 0 && note != pressedNote_)
    {
        if (pressedNote_ >= 0)
            processor_.injectMidiMessage(juce::MidiMessage::noteOff(1, pressedNote_, 0.0f));
        pressedNote_ = note;
        processor_.injectMidiMessage(juce::MidiMessage::noteOn(1, note, 0.8f));
        repaint();
    }
}

void PianoKeyboard::setSplitPoint(int note)
{
    splitPoint_ = juce::jlimit(21, 108, note);
    repaint();
}

void PianoKeyboard::setKeyPressed(int note)
{
    if (note >= 21 && note <= 108)
    {
        pressedNote_ = note;
        repaint();
    }
}

void PianoKeyboard::setKeyReleased(int note)
{
    if (pressedNote_ == note)
    {
        pressedNote_ = -1;
        repaint();
    }
}

void PianoKeyboard::setShowSplit(bool show)
{
    showSplit_ = show;
    repaint();
}

bool PianoKeyboard::isBlackKey(int note) const
{
    int n = note % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

juce::Rectangle<float> PianoKeyboard::getWhiteKeyRect(int note) const
{
    auto b = getLocalBounds().toFloat();
    int whiteKeyIndex = 0;
    for (int n = 21; n < note; ++n)
        if (!isBlackKey(n))
            whiteKeyIndex++;

    int totalWhiteKeys = 0;
    for (int n = 21; n <= 108; ++n)
        if (!isBlackKey(n))
            totalWhiteKeys++;

    float keyWidth = b.getWidth() / totalWhiteKeys;
    return { b.getX() + whiteKeyIndex * keyWidth, b.getY(), keyWidth - 1, b.getHeight() };
}

juce::Rectangle<float> PianoKeyboard::getBlackKeyRect(int note) const
{
    auto whiteRect = getWhiteKeyRect(note - 1);
    float blackWidth = whiteRect.getWidth() * 0.6f;
    float blackHeight = getHeight() * 0.6f;
    return { whiteRect.getRight() - blackWidth * 0.5f, (float)getY(), blackWidth, blackHeight };
}

int PianoKeyboard::getNoteAtPosition(juce::Point<int> pos) const
{
    for (int note = 108; note >= 21; --note)
    {
        if (isBlackKey(note))
        {
            if (getBlackKeyRect(note).toNearestInt().contains(pos))
                return note;
        }
    }
    for (int note = 21; note <= 108; ++note)
    {
        if (!isBlackKey(note))
        {
            if (getWhiteKeyRect(note).toNearestInt().contains(pos))
                return note;
        }
    }
    return -1;
}

} // namespace opensynth

// ── MidiLearnManager ────────────────────────────────────────────────────────

namespace opensynth {

void MidiLearnManager::startLearning(const juce::String& paramID)
{
    learningParam_ = paramID;
}

void MidiLearnManager::cancelLearning()
{
    learningParam_.clear();
}

bool MidiLearnManager::handleMidiCC(int ccNumber, float value, juce::AudioProcessorValueTreeState& apvts)
{
    if (learningParam_.isNotEmpty()) {
        auto it = ccToParam_.find(ccNumber);
        if (it != ccToParam_.end()) {
            paramToCC_.erase(it->second);
        }
        auto it2 = paramToCC_.find(learningParam_);
        if (it2 != paramToCC_.end()) {
            ccToParam_.erase(it2->second);
        }
        paramToCC_[learningParam_] = ccNumber;
        ccToParam_[ccNumber] = learningParam_;
        learningParam_.clear();
        return true;
    }

    auto it = ccToParam_.find(ccNumber);
    if (it != ccToParam_.end()) {
        auto* param = apvts.getParameter(it->second);
        if (param != nullptr) {
            param->setValueNotifyingHost(value / 127.0f);
        }
        return true;
    }
    return false;
}

std::vector<MidiLearnManager::Mapping> MidiLearnManager::getMappings() const
{
    std::vector<Mapping> result;
    result.reserve(paramToCC_.size());
    for (const auto& kv : paramToCC_) {
        result.push_back({kv.first, kv.second});
    }
    return result;
}

void MidiLearnManager::setMappings(const std::vector<Mapping>& mappings)
{
    paramToCC_.clear();
    ccToParam_.clear();
    for (const auto& m : mappings) {
        if (m.ccNumber >= 0 && m.ccNumber <= 127) {
            paramToCC_[m.paramID] = m.ccNumber;
            ccToParam_[m.ccNumber] = m.paramID;
        }
    }
}

int MidiLearnManager::getCCForParam(const juce::String& paramID) const
{
    auto it = paramToCC_.find(paramID);
    return (it != paramToCC_.end()) ? it->second : -1;
}

// ── MidiLearnableKnob ───────────────────────────────────────────────────────

MidiLearnableKnob::MidiLearnableKnob(const juce::String& name, juce::Colour accent,
                                     juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& paramID,
                                     MidiLearnManager& midiLearn)
    : SynthKnob(name, accent), apvts_(apvts), paramID_(paramID), midiLearn_(midiLearn)
{
}

void MidiLearnableKnob::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        showContextMenu();
        return;
    }
    SynthKnob::mouseDown(e);
}

void MidiLearnableKnob::showContextMenu()
{
    juce::PopupMenu menu;
    int currentCC = midiLearn_.getCCForParam(paramID_);

    if (currentCC >= 0) {
        menu.addItem(juce::PopupMenu::Item("Mapped to CC " + juce::String(currentCC)).setEnabled(false));
        menu.addSeparator();
        menu.addItem("Unmap MIDI", [this]() {
            midiLearn_.setMappings({});
            repaint();
        });
    } else {
        menu.addItem("Learn MIDI CC", [this]() {
            midiLearn_.startLearning(paramID_);
            repaint();
        });
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this));
}

void MidiLearnableKnob::paint(juce::Graphics& g)
{
    SynthKnob::paint(g);

    auto bounds = getLocalBounds().toFloat();
    float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 8.0f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = size * 0.5f;

    if (midiLearn_.isLearning() && midiLearn_.getLearningParam() == paramID_) {
        float pulse = 0.5f + 0.5f * std::sin(juce::Time::getMillisecondCounter() / 200.0f);
        g.setColour(SynthColors::neonYellow().withAlpha(pulse));
        g.drawEllipse(cx - radius - 3, cy - radius - 3, size + 6, size + 6, 2.0f);
    } else if (midiLearn_.getCCForParam(paramID_) >= 0) {
        g.setColour(SynthColors::neonYellow());
        g.fillEllipse(cx + radius - 6, cy - radius - 2, 8, 8);
    }
}

} // namespace opensynth

// ── OpenSynthEditor ────────────────────────────────────────────────────

namespace opensynth {

OpenSynthEditor::OpenSynthEditor(OpenSynthProcessor& proc)
    : AudioProcessorEditor(&proc), processor_(proc),
      osc1Panel_(proc.getParameters(), 1),
      osc2Panel_(proc.getParameters(), 2),
      filterPanel_(proc.getParameters()),
      ampEnvPanel_(proc.getParameters(), "AMP"),
      filterEnvPanel_(proc.getParameters(), "FILTER"),
      fx1Panel_(proc.getParameters(), 1),
      fx2Panel_(proc.getParameters(), 2),
      fx3Panel_(proc.getParameters(), 3),
      arpPanel_(proc.getParameters()),
      realismPanel_(proc.getParameters()),
      mpePanel_(proc.getParameters()),
      dbeamPanel_(proc.getParameters()),
      phraseSamplerPanel_(proc),
      samplePanel_(proc.getParameters(), proc),
      performancePanel_(proc.getParameters()),
      keyboard_(proc)
{
    setSize(1400, 900);
    setResizable(false, false);

    // ── Blackshield default LookAndFeel colour scheme ─────────────────
    auto& laf = getLookAndFeel();
    laf.setColour(juce::ResizableWindow::backgroundColourId, BlackshieldColours::iron());
    laf.setColour(juce::Label::textColourId, BlackshieldColours::bone());
    laf.setColour(juce::TextButton::buttonColourId, BlackshieldColours::steel());
    laf.setColour(juce::TextButton::buttonOnColourId, BlackshieldColours::blood());
    laf.setColour(juce::TextButton::textColourOffId, BlackshieldColours::bone());
    laf.setColour(juce::TextButton::textColourOnId, BlackshieldColours::boneBright());
    laf.setColour(juce::ComboBox::backgroundColourId, BlackshieldColours::steel());
    laf.setColour(juce::ComboBox::textColourId, BlackshieldColours::bone());
    laf.setColour(juce::ComboBox::outlineColourId, BlackshieldColours::ash());
    laf.setColour(juce::ComboBox::arrowColourId, BlackshieldColours::bone());
    laf.setColour(juce::PopupMenu::backgroundColourId, BlackshieldColours::steel());
    laf.setColour(juce::PopupMenu::textColourId, BlackshieldColours::bone());
    laf.setColour(juce::PopupMenu::highlightedBackgroundColourId, BlackshieldColours::blood());
    laf.setColour(juce::PopupMenu::highlightedTextColourId, BlackshieldColours::boneBright());
    laf.setColour(juce::TextEditor::backgroundColourId, BlackshieldColours::steel());
    laf.setColour(juce::TextEditor::textColourId, BlackshieldColours::bone());
    laf.setColour(juce::TextEditor::highlightColourId, BlackshieldColours::blood());
    laf.setColour(juce::TextEditor::highlightedTextColourId, BlackshieldColours::boneBright());
    laf.setColour(juce::TextEditor::outlineColourId, BlackshieldColours::ash());
    laf.setColour(juce::Slider::textBoxTextColourId, BlackshieldColours::bone());
    laf.setColour(juce::Slider::textBoxBackgroundColourId, BlackshieldColours::steel());
    laf.setColour(juce::Slider::textBoxOutlineColourId, BlackshieldColours::ash());
    laf.setColour(juce::ScrollBar::thumbColourId, BlackshieldColours::steelLight());
    laf.setColour(juce::AlertWindow::backgroundColourId, BlackshieldColours::iron());
    laf.setColour(juce::AlertWindow::textColourId, BlackshieldColours::bone());
    laf.setColour(juce::TooltipWindow::backgroundColourId, BlackshieldColours::steel());
    laf.setColour(juce::TooltipWindow::textColourId, BlackshieldColours::bone());

    // Title
    titleLabel_.setText("Open Synth", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::neonPurple());
    addAndMakeVisible(titleLabel_);

    // Header buttons
    presetButton_.setButtonText("Presets");
    presetButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    presetButton_.setColour(juce::TextButton::textColourOffId, SynthColors::hotPink());
    presetButton_.onClick = [this]() { showPresetBrowser(); };
    addAndMakeVisible(presetButton_);

    setlistButton_.setButtonText("Setlist");
    setlistButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    setlistButton_.setColour(juce::TextButton::textColourOffId, SynthColors::cyan());
    setlistButton_.onClick = [this]() { showSetlistMode(); };
    addAndMakeVisible(setlistButton_);

    undoButton_.setButtonText("Undo");
    undoButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    undoButton_.setColour(juce::TextButton::textColourOffId, SynthColors::textDim());
    undoButton_.onClick = [this]() {
        processor_.getUndoManager().undo();
        showUndoFeedback("Undo");
    };
    addAndMakeVisible(undoButton_);

    redoButton_.setButtonText("Redo");
    redoButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    redoButton_.setColour(juce::TextButton::textColourOffId, SynthColors::textDim());
    redoButton_.onClick = [this]() {
        processor_.getUndoManager().redo();
        showUndoFeedback("Redo");
    };
    addAndMakeVisible(redoButton_);

    // Undo feedback label
    undoFeedbackLabel_.setText("", juce::dontSendNotification);
    undoFeedbackLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    undoFeedbackLabel_.setColour(juce::Label::textColourId, SynthColors::neonYellow());
    undoFeedbackLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(undoFeedbackLabel_);

    // Setlist overlay
    setlistOverlay_.onSlotSelected = [this](int slotIndex) {
        juce::String name = setlistOverlay_.getSlotPresetName(slotIndex);
        if (name.isNotEmpty()) {
            // Find preset by name in full library
            for (int i = 0; i < kNumFullPresets; ++i) {
                if (juce::String(kFullPresets[i].name) == name) {
                    loadPresetByIndex(i);
                    setlistOverlay_.setVisible(false);
                    break;
                }
            }
        }
    };
    setlistOverlay_.onSlotAssigned = [this](int slotIndex) {
        juce::String currentName = titleLabel_.getText();
        setlistOverlay_.assignPresetToSlot(slotIndex, currentName);
    };
    addChildComponent(setlistOverlay_);

    // Favorites bar
    favorites_.onPresetSelected = [this](int idx) {
        if (idx >= 0 && idx < kNumFullPresets)
            loadPresetByIndex(idx);
    };
    addAndMakeVisible(favorites_);

    // Preset browser callback
    presetBrowser_.onPresetSelected = [this](int presetIndex) {
        if (presetIndex >= 0 && presetIndex < kNumFullPresets) {
            loadPresetByIndex(presetIndex);
            presetBrowser_.setVisible(false);
        }
    };
    presetBrowser_.onSavePresetRequested = [this]() {
        saveCurrentPreset();
    };
    presetBrowser_.onPresetsImported = [this](int n) {
        showUndoFeedback(n > 0 ? "Imported " + juce::String(n) + (n == 1 ? " preset" : " presets")
                               : juce::String("Import failed"));
    };
    presetBrowser_.onUserPresetSelected = [this](const UserPreset& preset) {
        userPresetManager_.loadPreset(preset, processor_.getParameters());
        titleLabel_.setText(preset.name, juce::dontSendNotification);
        presetBrowser_.setCurrentPreset(-1, preset.name);
        presetBrowser_.setVisible(false);
    };

    // Meters + Scope
    addAndMakeVisible(meters_);
    addAndMakeVisible(scope_);

    // Panels
    addAndMakeVisible(osc1Panel_);
    addAndMakeVisible(osc2Panel_);
    addAndMakeVisible(filterPanel_);
    addAndMakeVisible(ampEnvPanel_);
    addAndMakeVisible(filterEnvPanel_);
    addAndMakeVisible(fx1Panel_);
    addAndMakeVisible(fx2Panel_);
    addAndMakeVisible(fx3Panel_);
    addAndMakeVisible(arpPanel_);
    addAndMakeVisible(realismPanel_);
    addAndMakeVisible(mpePanel_);
    addAndMakeVisible(samplePanel_);
    addAndMakeVisible(dbeamPanel_);
    addAndMakeVisible(phraseSamplerPanel_);
    addAndMakeVisible(performancePanel_);
    addAndMakeVisible(keyboard_);
    addAndMakeVisible(waveformDisplay_);
    proc.setWaveformDisplay(&waveformDisplay_);

    // Overlays (initially hidden)
    addChildComponent(presetBrowser_);
    addChildComponent(splitOverlay_);

    splitOverlay_.onSplitChanged = [this](int note)
    {
        keyboard_.setSplitPoint(note);
        // Update performance panel slider to match
        if (auto* param = processor_.getParameters().getParameter("perfSplitPoint"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost((note - 21) / 87.0f);
            param->endChangeGesture();
        }
    };

    performancePanel_.onSplitChanged = [this](int note)
    {
        keyboard_.setSplitPoint(note);
        splitOverlay_.setSplitPoint(note);
    };

    performancePanel_.onLayerChanged = [this](bool enabled)
    {
        juce::ignoreUnused(enabled);
        // Layer on/off could toggle osc2 volume or enable split mode
    };

    dbeamPanel_.onValueChanged = [this](float value)
    {
        if (auto* param = processor_.getParameters().getParameter("dbeamValue"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(value);
            param->endChangeGesture();
        }

        // Also route to target parameter based on dbeamTarget
        int target = static_cast<int>(*processor_.getParameters().getRawParameterValue("dbeamTarget"));
        switch (target)
        {
        case 0: // Filter cutoff
            if (auto* p = processor_.getParameters().getParameter("filterCutoff"))
            {
                float min = 20.0f, max = 20000.0f;
                p->beginChangeGesture();
                p->setValueNotifyingHost((min + value * (max - min) - 20.0f) / 19980.0f);
                p->endChangeGesture();
            }
            break;
        case 1: // Pitch bend
            // Pitch bend is handled per-note, store in a mod source
            break;
        case 2: // FX depth
            if (auto* p = processor_.getParameters().getParameter("fx1Param3"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(value);
                p->endChangeGesture();
            }
            break;
        case 3: // Master volume
            if (auto* p = processor_.getParameters().getParameter("masterVolume"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(value);
                p->endChangeGesture();
            }
            break;
        }
    };

    // Start meter timer
    startTimerHz(10);

    // Load persisted app state (setlist, favorites, last preset)
    loadAppState();

    // Open default MIDI input device
    openDefaultMidiInput();
}

void OpenSynthEditor::loadAppState()
{
    // Load setlist
    auto setlist = appStateManager_.loadSetlist();
    for (int i = 0; i < SetlistState::kNumSlots; ++i)
        setlistOverlay_.assignPresetToSlot(i, setlist.slotPresetNames[i]);

    // Load favorites
    auto favorites = appStateManager_.loadFavorites();
    for (int i = 0; i < FavoritesState::kNumSlots; ++i)
    {
        int idx = favorites.presetIndices[i];
        if (idx >= 0 && idx < kNumFullPresets)
            favorites_.assignPresetIndex(i, idx);
    }

    // Load last preset
    auto lastID = appStateManager_.loadLastPresetID();
    if (lastID.isNotEmpty())
        loadPresetByID(lastID);
    else
        loadPresetByIndex(0);

    // Refresh user presets in browser
    presetBrowser_.refreshUserPresets();
}

void OpenSynthEditor::saveAppState()
{
    // Save setlist
    SetlistState setlist;
    for (int i = 0; i < SetlistState::kNumSlots; ++i)
        setlist.slotPresetNames[i] = setlistOverlay_.getSlotPresetName(i);
    appStateManager_.saveSetlist(setlist);

    // Save favorites
    FavoritesState favorites;
    for (int i = 0; i < FavoritesState::kNumSlots; ++i)
        favorites.presetIndices[i] = favorites_.getPresetIndex(i);
    appStateManager_.saveFavorites(favorites);

    // Save last preset ID
    if (currentPresetIndex_ >= 0 && currentPresetIndex_ < kNumFullPresets)
        appStateManager_.saveLastPresetID(kFullPresets[currentPresetIndex_].id);
}

void OpenSynthEditor::handleMidiCC(int ccNumber, float value)
{
    midiLearnManager_.handleMidiCC(ccNumber, value, processor_.getParameters());
}

void OpenSynthEditor::openDefaultMidiInput()
{
    auto devices = juce::MidiInput::getAvailableDevices();
    if (devices.isEmpty()) return;

    // Prefer the first non-virtual device, or just the first one
    for (const auto& device : devices)
    {
        midiInput_ = juce::MidiInput::openDevice(device.identifier, this);
        if (midiInput_ != nullptr)
        {
            midiInput_->start();
            break;
        }
    }
}

void OpenSynthEditor::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);

    if (message.isNoteOn())
    {
        int note = message.getNoteNumber();
        processor_.injectMidiMessage(juce::MidiMessage::noteOn(1, note, message.getFloatVelocity()));
        keyboard_.setKeyPressed(note);
    }
    else if (message.isNoteOff())
    {
        int note = message.getNoteNumber();
        processor_.injectMidiMessage(juce::MidiMessage::noteOff(1, note, 0.0f));
        keyboard_.setKeyReleased(note);
    }
    else if (message.isController())
    {
        handleMidiCC(message.getControllerNumber(), message.getControllerValue() / 127.0f);
    }
    else if (message.isPitchWheel())
    {
        processor_.injectMidiMessage(message);
    }
    else if (message.isSustainPedalOn() || message.isSustainPedalOff())
    {
        processor_.injectMidiMessage(message);
    }
}

void OpenSynthEditor::timerCallback()
{
    int voices = processor_.getSynth().getActiveVoiceCount();
    float cpu = processor_.getSynth().getCpuLoad() * 100.0f;
    meters_.setVoiceCount(voices);
    meters_.setCpuLoad(cpu);

    // Push audio buffer to scope
    auto buffer = processor_.getSynth().getLastAudioBuffer();
    if (!buffer.empty())
        scope_.pushBuffer(buffer, 2);

    // Update undo/redo button states
    undoButton_.setEnabled(processor_.getUndoManager().canUndo());
    redoButton_.setEnabled(processor_.getUndoManager().canRedo());
    undoButton_.setColour(juce::TextButton::textColourOffId,
        processor_.getUndoManager().canUndo() ? SynthColors::text() : SynthColors::textDim());
    redoButton_.setColour(juce::TextButton::textColourOffId,
        processor_.getUndoManager().canRedo() ? SynthColors::text() : SynthColors::textDim());

    // Fade out undo feedback
    if (undoFeedbackCounter_ > 0)
    {
        undoFeedbackCounter_--;
        if (undoFeedbackCounter_ <= 0)
            undoFeedbackLabel_.setText("", juce::dontSendNotification);
    }
}

void OpenSynthEditor::showPresetBrowser()
{
    presetBrowser_.setVisible(true);
    presetBrowser_.setBounds(getLocalBounds());
    presetBrowser_.toFront(true);
}

void OpenSynthEditor::showSetlistMode()
{
    setlistOverlay_.setVisible(true);
    setlistOverlay_.setBounds(getLocalBounds());
    setlistOverlay_.toFront(true);
}

void OpenSynthEditor::loadFavoritePreset(int index)
{
    int presetIdx = favorites_.getPresetIndex(index);
    if (presetIdx >= 0 && presetIdx < kNumFullPresets)
        loadPresetByIndex(presetIdx);
}

void OpenSynthEditor::loadPresetByIndex(int index)
{
    if (index >= 0 && index < kNumFullPresets) {
        currentPresetIndex_ = index;
        loadPresetByID(kFullPresets[index].id);
    }
}

void OpenSynthEditor::loadPresetByID(const juce::String& id)
{
    // Find preset in full library
    for (int i = 0; i < kNumFullPresets; ++i) {
        if (juce::String(kFullPresets[i].id) == id) {
            const auto& p = kFullPresets[i];
            currentPresetIndex_ = i;
            // Update title
            titleLabel_.setText(p.name, juce::dontSendNotification);
            presetBrowser_.setCurrentPreset(i, juce::String(p.name));
            // Push to APVTS (UI updates automatically via attachments)
            applyPresetToAPVTS(p, processor_.getParameters());
            // Push to engine (immediate audio update)
            applyPresetToEngine(p, processor_.getSynth());
            // Configure sample player for acoustic presets
            configureSamplePlayerForPreset(p);
            break;
        }
    }
}

void OpenSynthEditor::configureSamplePlayerForPreset(const PresetData& p)
{
    auto& synth = processor_.getSynth();
    auto* engine = synth.getEngine();
    if (!engine) return;

    // If sampleMix > 0, we need a sample player
    if (p.sampleMix > 0.0f) {
        if (!engine->getSamplePlayer()) {
            auto sp = std::make_unique<SamplePlayer>();
            sp->prepare(engine->getSampleRate());
            engine->setSamplePlayer(std::move(sp));
        }
        engine->getSamplePlayer()->setMixLevel(p.sampleMix);
        // Try to load a matching sample from the factory library
        loadSampleForPreset(p, *engine->getSamplePlayer());
    } else {
        // Disable sample player for pure synth presets
        if (engine->getSamplePlayer()) {
            engine->getSamplePlayer()->setMixLevel(0.0f);
            engine->getSamplePlayer()->clear();
        }
    }
}

void OpenSynthEditor::loadSampleForPreset(const PresetData& p, SamplePlayer& player)
{
    // Use the preset's sampleManifestId if available
    if (p.sampleManifestId[0] != '\0') {
        juce::File manifestFile = getSampleManifestPath(p.sampleManifestId);
        if (manifestFile.existsAsFile()) {
            player.clear();
            if (player.loadMultiSample(manifestFile.getFullPathName().toStdString())) {
                return;
            }
        }
    }

    // Fallback: try legacy path samples/<category>/<name>.wav
    juce::File sampleDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                               .getParentDirectory()
                               .getChildFile("samples")
                               .getChildFile(p.category);
    juce::String safeName = juce::String(p.name).replaceCharacter(' ', '_');
    juce::File sampleFile = sampleDir.getChildFile(safeName + ".wav");

    if (sampleFile.existsAsFile()) {
        player.clear();
        player.loadSample(sampleFile.getFullPathName().toStdString(), 60, 0, 127);
    }
}

void OpenSynthEditor::saveCurrentPreset()
{
    auto* alert = new juce::AlertWindow("Save Preset", "Enter a name for your preset:", juce::AlertWindow::QuestionIcon, this);
    alert->addTextEditor("name", titleLabel_.getText(), "Preset Name");
    alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result)
    {
        if (result == 1)
        {
            juce::String name = alert->getTextEditorContents("name").trim();
            if (name.isNotEmpty())
            {
                if (userPresetManager_.savePreset(name, "Custom", {}, processor_.getParameters()))
                {
                    titleLabel_.setText(name, juce::dontSendNotification);
                    presetBrowser_.refreshUserPresets();
                    showUndoFeedback("Preset Saved");
                }
            }
        }
        delete alert;
    }), true);
}

void OpenSynthEditor::showUndoFeedback(const juce::String& text)
{
    undoFeedbackLabel_.setText(text, juce::dontSendNotification);
    undoFeedbackCounter_ = 30; // ~3 seconds at 10Hz timer
}

void OpenSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(SynthColors::background());

    // Animated grid lines with subtle pulse
    float alpha = 0.08f + 0.04f * std::sin(juce::Time::getMillisecondCounter() / 2000.0f);
    g.setColour(SynthColors::neonPurple().withAlpha(alpha));
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0, getHeight());
    for (int y = 0; y < getHeight(); y += 40)
        g.drawHorizontalLine(y, 0, getWidth());

    // Horizon line (synthwave sunset effect)
    float horizonY = getHeight() * 0.65f;
    g.setColour(SynthColors::hotPink().withAlpha(0.05f));
    g.drawHorizontalLine((int)horizonY, 0, (float)getWidth());
    g.setColour(SynthColors::hotPink().withAlpha(0.03f));
    g.drawHorizontalLine((int)horizonY - 1, 0, (float)getWidth());
    g.drawHorizontalLine((int)horizonY + 1, 0, (float)getWidth());
}

void OpenSynthEditor::resized()
{
    auto b = getLocalBounds().reduced(12);

    // Header row
    auto header = b.removeFromTop(40);
    titleLabel_.setBounds(header.removeFromLeft(220));
    favorites_.setBounds(header.removeFromLeft(320));
    meters_.setBounds(header.removeFromLeft(180));
    scope_.setBounds(header.removeFromLeft(120));
    undoButton_.setBounds(header.removeFromRight(60));
    redoButton_.setBounds(header.removeFromRight(60));
    presetButton_.setBounds(header.removeFromRight(90));
    setlistButton_.setBounds(header.removeFromRight(90));

    // Undo feedback overlay (centered)
    undoFeedbackLabel_.setBounds(getLocalBounds().reduced(200, 300));

    b.removeFromTop(8);

    // Calculate responsive panel sizes
    int availWidth = b.getWidth();
    int availHeight = b.getHeight();
    int gap = juce::jmin(12, availWidth / 60);
    int panelWidth = (availWidth - gap * 3) / 4;
    int topHeight = juce::jmin(220, availHeight / 3);
    int midHeight = juce::jmin(220, availHeight / 3);

    // Top row: Oscillators + Filter + Arp
    auto topRow = b.removeFromTop(topHeight);
    osc1Panel_.setBounds(topRow.removeFromLeft(panelWidth));
    topRow.removeFromLeft(gap);
    osc2Panel_.setBounds(topRow.removeFromLeft(panelWidth));
    topRow.removeFromLeft(gap);
    filterPanel_.setBounds(topRow.removeFromLeft(panelWidth));
    topRow.removeFromLeft(gap);
    arpPanel_.setBounds(topRow.removeFromLeft(panelWidth));

    b.removeFromTop(gap);

    // Middle row: Envelopes + FX slots + Realism + MPE + D-Beam + Performance
    auto midRow = b.removeFromTop(midHeight);
    int midPanelWidth = (availWidth - gap * 7) / 8;
    ampEnvPanel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    filterEnvPanel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    fx1Panel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    fx2Panel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    fx3Panel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    realismPanel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    mpePanel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    dbeamPanel_.setBounds(midRow.removeFromLeft(midPanelWidth));
    midRow.removeFromLeft(gap);
    performancePanel_.setBounds(midRow);

    b.removeFromTop(gap);

    // Bottom: Sample panel + Phrase sampler + Waveform + Keyboard
    int samplerHeight = juce::jmin(160, b.getHeight() / 3);
    auto samplerRow = b.removeFromTop(samplerHeight);
    samplePanel_.setBounds(samplerRow.removeFromLeft(juce::jmin(300, availWidth / 4)));
    samplerRow.removeFromLeft(gap);
    phraseSamplerPanel_.setBounds(samplerRow.removeFromLeft(juce::jmin(300, availWidth / 4)));
    samplerRow.removeFromLeft(gap);
    waveformDisplay_.setBounds(samplerRow.removeFromLeft(juce::jmin(200, availWidth / 6)));
    samplerRow.removeFromLeft(gap);
    keyboard_.setBounds(samplerRow);

    // Overlays fill entire editor
    presetBrowser_.setBounds(getLocalBounds());
    splitOverlay_.setBounds(keyboard_.getBounds());
    setlistOverlay_.setBounds(getLocalBounds());
}

// ── D-Beam Panel ────────────────────────────────────────────────────────────

DBeamPanel::DBeamPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    targetSelector_.addItemList({"Filter Cutoff", "Pitch Bend", "FX Depth", "Master Volume"}, 1);
    addAndMakeVisible(targetSelector_);
    targetAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, "dbeamTarget", targetSelector_);
}

void DBeamPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(SynthColors::neonYellow());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("D-BEAM", 12, 8, 120, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);

    // Draw horizontal bar representing current value
    int barY = 60;
    int barHeight = 20;
    int barX = 12;
    int barWidth = getWidth() - 24;

    g.setColour(SynthColors::surface());
    g.fillRoundedRectangle((float)barX, (float)barY, (float)barWidth, (float)barHeight, 4.0f);

    // Filled portion
    int fillWidth = static_cast<int>(barWidth * currentValue_);
    juce::Colour beamColor = SynthColors::neonYellow().withAlpha(0.6f + 0.4f * currentValue_);
    g.setColour(beamColor);
    g.fillRoundedRectangle((float)barX, (float)barY, (float)fillWidth, (float)barHeight, 4.0f);

    // Glow line at the value position
    g.setColour(SynthColors::neonYellow().brighter());
    g.drawVerticalLine(barX + fillWidth, barY, barY + barHeight);

    // Value text
    g.setColour(SynthColors::text());
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(juce::String(static_cast<int>(currentValue_ * 100)) + "%",
               barX, barY + barHeight + 4, barWidth, 16, juce::Justification::centred);

    // Instruction
    g.setColour(SynthColors::textDim());
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("Click & drag vertically to control",
               barX, barY + barHeight + 20, barWidth, 16, juce::Justification::centred);
}

void DBeamPanel::resized()
{
    targetSelector_.setBounds(12, 36, getWidth() - 24, 22);
}

void DBeamPanel::mouseDown(const juce::MouseEvent& e)
{
    dragging_ = true;
    updateValueFromY(e.getPosition().getY());
}

void DBeamPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging_)
        updateValueFromY(e.getPosition().getY());
}

void DBeamPanel::mouseUp(const juce::MouseEvent&)
{
    dragging_ = false;
}

void DBeamPanel::updateValueFromY(int y)
{
    float norm = 1.0f - juce::jlimit(0.0f, 1.0f, (float)(y - 50) / (getHeight() - 70));
    currentValue_ = norm;
    if (onValueChanged)
        onValueChanged(currentValue_);
    repaint();
}

// ── Phrase Sampler Panel ────────────────────────────────────────────────────

PhraseSamplerPanel::PhraseSamplerPanel(OpenSynthProcessor& processor)
    : processor_(processor)
{
    setOpaque(false);

    loadButton_.setButtonText("Load");
    loadButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    loadButton_.setColour(juce::TextButton::textColourOffId, SynthColors::neonYellow());
    loadButton_.onClick = [this]() { loadFile(); };
    addAndMakeVisible(loadButton_);

    playButton_.setButtonText("Play");
    playButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    playButton_.setColour(juce::TextButton::textColourOffId, SynthColors::cyan());
    playButton_.onClick = [this]() { play(); };
    addAndMakeVisible(playButton_);

    stopButton_.setButtonText("Stop");
    stopButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    stopButton_.setColour(juce::TextButton::textColourOffId, SynthColors::danger());
    stopButton_.onClick = [this]() { stop(); };
    addAndMakeVisible(stopButton_);

    loopButton_.setButtonText("Loop");
    loopButton_.setColour(juce::ToggleButton::textColourId, SynthColors::textDim());
    loopButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::hotPink());
    loopButton_.onClick = [this]() {
        processor_.phraseSample.looping.store(loopButton_.getToggleState(),
                                               std::memory_order_release);
    };
    addAndMakeVisible(loopButton_);

    fileLabel_.setText("No file loaded", juce::dontSendNotification);
    fileLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    fileLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(fileLabel_);

    posLabel_.setText("", juce::dontSendNotification);
    posLabel_.setColour(juce::Label::textColourId, SynthColors::cyan());
    posLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    addAndMakeVisible(posLabel_);

    volumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider_.setRange(0.0, 1.0, 0.01);
    volumeSlider_.setValue(0.8);
    volumeSlider_.onValueChange = [this]() {
        processor_.phraseSample.volume = static_cast<float>(volumeSlider_.getValue());
    };
    addAndMakeVisible(volumeSlider_);

    formatManager_.registerBasicFormats();

    startTimerHz(15);
}

PhraseSamplerPanel::~PhraseSamplerPanel() = default;

void PhraseSamplerPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    float w = b.getWidth();

    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(SynthColors::hotPink());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("PHRASE SAMPLER", 12, 8, 150, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, w - 8);

    // Waveform area
    float waveY = 92.0f;
    float waveH = 42.0f;
    g.setColour(SynthColors::surface());
    g.fillRoundedRectangle(12.0f, waveY, w - 24.0f, waveH, 4.0f);

    // Draw real waveform thumbnail
    if (!thumbnail_.empty())
    {
        float cy = waveY + waveH * 0.5f;
        float scale = waveH * 0.45f;
        int numSamples = static_cast<int>(thumbnail_.size());

        // Center line
        g.setColour(SynthColors::gridLine().withAlpha(0.3f));
        g.drawHorizontalLine(static_cast<int>(cy), 12.0f, w - 12.0f);

        // Waveform
        g.setColour(SynthColors::hotPink());
        juce::Path path;
        float xStep = (w - 24.0f) / static_cast<float>(numSamples - 1);
        path.startNewSubPath(12.0f, cy - thumbnail_[0] * scale);
        for (int i = 1; i < numSamples; ++i)
            path.lineTo(12.0f + i * xStep, cy - thumbnail_[i] * scale);
        g.strokePath(path, juce::PathStrokeType(1.5f));

        // Playhead position
        if (processor_.phraseSample.playing.load(std::memory_order_acquire))
        {
            int pos = processor_.phraseSample.playPosition.load(std::memory_order_relaxed);
            int total = processor_.phraseSample.getNumSamples();
            if (total > 0)
            {
                float px = 12.0f + (w - 24.0f) * static_cast<float>(pos) / static_cast<float>(total);
                g.setColour(SynthColors::neonYellow());
                g.drawVerticalLine(static_cast<int>(px), waveY, waveY + waveH);
            }
        }
    }
    else
    {
        // Empty state — animated placeholder dots
        g.setColour(SynthColors::neonPurple().withAlpha(0.3f));
        float cy = waveY + waveH * 0.5f;
        for (int i = 0; i < static_cast<int>(w) - 24; i += 4)
        {
            float y = cy + std::sin(i * 0.1f + juce::Time::getMillisecondCounter() / 500.0f) * 8.0f;
            g.fillEllipse(12.0f + i, y, 2.0f, 2.0f);
        }
    }
}

void PhraseSamplerPanel::resized()
{
    int y = 38;
    loadButton_.setBounds(12, y, 52, 22);
    playButton_.setBounds(68, y, 44, 22);
    stopButton_.setBounds(116, y, 44, 22);
    loopButton_.setBounds(164, y, 50, 22);
    y += 26;
    fileLabel_.setBounds(12, y, getWidth() - 24, 16);
    y += 18;
    posLabel_.setBounds(12, y, getWidth() - 24, 14);
    y += 16;
    // Waveform area is drawn in paint() at fixed y
    volumeSlider_.setBounds(12, 140, getWidth() - 24, 18);
}

void PhraseSamplerPanel::timerCallback()
{
    // Update playhead position label
    if (processor_.phraseSample.playing.load(std::memory_order_acquire))
    {
        int pos = processor_.phraseSample.playPosition.load(std::memory_order_relaxed);
        int total = processor_.phraseSample.getNumSamples();
        double sr = processor_.phraseSample.sampleRate;
        if (total > 0 && sr > 0)
        {
            double posSec = static_cast<double>(pos) / sr;
            double totalSec = static_cast<double>(total) / sr;
            auto fmtTime = [](double s) -> juce::String {
                int min = static_cast<int>(s) / 60;
                double sec = s - min * 60;
                return juce::String::formatted("%d:%05.2f", min, sec);
            };
            posLabel_.setText(fmtTime(posSec) + " / " + fmtTime(totalSec),
                              juce::dontSendNotification);
        }
        repaint();
    }
    else if (!posLabel_.getText().isEmpty())
    {
        posLabel_.setText("", juce::dontSendNotification);
    }
}

void PhraseSamplerPanel::loadFile()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select an audio file...",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.flac;*.ogg");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File()) return;

        auto* reader = formatManager_.createReaderFor(file);
        if (reader == nullptr)
        {
            fileLabel_.setText("Failed to load", juce::dontSendNotification);
            fileLabel_.setColour(juce::Label::textColourId, SynthColors::danger());
            return;
        }

        // Read entire file into the processor's phrase buffer
        int numSamples = static_cast<int>(reader->lengthInSamples);
        int numChannels = reader->numChannels;
        double sampleRate = reader->sampleRate;

        auto& buf = processor_.phraseSample.buffer;
        buf.setSize(numChannels, numSamples);
        reader->read(&buf, 0, numSamples, 0, true, true);
        delete reader;

        processor_.phraseSample.sampleRate = sampleRate;
        processor_.phraseSample.stop();

        currentFile_ = file;
        fileLabel_.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
        fileLabel_.setColour(juce::Label::textColourId, SynthColors::text());

        // Build waveform thumbnail (downsample for display)
        const int kThumbnailSize = 256;
        thumbnail_.resize(kThumbnailSize);
        thumbnailSamples_ = numSamples;
        const float* ch0 = buf.getReadPointer(0);
        int samplesPerBucket = numSamples / kThumbnailSize;
        if (samplesPerBucket < 1) samplesPerBucket = 1;
        for (int i = 0; i < kThumbnailSize; ++i)
        {
            float maxVal = 0.0f;
            int start = i * samplesPerBucket;
            int end = std::min(start + samplesPerBucket, numSamples);
            for (int s = start; s < end; ++s)
                maxVal = std::max(maxVal, std::abs(ch0[s]));
            thumbnail_[i] = maxVal;
        }

        repaint();
    });
}

void PhraseSamplerPanel::play()
{
    if (processor_.phraseSample.getNumSamples() > 0)
    {
        processor_.phraseSample.start();
        fileLabel_.setText(currentFile_.getFileNameWithoutExtension() + " (playing)",
                           juce::dontSendNotification);
    }
}

void PhraseSamplerPanel::stop()
{
    processor_.phraseSample.stop();
    if (currentFile_.existsAsFile())
    {
        fileLabel_.setText(currentFile_.getFileNameWithoutExtension(),
                           juce::dontSendNotification);
    }
    posLabel_.setText("", juce::dontSendNotification);
    repaint();
}

// ── Sample Panel ────────────────────────────────────────────────────────────

SamplePanel::SamplePanel(juce::AudioProcessorValueTreeState& apvts, OpenSynthProcessor& processor)
    : apvts_(apvts), processor_(processor),
      mixKnob_("Mix", SynthColors::hotPink()),
      attackKnob_("Atk", SynthColors::cyan()),
      decayKnob_("Dec", SynthColors::cyan()),
      sustainKnob_("Sus", SynthColors::cyan()),
      releaseKnob_("Rel", SynthColors::cyan())
{
    setOpaque(false);

    titleLabel_.setText("SAMPLE PLAYER", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::hotPink());
    addAndMakeVisible(titleLabel_);

    sampleNameLabel_.setText("No sample loaded", juce::dontSendNotification);
    sampleNameLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    addAndMakeVisible(sampleNameLabel_);

    categoryLabel_.setText("", juce::dontSendNotification);
    categoryLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    categoryLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    addAndMakeVisible(categoryLabel_);

    zoneCountLabel_.setText("Zones: 0", juce::dontSendNotification);
    zoneCountLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    zoneCountLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    addAndMakeVisible(zoneCountLabel_);

    mixKnob_.setRange(0.0, 1.0, 0.01);
    mixKnob_.setValue(0.0);
    addAndMakeVisible(mixKnob_);
    mixAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "sampleMix", mixKnob_);

    // ADSR knobs for sample envelope
    attackKnob_.setRange(0.1, 5000.0, 0.1);
    attackKnob_.setSkewFactorFromMidPoint(100.0);
    attackKnob_.setValue(10.0);
    addAndMakeVisible(attackKnob_);
    attackAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "sampleAttack", attackKnob_);

    decayKnob_.setRange(1.0, 5000.0, 0.1);
    decayKnob_.setSkewFactorFromMidPoint(200.0);
    decayKnob_.setValue(100.0);
    addAndMakeVisible(decayKnob_);
    decayAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "sampleDecay", decayKnob_);

    sustainKnob_.setRange(0.0, 1.0, 0.01);
    sustainKnob_.setValue(1.0);
    addAndMakeVisible(sustainKnob_);
    sustainAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "sampleSustain", sustainKnob_);

    releaseKnob_.setRange(1.0, 10000.0, 0.1);
    releaseKnob_.setSkewFactorFromMidPoint(500.0);
    releaseKnob_.setValue(200.0);
    addAndMakeVisible(releaseKnob_);
    releaseAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "sampleRelease", releaseKnob_);

    browseButton_.setButtonText("Browse...");
    browseButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    browseButton_.setColour(juce::TextButton::textColourOffId, SynthColors::neonYellow());
    browseButton_.onClick = [this]() { browseForSample(); };
    addAndMakeVisible(browseButton_);

    clearButton_.setButtonText("Clear");
    clearButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    clearButton_.setColour(juce::TextButton::textColourOffId, SynthColors::danger());
    clearButton_.onClick = [this]() { clearSample(); };
    addAndMakeVisible(clearButton_);

    editZonesButton_.setButtonText("Edit Zones");
    editZonesButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    editZonesButton_.setColour(juce::TextButton::textColourOffId, SynthColors::cyan());
    editZonesButton_.onClick = [this]() { showZoneEditor(); };
    addAndMakeVisible(editZonesButton_);

    zoneViewport_.setViewedComponent(&zoneContainer_, false);
    zoneViewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(zoneViewport_);

    startTimerHz(5);
}

void SamplePanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(SynthColors::hotPink());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("SAMPLE PLAYER", 12, 8, 150, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void SamplePanel::resized()
{
    int y = 38;
    sampleNameLabel_.setBounds(12, y, getWidth() - 24, 18);
    y += 18;
    categoryLabel_.setBounds(12, y, getWidth() - 24, 16);
    y += 18;
    zoneCountLabel_.setBounds(12, y, getWidth() - 24, 16);
    y += 22;

    mixKnob_.setBounds(12, y, 48, 64);
    attackKnob_.setBounds(64, y, 48, 64);
    decayKnob_.setBounds(116, y, 48, 64);
    sustainKnob_.setBounds(168, y, 48, 64);
    releaseKnob_.setBounds(220, y, 48, 64);
    browseButton_.setBounds(274, y + 4, 66, 22);
    clearButton_.setBounds(274, y + 30, 66, 22);
    editZonesButton_.setBounds(274, y + 56, 66, 22);
    y += 72;

    zoneViewport_.setBounds(12, y, getWidth() - 24, getHeight() - y - 8);
    zoneContainer_.setBounds(0, 0, zoneViewport_.getWidth() - 4, 200);

    if (zoneEditor_)
        zoneEditor_->setBounds(getLocalBounds());
}

void SamplePanel::refresh()
{
    rebuildZoneList();
}

void SamplePanel::timerCallback()
{
    refresh();
}

void SamplePanel::browseForSample()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select a sample or manifest...",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.flac;*.json");

    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File())
            return;

        auto* engine = processor_.getSynth().getEngine();
        if (!engine)
            return;

        if (!engine->getSamplePlayer())
        {
            auto sp = std::make_unique<SamplePlayer>();
            sp->prepare(engine->getSampleRate());
            engine->setSamplePlayer(std::move(sp));
        }

        auto* player = engine->getSamplePlayer();
        if (!player)
            return;

        if (file.getFileExtension() == ".json")
        {
            player->clear();
            if (player->loadMultiSample(file.getFullPathName().toStdString()))
            {
                sampleNameLabel_.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
                sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::text());
                categoryLabel_.setText("Multi-sample", juce::dontSendNotification);
            }
            else
            {
                sampleNameLabel_.setText("Failed to load manifest", juce::dontSendNotification);
                sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::danger());
            }
        }
        else
        {
            player->clear();
            if (player->loadSample(file.getFullPathName().toStdString(), 60, 0, 127))
            {
                sampleNameLabel_.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
                sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::text());
                categoryLabel_.setText("Single sample (C3 root)", juce::dontSendNotification);
            }
            else
            {
                sampleNameLabel_.setText("Failed to load sample", juce::dontSendNotification);
                sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::danger());
            }
        }

        rebuildZoneList();
    });
}

void SamplePanel::clearSample()
{
    auto* engine = processor_.getSynth().getEngine();
    if (engine && engine->getSamplePlayer())
    {
        engine->getSamplePlayer()->clear();
        engine->getSamplePlayer()->setMixLevel(0.0f);
    }

    sampleNameLabel_.setText("No sample loaded", juce::dontSendNotification);
    sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    categoryLabel_.setText("", juce::dontSendNotification);
    rebuildZoneList();
}

void SamplePanel::rebuildZoneList()
{
    zoneLabels_.clear();

    auto* engine = processor_.getSynth().getEngine();
    auto* player = engine ? engine->getSamplePlayer() : nullptr;
    int zoneCount = player ? player->zoneCount() : 0;
    zoneCountLabel_.setText("Zones: " + juce::String(zoneCount), juce::dontSendNotification);

    if (!player || zoneCount == 0)
    {
        zoneContainer_.removeAllChildren();
        zoneContainer_.setSize(zoneViewport_.getWidth() - 4, 20);
        auto* emptyLabel = new juce::Label();
        emptyLabel->setText("No zones loaded", juce::dontSendNotification);
        emptyLabel->setFont(juce::Font(juce::FontOptions(10.0f)));
        emptyLabel->setColour(juce::Label::textColourId, SynthColors::textDim());
        zoneContainer_.addAndMakeVisible(emptyLabel);
        emptyLabel->setBounds(4, 2, zoneContainer_.getWidth() - 8, 16);
        return;
    }

    int rowH = 18;
    int totalH = zoneCount * rowH + 4;
    zoneContainer_.setSize(zoneViewport_.getWidth() - 4, totalH);
    zoneContainer_.removeAllChildren();

    int y = 2;
    for (const auto& zone : player->getZones())
    {
        juce::String layers;
        int layerCount = 0;
        for (int i = 0; i < static_cast<int>(VelocityLayer::Count); ++i)
        {
            if (zone->streams[i])
            {
                if (layerCount > 0) layers += ", ";
                switch (static_cast<VelocityLayer>(i))
                {
                    case VelocityLayer::Soft:   layers += "S"; break;
                    case VelocityLayer::Medium: layers += "M"; break;
                    case VelocityLayer::Loud:   layers += "L"; break;
                    default: break;
                }
                ++layerCount;
            }
        }
        if (layerCount == 0) layers = "-";

        juce::String text = juce::String("Root ") + juce::MidiMessage::getMidiNoteName(zone->rootNote, true, true, 3)
            + " | " + juce::MidiMessage::getMidiNoteName(zone->minNote, true, true, 3)
            + "-" + juce::MidiMessage::getMidiNoteName(zone->maxNote, true, true, 3)
            + " | Vel " + juce::String(static_cast<int>(zone->minVelocity * 127))
            + "-" + juce::String(static_cast<int>(zone->maxVelocity * 127))
            + " | " + layers
            + (zone->rrGroup > 0 ? " | RR:" + juce::String(zone->rrGroup) : "")
            + (zone->isReleaseSample ? " | REL" : "")
            + (zone->startOffset > 0 ? " | Off:" + juce::String(zone->startOffset) : "");

        auto* label = new juce::Label();
        label->setText(text, juce::dontSendNotification);
        label->setFont(juce::Font(juce::FontOptions(10.0f)));
        label->setColour(juce::Label::textColourId, SynthColors::text());
        zoneContainer_.addAndMakeVisible(label);
        label->setBounds(4, y, zoneContainer_.getWidth() - 8, rowH);
        y += rowH;
    }

    // Release zones
    for (const auto& zone : player->getReleaseZones())
    {
        juce::String text = juce::String("REL ") + juce::MidiMessage::getMidiNoteName(zone->rootNote, true, true, 3)
            + " | " + juce::MidiMessage::getMidiNoteName(zone->minNote, true, true, 3)
            + "-" + juce::MidiMessage::getMidiNoteName(zone->maxNote, true, true, 3);

        auto* label = new juce::Label();
        label->setText(text, juce::dontSendNotification);
        label->setFont(juce::Font(juce::FontOptions(10.0f)));
        label->setColour(juce::Label::textColourId, SynthColors::danger());
        zoneContainer_.addAndMakeVisible(label);
        label->setBounds(4, y, zoneContainer_.getWidth() - 8, rowH);
        y += rowH;
    }
}

// ── Drag & Drop ──────────────────────────────────────────────────────────────

bool SamplePanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aiff")
            || f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".json"))
            return true;
    }
    return false;
}

void SamplePanel::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.isEmpty()) return;
    loadFile(juce::File(files[0]));
}

void SamplePanel::loadFile(const juce::File& file)
{
    auto* engine = processor_.getSynth().getEngine();
    if (!engine) return;

    if (!engine->getSamplePlayer())
    {
        auto sp = std::make_unique<SamplePlayer>();
        sp->prepare(engine->getSampleRate());
        engine->setSamplePlayer(std::move(sp));
    }

    auto* player = engine->getSamplePlayer();
    if (!player) return;

    if (file.getFileExtension() == ".json")
    {
        player->clear();
        if (player->loadMultiSample(file.getFullPathName().toStdString()))
        {
            sampleNameLabel_.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
            sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::text());
            categoryLabel_.setText("Multi-sample", juce::dontSendNotification);
        }
        else
        {
            sampleNameLabel_.setText("Failed to load manifest", juce::dontSendNotification);
            sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::danger());
        }
    }
    else
    {
        player->clear();
        if (player->loadSample(file.getFullPathName().toStdString(), 60, 0, 127))
        {
            sampleNameLabel_.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
            sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::text());
            categoryLabel_.setText("Single sample (C3 root)", juce::dontSendNotification);
        }
        else
        {
            sampleNameLabel_.setText("Failed to load sample", juce::dontSendNotification);
            sampleNameLabel_.setColour(juce::Label::textColourId, SynthColors::danger());
        }
    }

    rebuildZoneList();
}

// ── Zone Editor ─────────────────────────────────────────────────────────────

static const juce::Colour zoneColors[] = {
    juce::Colour(0xFF00F0FF),  // cyan
    juce::Colour(0xFFFF7EDB),  // hot pink
    juce::Colour(0xFFF3E70F),  // yellow
    juce::Colour(0xFF8F00FF),  // purple
    juce::Colour(0xFF00FF7F),  // green
    juce::Colour(0xFFFF6600),  // orange
    juce::Colour(0xFF6666FF),  // blue
    juce::Colour(0xFFFF0066),  // red
};

ZoneEditor::ZoneEditor(OpenSynthProcessor& processor, std::function<void()> onClose)
    : processor_(processor), onClose_(std::move(onClose))
{
    setOpaque(false);

    titleLabel_.setText("ZONE EDITOR", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::cyan());
    addAndMakeVisible(titleLabel_);

    closeButton_.setButtonText("X");
    closeButton_.setColour(juce::TextButton::buttonColourId, SynthColors::danger());
    closeButton_.setColour(juce::TextButton::textColourOffId, SynthColors::text());
    closeButton_.onClick = [this]() { onClose_(); };
    addAndMakeVisible(closeButton_);

    selLabel_.setText("No zone selected", juce::dontSendNotification);
    selLabel_.setFont(juce::Font(juce::FontOptions(12.0f)));
    selLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    addAndMakeVisible(selLabel_);

    // Note sliders
    auto makeNoteSlider = [this](juce::Slider& s, const juce::String& /*name*/) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 18);
        s.setRange(0.0, 127.0, 1.0);
        addAndMakeVisible(s);
    };

    rootNoteSlider_.onValueChange = [this]() { applySlidersToZone(); };
    minNoteSlider_.onValueChange = [this]() { applySlidersToZone(); };
    maxNoteSlider_.onValueChange = [this]() { applySlidersToZone(); };
    minVelSlider_.onValueChange = [this]() { applySlidersToZone(); };
    maxVelSlider_.onValueChange = [this]() { applySlidersToZone(); };

    makeNoteSlider(rootNoteSlider_, "Root");
    makeNoteSlider(minNoteSlider_, "Min");
    makeNoteSlider(maxNoteSlider_, "Max");

    minVelSlider_.setRange(0.0, 127.0, 1.0);
    minVelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    minVelSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 18);
    addAndMakeVisible(minVelSlider_);

    maxVelSlider_.setRange(0.0, 127.0, 1.0);
    maxVelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    maxVelSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 42, 18);
    addAndMakeVisible(maxVelSlider_);

    loopToggle_.setButtonText("Loop");
    loopToggle_.setColour(juce::ToggleButton::textColourId, SynthColors::text());
    loopToggle_.setColour(juce::ToggleButton::tickColourId, SynthColors::hotPink());
    loopToggle_.onClick = [this]() { applySlidersToZone(); };
    addAndMakeVisible(loopToggle_);

    deleteButton_.setButtonText("Delete Zone");
    deleteButton_.setColour(juce::TextButton::buttonColourId, SynthColors::danger());
    deleteButton_.setColour(juce::TextButton::textColourOffId, SynthColors::text());
    deleteButton_.onClick = [this]() {
        auto* engine = processor_.getSynth().getEngine();
        auto* player = engine ? engine->getSamplePlayer() : nullptr;
        if (player && selectedZone_ >= 0 && selectedZone_ < player->zoneCount()) {
            player->removeZone(selectedZone_);
            selectedZone_ = -1;
            updateSlidersFromZone();
            repaint();
        }
    };
    addAndMakeVisible(deleteButton_);
}

float ZoneEditor::noteToX(int note, float width) const
{
    return (static_cast<float>(note - kKeyStart) / static_cast<float>(kNumKeys)) * width;
}

int ZoneEditor::xToNote(float x, float width) const
{
    int note = static_cast<int>((x / width) * kNumKeys) + kKeyStart;
    return std::clamp(note, 0, 127);
}

void ZoneEditor::updateSlidersFromZone()
{
    auto* engine = processor_.getSynth().getEngine();
    auto* player = engine ? engine->getSamplePlayer() : nullptr;
    if (!player || selectedZone_ < 0 || selectedZone_ >= player->zoneCount()) {
        selLabel_.setText("No zone selected", juce::dontSendNotification);
        rootNoteSlider_.setEnabled(false);
        minNoteSlider_.setEnabled(false);
        maxNoteSlider_.setEnabled(false);
        minVelSlider_.setEnabled(false);
        maxVelSlider_.setEnabled(false);
        loopToggle_.setEnabled(false);
        deleteButton_.setEnabled(false);
        return;
    }

    const auto& zones = player->getZones();
    const auto& zone = zones[selectedZone_];
    rootNoteSlider_.setValue(zone->rootNote, juce::dontSendNotification);
    minNoteSlider_.setValue(zone->minNote, juce::dontSendNotification);
    maxNoteSlider_.setValue(zone->maxNote, juce::dontSendNotification);
    minVelSlider_.setValue(zone->minVelocity * 127.0f, juce::dontSendNotification);
    maxVelSlider_.setValue(zone->maxVelocity * 127.0f, juce::dontSendNotification);
    loopToggle_.setToggleState(zone->loopEnabled, juce::dontSendNotification);

    juce::String name = juce::MidiMessage::getMidiNoteName(zone->rootNote, true, true, 3);
    selLabel_.setText("Zone " + juce::String(selectedZone_) + " — Root " + name,
                      juce::dontSendNotification);
    selLabel_.setColour(juce::Label::textColourId, SynthColors::text());

    rootNoteSlider_.setEnabled(true);
    minNoteSlider_.setEnabled(true);
    maxNoteSlider_.setEnabled(true);
    minVelSlider_.setEnabled(true);
    maxVelSlider_.setEnabled(true);
    loopToggle_.setEnabled(true);
    deleteButton_.setEnabled(true);
}

void ZoneEditor::applySlidersToZone()
{
    auto* engine = processor_.getSynth().getEngine();
    auto* player = engine ? engine->getSamplePlayer() : nullptr;
    if (!player || selectedZone_ < 0 || selectedZone_ >= player->zoneCount()) return;

    auto& zones = player->getZones();
    auto& zone = zones[selectedZone_];
    zone->rootNote = static_cast<int>(rootNoteSlider_.getValue());
    zone->minNote = static_cast<int>(minNoteSlider_.getValue());
    zone->maxNote = static_cast<int>(maxNoteSlider_.getValue());
    zone->minVelocity = static_cast<float>(minVelSlider_.getValue()) / 127.0f;
    zone->maxVelocity = static_cast<float>(maxVelSlider_.getValue()) / 127.0f;
    zone->loopEnabled = loopToggle_.getToggleState();
    repaint();
}

void ZoneEditor::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::surface().withAlpha(0.95f));
    g.fillRoundedRectangle(b, 12.0f);

    g.setColour(SynthColors::card());
    g.drawRoundedRectangle(b, 12.0f, 2.0f);

    // Keyboard map area (below title bar)
    float mapTop = 44.0f;
    float mapH = 120.0f;
    float mapW = static_cast<float>(getWidth()) - 24.0f;
    float mapX = 12.0f;

    // Background
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(mapX, mapTop, mapW, mapH, 6.0f);

    // Draw piano keys
    for (int n = kKeyStart; n < kKeyEnd; ++n)
    {
        float x = mapX + noteToX(n, mapW);
        float w = noteToX(n + 1, mapW) - noteToX(n, mapW);
        int noteInOctave = n % 12;
        bool isBlack = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6
                        || noteInOctave == 8 || noteInOctave == 10);

        g.setColour(isBlack ? juce::Colour(0xFF1A1A2E) : juce::Colour(0xFF2D2D44));
        g.fillRect(x, mapTop + mapH - 20.0f, w, 20.0f);

        // Note name for C notes
        if (noteInOctave == 0) {
            g.setColour(SynthColors::textDim());
            g.setFont(juce::Font(juce::FontOptions(8.0f)));
            int octave = n / 12 - 1;
            g.drawText("C" + juce::String(octave), static_cast<int>(x), static_cast<int>(mapTop + mapH - 18), static_cast<int>(w), 16, juce::Justification::centred);
        }
    }

    // Draw zone bars
    auto* engine = processor_.getSynth().getEngine();
    auto* player = engine ? engine->getSamplePlayer() : nullptr;
    if (player) {
        const auto& zones = player->getZones();
        float zoneBarTop = mapTop + 8.0f;
        float zoneBarH = 60.0f;

        for (int i = 0; i < static_cast<int>(zones.size()); ++i) {
            const auto& z = zones[i];
            float x1 = mapX + noteToX(std::max(z->minNote, kKeyStart), mapW);
            float x2 = mapX + noteToX(std::min(z->maxNote + 1, kKeyEnd), mapW);
            float w = x2 - x1;
            if (w < 1.0f) w = 1.0f;

            juce::Colour color = zoneColors[i % 8];
            float alpha = (i == selectedZone_) ? 0.9f : 0.5f;

            g.setColour(color.withAlpha(alpha));
            g.fillRoundedRectangle(x1, zoneBarTop, w, zoneBarH, 4.0f);

            // Root note marker
            float rootX = mapX + noteToX(z->rootNote, mapW);
            g.setColour(SynthColors::text());
            g.drawVerticalLine(static_cast<int>(rootX), zoneBarTop, zoneBarTop + zoneBarH);

            // Zone label
            if (w > 30.0f) {
                g.setColour(SynthColors::text());
                g.setFont(juce::Font(juce::FontOptions(9.0f)));
                juce::String name = juce::MidiMessage::getMidiNoteName(z->rootNote, true, true, 3);
                g.drawText("#" + juce::String(i) + " " + name,
                           static_cast<int>(x1 + 4), static_cast<int>(zoneBarTop + 2),
                           static_cast<int>(w - 8), 14, juce::Justification::left);
            }

            // Selection outline
            if (i == selectedZone_) {
                g.setColour(color);
                g.drawRoundedRectangle(x1, zoneBarTop, w, zoneBarH, 4.0f, 2.0f);
            }
        }
    }
}

void ZoneEditor::resized()
{
    int w = getWidth();
    titleLabel_.setBounds(12, 10, 160, 28);
    closeButton_.setBounds(w - 44, 10, 30, 28);

    // Edit controls below the keyboard map (map is at y=44..164)
    int y = 174;
    int colW = (w - 24) / 2;

    selLabel_.setBounds(12, y, w - 24, 18);
    y += 22;

    // Left column: note sliders
    auto addSliderRow = [&](juce::Slider& s, int yy, const char* /*label*/) {
        // Label drawn in paint, slider occupies full row
        s.setBounds(56, yy, colW - 68, 22);
    };

    addSliderRow(rootNoteSlider_, y, "Root");   y += 26;
    addSliderRow(minNoteSlider_, y, "Min Note"); y += 26;
    addSliderRow(maxNoteSlider_, y, "Max Note"); y += 26;

    // Right column: velocity + loop + delete
    int rx = colW + 16;
    minVelSlider_.setBounds(rx + 56, 196, colW - 68, 22);
    maxVelSlider_.setBounds(rx + 56, 222, colW - 68, 22);
    loopToggle_.setBounds(rx, 248, 80, 22);
    deleteButton_.setBounds(rx + 90, 248, 100, 22);
}

void ZoneEditor::mouseDown(const juce::MouseEvent& e)
{
    // Check if click is in the zone map area
    float mapTop = 44.0f;
    float mapH = 120.0f;
    float mapW = static_cast<float>(getWidth()) - 24.0f;
    float mapX = 12.0f;

    if (e.y < mapTop || e.y > mapTop + mapH + 68.0f) return;

    auto* engine = processor_.getSynth().getEngine();
    auto* player = engine ? engine->getSamplePlayer() : nullptr;
    if (!player) return;

    const auto& zones = player->getZones();
    int clickNote = xToNote(static_cast<float>(e.x) - mapX, mapW);

    // Find the zone that contains this note
    for (int i = 0; i < static_cast<int>(zones.size()); ++i) {
        if (clickNote >= zones[i]->minNote && clickNote <= zones[i]->maxNote) {
            selectedZone_ = i;
            updateSlidersFromZone();
            repaint();
            return;
        }
    }

    // Clicked empty area — deselect
    selectedZone_ = -1;
    updateSlidersFromZone();
    repaint();
}

// ── Zone Editor (stub) ─────────────────────────────────────────────────────

void SamplePanel::showZoneEditor()
{
    auto* editor = new ZoneEditor(processor_, [this]() { hideZoneEditor(); });
    zoneEditor_.reset(editor);
    addAndMakeVisible(zoneEditor_.get());
    zoneEditor_->setBounds(getLocalBounds());
}

void SamplePanel::hideZoneEditor()
{
    zoneEditor_.reset();
    rebuildZoneList();
}

// ── Performance Panel ───────────────────────────────────────────────────────

PerformancePanel::PerformancePanel(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
    titleLabel_.setText("PERFORMANCE", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::cyan());
    addAndMakeVisible(titleLabel_);

    splitLabel_.setText("Split", juce::dontSendNotification);
    splitLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    splitLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(splitLabel_);

    splitSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    splitSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    splitSlider_.setRange(21, 108, 1);
    splitSlider_.setValue(60);
    addAndMakeVisible(splitSlider_);
    splitAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "perfSplitPoint", splitSlider_);

    layerButton_.setButtonText("Layer");
    layerButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::hotPink());
    addAndMakeVisible(layerButton_);
    layerAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, "perfLayerEnabled", layerButton_);

    splitButton_.setButtonText("Split");
    splitButton_.setColour(juce::ToggleButton::tickColourId, SynthColors::hotPink());
    addAndMakeVisible(splitButton_);
    splitEnableAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts_, "perfSplitEnabled", splitButton_);

    transposeLabel_.setText("Transpose", juce::dontSendNotification);
    transposeLabel_.setColour(juce::Label::textColourId, SynthColors::textDim());
    transposeLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(transposeLabel_);

    transposeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    transposeSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    transposeSlider_.setRange(-12, 12, 1);
    transposeSlider_.setValue(0);
    addAndMakeVisible(transposeSlider_);
    transposeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts_, "perfTranspose", transposeSlider_);

    // Callbacks
    splitSlider_.onValueChange = [this]()
    {
        if (onSplitChanged)
            onSplitChanged(static_cast<int>(splitSlider_.getValue()));
    };

    layerButton_.onClick = [this]()
    {
        if (onLayerChanged)
            onLayerChanged(layerButton_.getToggleState());
    };
}

void PerformancePanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(SynthColors::card());
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(SynthColors::cyan());
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText("PERFORMANCE", 12, 8, 120, 20, juce::Justification::left);
    g.setColour(SynthColors::gridLine());
    g.drawHorizontalLine(32, 8, getWidth() - 8);
}

void PerformancePanel::resized()
{
    titleLabel_.setBounds(12, 8, 120, 20);

    int y = 38;
    splitLabel_.setBounds(12, y, 50, 20);
    splitSlider_.setBounds(64, y, getWidth() - 76, 20);
    y += 28;

    layerButton_.setBounds(12, y, 80, 22);
    splitButton_.setBounds(100, y, 80, 22);
    y += 30;

    transposeLabel_.setBounds(12, y, 60, 20);
    transposeSlider_.setBounds(74, y, getWidth() - 86, 20);
}

bool OpenSynthEditor::keyPressed(const juce::KeyPress& key)
{
    // Undo/Redo
    auto mods = juce::ModifierKeys::getCurrentModifiers();
    if (mods.isCtrlDown() && key.getKeyCode() == 'z' && !mods.isShiftDown())
    {
        processor_.getUndoManager().undo();
        return true;
    }
    if (mods.isCtrlDown() && (key.getKeyCode() == 'y' || (key.getKeyCode() == 'z' && mods.isShiftDown())))
    {
        processor_.getUndoManager().redo();
        return true;
    }

    // Preset navigation: Up/Down arrows
    if (key == juce::KeyPress::upKey)
    {
        if (kNumFullPresets > 0) {
            currentPresetIndex_--;
            if (currentPresetIndex_ < 0)
                currentPresetIndex_ = kNumFullPresets - 1;
            loadPresetByIndex(currentPresetIndex_);
        }
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        if (kNumFullPresets > 0) {
            currentPresetIndex_++;
            if (currentPresetIndex_ >= kNumFullPresets)
                currentPresetIndex_ = 0;
            loadPresetByIndex(currentPresetIndex_);
        }
        return true;
    }

    // Space = toggle arpeggiator
    if (key == juce::KeyPress::spaceKey)
    {
        auto* param = processor_.getParameters().getParameter("arpEnabled");
        if (param != nullptr)
        {
            bool newVal = !param->getValue();
            param->beginChangeGesture();
            param->setValueNotifyingHost(newVal ? 1.0f : 0.0f);
            param->endChangeGesture();
        }
        return true;
    }

    // Ctrl+Z = undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0))
    {
        processor_.getUndoManager().undo();
        showUndoFeedback("Undo");
        return true;
    }

    // Ctrl+Y = redo
    if (key == juce::KeyPress('y', juce::ModifierKeys::ctrlModifier, 0))
    {
        processor_.getUndoManager().redo();
        showUndoFeedback("Redo");
        return true;
    }

    // Ctrl+Shift+Z = redo (alternative)
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        processor_.getUndoManager().redo();
        showUndoFeedback("Redo");
        return true;
    }

    // 1-8 = favorite presets
    int favKey = key.getTextCharacter();
    if (favKey >= '1' && favKey <= '8')
    {
        loadFavoritePreset(favKey - '1');
        return true;
    }

    // P = open preset browser
    if (favKey == 'p' || favKey == 'P')
    {
        showPresetBrowser();
        return true;
    }

    // M = toggle master mute (set master volume to 0 / restore)
    if (favKey == 'm' || favKey == 'M')
    {
        auto* param = processor_.getParameters().getParameter("masterVolume");
        if (param != nullptr)
        {
            static float lastVolume = 0.8f;
            float current = param->getValue();
            if (current > 0.01f)
            {
                lastVolume = current;
                param->beginChangeGesture();
                param->setValueNotifyingHost(0.0f);
                param->endChangeGesture();
            }
            else
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(lastVolume);
                param->endChangeGesture();
            }
        }
        return true;
    }

    // Computer keyboard as piano
    int note = keyToNote(favKey);
    if (note >= 0)
    {
        if (heldKeys_.insert(favKey).second) // was not already held
        {
            processor_.injectMidiMessage(juce::MidiMessage::noteOn(1, note, 0.9f));
            keyboard_.setKeyPressed(note);
        }
        return true;
    }

    return false;
}

bool OpenSynthEditor::keyStateChanged(bool isKeyDown)
{
    juce::ignoreUnused(isKeyDown);

    // Check which held keys have been released
    std::vector<int> released;
    for (int keyCode : heldKeys_)
    {
        if (!juce::KeyPress::isKeyCurrentlyDown(keyCode))
        {
            int note = keyToNote(keyCode);
            if (note >= 0)
            {
                processor_.injectMidiMessage(juce::MidiMessage::noteOff(1, note, 0.0f));
                keyboard_.setKeyReleased(note);
            }
            released.push_back(keyCode);
        }
    }
    for (int keyCode : released)
        heldKeys_.erase(keyCode);
    return false; // allow other handlers
}

int OpenSynthEditor::keyToNote(int keyCode)
{
    switch (keyCode)
    {
        case 'z': return 48;
        case 's': return 49;
        case 'x': return 50;
        case 'd': return 51;
        case 'c': return 52;
        case 'v': return 53;
        case 'g': return 54;
        case 'b': return 55;
        case 'h': return 56;
        case 'n': return 57;
        case 'j': return 58;
        case 'm': return 59;
        case 'q': return 60;
        case '2': return 61;
        case 'w': return 62;
        case '3': return 63;
        case 'e': return 64;
        case 'r': return 65;
        case '5': return 66;
        case 't': return 67;
        case '6': return 68;
        case 'y': return 69;
        case '7': return 70;
        case 'u': return 71;
        default:  return -1;
    }
}

} // namespace opensynth

// ── SetlistOverlay ──────────────────────────────────────────────────────────

namespace opensynth {

SetlistOverlay::SetlistOverlay()
{
    setOpaque(true);

    titleLabel_.setText("SETLIST MODE", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId, SynthColors::cyan());
    addAndMakeVisible(titleLabel_);

    closeButton_.setButtonText("X");
    closeButton_.setColour(juce::TextButton::buttonColourId, SynthColors::danger());
    closeButton_.onClick = [this]() { setVisible(false); };
    addAndMakeVisible(closeButton_);

    clearButton_.setButtonText("Clear All");
    clearButton_.setColour(juce::TextButton::buttonColourId, SynthColors::card());
    clearButton_.setColour(juce::TextButton::textColourOffId, SynthColors::textDim());
    clearButton_.onClick = [this]() { clearAll(); };
    addAndMakeVisible(clearButton_);

    for (int i = 0; i < kNumSlots; ++i)
    {
        slotPresetNames_[i] = "Empty";
        slotButtons_[i].setButtonText(juce::String(i + 1) + ": Empty");
        slotButtons_[i].setColour(juce::TextButton::buttonColourId, SynthColors::card());
        slotButtons_[i].setColour(juce::TextButton::textColourOffId, SynthColors::textDim());
        slotButtons_[i].onClick = [this, i]() { slotClicked(i); };
        addAndMakeVisible(slotButtons_[i]);
    }
}

void SetlistOverlay::paint(juce::Graphics& g)
{
    g.fillAll(SynthColors::background().withAlpha(0.95f));

    g.setColour(SynthColors::gridLine());
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0, getHeight());
    for (int y = 0; y < getHeight(); y += 40)
        g.drawHorizontalLine(y, 0, getWidth());
}

void SetlistOverlay::resized()
{
    auto b = getLocalBounds().reduced(20);
    titleLabel_.setBounds(b.removeFromTop(30));
    closeButton_.setBounds(getWidth() - 50, 20, 30, 30);

    auto controls = b.removeFromTop(30);
    clearButton_.setBounds(controls.removeFromLeft(100));
    b.removeFromTop(12);

    int cols = 4;
    int rows = kNumSlots / cols;
    int btnWidth = (b.getWidth() - (cols - 1) * 12) / cols;
    int btnHeight = (b.getHeight() - (rows - 1) * 12) / rows;

    for (int i = 0; i < kNumSlots; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        slotButtons_[i].setBounds(
            b.getX() + col * (btnWidth + 12),
            b.getY() + row * (btnHeight + 12),
            btnWidth, btnHeight);
    }
}

void SetlistOverlay::setVisible(bool shouldBeVisible)
{
    juce::Component::setVisible(shouldBeVisible);
}

void SetlistOverlay::slotClicked(int index)
{
    if (juce::ModifierKeys::getCurrentModifiers().isShiftDown())
    {
        if (onSlotAssigned)
            onSlotAssigned(index);
    }
    else
    {
        if (onSlotSelected)
            onSlotSelected(index);
    }
}

void SetlistOverlay::assignPresetToSlot(int slotIndex, const juce::String& presetName)
{
    if (slotIndex >= 0 && slotIndex < kNumSlots)
    {
        slotPresetNames_[slotIndex] = presetName.isEmpty() ? "Empty" : presetName;
        slotButtons_[slotIndex].setButtonText(juce::String(slotIndex + 1) + ": " + slotPresetNames_[slotIndex]);
        slotButtons_[slotIndex].setColour(juce::TextButton::textColourOffId,
            presetName.isEmpty() ? SynthColors::textDim() : SynthColors::neonYellow());
    }
}

juce::String SetlistOverlay::getSlotPresetName(int slotIndex) const
{
    if (slotIndex >= 0 && slotIndex < kNumSlots)
        return slotPresetNames_[slotIndex];
    return {};
}

void SetlistOverlay::clearSlot(int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < kNumSlots)
        assignPresetToSlot(slotIndex, "Empty");
}

void SetlistOverlay::clearAll()
{
    for (int i = 0; i < kNumSlots; ++i)
        assignPresetToSlot(i, "Empty");
}

} // namespace opensynth

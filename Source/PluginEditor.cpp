#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      { 0xff1d1b1a };
    const juce::Colour kPanel   { 0xff2b2725 };
    const juce::Colour kAccent  { 0xffd9772b };
    const juce::Colour kText    { 0xffece3d8 };
    constexpr int kCell = 86, kCellH = 96, kHeader = 26, kPad = 8;
}

BabushkaEditor::BabushkaEditor (BabushkaProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    lnf.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    lnf.setColour (juce::Slider::thumbColourId, kAccent);
    lnf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lnf.setColour (juce::ComboBox::backgroundColourId, kBg);
    lnf.setColour (juce::ComboBox::outlineColourId, kAccent.withAlpha (0.5f));
    lnf.setColour (juce::Label::textColourId, kText);
    lnf.setColour (juce::ToggleButton::tickColourId, kAccent);
    setLookAndFeel (&lnf);

    addSection ("MODULATION",  { "lfoRate", "lfoWave", "lfoPitchAmt", "lfoCutoffAmt", "lfoPwAmt", "modWheel" });
    addSection ("OSCILLATOR 1", { "osc1Octave", "osc1Wave" });
    addSection ("OSCILLATOR 2", { "osc2Octave", "osc2Wave", "osc2Freq", "osc2Sync" });
    addSection ("MIXER",       { "mixOsc1", "mixOsc2", "mixNoise" });
    addSection ("FILTER",      { "cutoff", "resonance", "kbdTrack", "envAmt" });
    addSection ("ENVELOPE",    { "attack", "decay", "sustain", "release" });
    addSection ("OUTPUT",      { "vcaMode", "reverbMix", "volume" });
    addSection ("CONTROLLER / GLOBAL", { "glide", "glideType", "legatoGlide", "bendRange", "notePriority", "fineTune", "drift", "oversampling" });

    setResizable (true, true);
    setResizeLimits (720, 480, 1800, 1200);
    setSize (1040, 700);
}

BabushkaEditor::~BabushkaEditor() { setLookAndFeel (nullptr); }

void BabushkaEditor::addSection (const juce::String& title, std::initializer_list<const char*> paramIds)
{
    Section s; s.title = title;
    for (auto* id : paramIds)
    {
        auto* param = processor.apvts.getParameter (id);
        jassert (param != nullptr);
        auto c = std::make_unique<Control>();
        c->paramId = id;
        c->label = std::make_unique<juce::Label> (juce::String(), param->getName (32));
        c->label->setJustificationType (juce::Justification::centred);
        c->label->setFont (juce::FontOptions (12.0f));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            auto box = std::make_unique<juce::ComboBox>();
            box->addItemList (choice->choices, 1);
            c->comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, *box);
            c->widget = std::move (box);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            auto b = std::make_unique<juce::ToggleButton> ("on");
            c->buttonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id, *b);
            c->widget = std::move (b);
        }
        else
        {
            auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
            sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
            sl->setNumDecimalPlacesToDisplay (2);
            c->sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, *sl);
            sl->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            c->widget = std::move (sl);
        }
        addAndMakeVisible (*c->widget);
        addAndMakeVisible (*c->label);
        s.controls.push_back (std::move (c));
    }
    sections.push_back (std::move (s));
}

void BabushkaEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("BABUSHKA", 14, 6, 300, 30, juce::Justification::centredLeft);
    g.setColour (kText.withAlpha (0.55f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("semi-modular analog voice  |  core build 0.1", 170, 6, 400, 30, juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        g.setColour (kPanel);
        g.fillRoundedRectangle (s.bounds.toFloat(), 6.0f);
        g.setColour (kAccent);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (s.title, s.bounds.getX() + 10, s.bounds.getY() + 4, s.bounds.getWidth() - 20, 18, juce::Justification::centredLeft);
    }
}

void BabushkaEditor::resized()
{
    // Flow layout: sections wrap across rows; each section is a grid of cells.
    const int top = 42, width = getWidth() - 2 * kPad;
    int x = kPad, y = top, rowH = 0;
    for (auto& s : sections)
    {
        const int n = (int) s.controls.size();
        const int cols = std::min (n, 4);
        const int rows = (n + cols - 1) / cols;
        const int w = cols * kCell + 12, h = kHeader + rows * kCellH + 6;
        if (x + w > kPad + width) { x = kPad; y += rowH + kPad; rowH = 0; }
        s.bounds = { x, y, w, h };
        for (int i = 0; i < n; ++i)
        {
            auto& c = *s.controls[(size_t) i];
            const int cx = x + 6 + (i % cols) * kCell, cy = y + kHeader + (i / cols) * kCellH;
            c.label->setBounds (cx, cy, kCell, 16);
            const bool isKnob = dynamic_cast<juce::Slider*> (c.widget.get()) != nullptr;
            if (isKnob) c.widget->setBounds (cx + 4, cy + 16, kCell - 8, kCellH - 20);
            else        c.widget->setBounds (cx + 4, cy + 34, kCell - 8, 24);
        }
        x += w + kPad;
        rowH = std::max (rowH, h);
    }
}

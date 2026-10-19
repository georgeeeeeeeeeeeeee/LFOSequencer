#include "PluginEditor.h"

// ── Palette ───────────────────────────────────────────────────────────────────
static const juce::Colour kSilver { 0xFFC0C0C0 };
static const juce::Colour kNavy   { 0xFF000080 };
static const juce::Colour kBlack  { 0xFF000000 };
static const juce::Colour kWhite  { 0xFFFFFFFF };
static const juce::Colour kGray   { 0xFF808080 };
static const juce::Colour kDkGray { 0xFF404040 };
static const juce::Colour kLtGray { 0xFFDFDFDF };

// ── 3D bevel helper ───────────────────────────────────────────────────────────
// raised=true  → raised surface (buttons, panels)
// raised=false → sunken field  (inputs, displays)
static void draw3DBorder(juce::Graphics& g, juce::Rectangle<int> r, bool raised)
{
    g.setColour(raised ? kWhite : kDkGray);
    g.drawHorizontalLine(r.getY(),        (float)r.getX(),     (float)r.getRight());
    g.drawVerticalLine  (r.getX(),        (float)(r.getY()+1), (float)r.getBottom());

    g.setColour(raised ? kDkGray : kWhite);
    g.drawHorizontalLine(r.getBottom()-1, (float)r.getX(),     (float)r.getRight());
    g.drawVerticalLine  (r.getRight()-1,  (float)r.getY(),     (float)r.getBottom());

    g.setColour(raised ? kLtGray : kGray);
    g.drawHorizontalLine(r.getY()+1,      (float)(r.getX()+1), (float)(r.getRight()-1));
    g.drawVerticalLine  (r.getX()+1,      (float)(r.getY()+2), (float)(r.getBottom()-1));

    g.setColour(raised ? kGray : kLtGray);
    g.drawHorizontalLine(r.getBottom()-2, (float)(r.getX()+1), (float)(r.getRight()-1));
    g.drawVerticalLine  (r.getRight()-2,  (float)(r.getY()+1), (float)(r.getBottom()-1));
}

// ── MIDI note helpers ─────────────────────────────────────────────────────────
static juce::String midiNoteName(int n)
{
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String(names[n % 12]) + juce::String((n / 12) - 1);
}

static int parseNoteInput(const juce::String& s)
{
    auto t = s.trim().toUpperCase();
    if (t.isEmpty()) return -1;
    if (t.containsOnly("0123456789"))
        return juce::jlimit(0, 127, t.getIntValue());

    // Check sharps first so "C#" beats "C"
    static const char* noteNames[] = { "C#","D#","F#","G#","A#","C","D","E","F","G","A","B" };
    static const int   semitones[] = {  1,   3,   6,   8,   10,  0,  2,  4,  5,  7,  9,  11 };
    int semitone = -1, nameLen = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (t.startsWith(noteNames[i]))
        {
            semitone = semitones[i];
            nameLen  = (int)std::strlen(noteNames[i]);
            break;
        }
    }
    if (semitone < 0) return -1;
    auto octStr  = t.substring(nameLen);
    bool neg     = octStr.startsWith("-");
    if (neg) octStr = octStr.substring(1);
    if (octStr.isEmpty() || !octStr.containsOnly("0123456789")) return -1;
    int oct = octStr.getIntValue() * (neg ? -1 : 1);
    return juce::jlimit(0, 127, (oct + 1) * 12 + semitone);
}

//==============================================================================
// Win95LookAndFeel
//==============================================================================
Win95LookAndFeel::Win95LookAndFeel()
{
    setColour(juce::TextButton::buttonColourId,                   kSilver);
    setColour(juce::TextButton::textColourOffId,                   kBlack);
    setColour(juce::ToggleButton::textColourId,                    kBlack);
    setColour(juce::ToggleButton::tickColourId,                    kBlack);
    setColour(juce::ComboBox::backgroundColourId,                  kWhite);
    setColour(juce::ComboBox::textColourId,                        kBlack);
    setColour(juce::ComboBox::outlineColourId,                     juce::Colours::transparentBlack);
    setColour(juce::Label::backgroundColourId,                     juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,                           kBlack);
    setColour(juce::PopupMenu::backgroundColourId,                 kSilver);
    setColour(juce::PopupMenu::textColourId,                       kBlack);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,      kNavy);
    setColour(juce::PopupMenu::highlightedTextColourId,            kWhite);
}

void Win95LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool, bool isDown)
{
    g.setColour(kSilver);
    g.fillRect(button.getLocalBounds());
    draw3DBorder(g, button.getLocalBounds(), !isDown);
}

void Win95LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool, bool)
{
    const int boxSize = 13;
    int cy = (button.getHeight() - boxSize) / 2;
    juce::Rectangle<int> box(3, cy, boxSize, boxSize);

    // Sunken bevel around the checkbox square
    draw3DBorder(g, box, false);

    // White fill inside
    g.setColour(kWhite);
    g.fillRect(box.reduced(2));

    // Checkmark (X style)
    if (button.getToggleState())
    {
        g.setColour(kBlack);
        auto f = box.reduced(3).toFloat();
        g.drawLine(f.getX(), f.getY(), f.getRight(), f.getBottom(), 1.5f);
        g.drawLine(f.getRight(), f.getY(), f.getX(), f.getBottom(), 1.5f);
    }

    // Label text to the right of the box
    g.setColour(kBlack);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.drawText(button.getButtonText(),
               box.getRight() + 4, 0,
               button.getWidth() - box.getRight() - 4, button.getHeight(),
               juce::Justification::centredLeft);
}

void Win95LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                     bool isButtonDown, int, int, int, int,
                                     juce::ComboBox&)
{
    const int arrowW = 16;
    auto fullBounds  = juce::Rectangle<int>(0, 0, width, height);
    auto textBounds  = fullBounds.withTrimmedRight(arrowW);
    auto arrowBounds = fullBounds.removeFromRight(arrowW);

    // Text area — white fill, sunken bevel over the whole combo
    g.setColour(kWhite);
    g.fillRect(textBounds);
    draw3DBorder(g, juce::Rectangle<int>(0, 0, width, height), false);

    // Arrow button — silver fill, raised or sunken
    g.setColour(kSilver);
    g.fillRect(arrowBounds);
    draw3DBorder(g, arrowBounds, !isButtonDown);

    // Draw a small downward-pointing triangle
    auto c = arrowBounds.getCentre().toFloat();
    juce::Path tri;
    tri.addTriangle(c.getX() - 4.0f, c.getY() - 2.0f,
                    c.getX() + 4.0f, c.getY() - 2.0f,
                    c.getX(),         c.getY() + 3.0f);
    g.setColour(kBlack);
    g.fillPath(tri);
}

void Win95LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(3, 1, box.getWidth() - 19, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
}

void Win95LookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto bg = label.findColour(juce::Label::backgroundColourId);
    g.fillAll(bg);

    // Sunken 1-px bevel for white-background (value box) labels
    bool isWhiteBg = (!bg.isTransparent() && bg.getARGB() == kWhite.getARGB());
    if (isWhiteBg)
    {
        auto b = label.getLocalBounds();
        g.setColour(kGray);
        g.drawHorizontalLine(b.getY(),         (float)b.getX(), (float)b.getRight());
        g.drawVerticalLine  (b.getX(),         (float)b.getY(), (float)b.getBottom());
        g.setColour(kWhite);
        g.drawHorizontalLine(b.getBottom() - 1,(float)b.getX(), (float)b.getRight());
        g.drawVerticalLine  (b.getRight()  - 1,(float)b.getY(), (float)b.getBottom());
    }

    if (!label.isBeingEdited())
    {
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(label.getFont());
        auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
        g.drawFittedText(label.getText(), area, label.getJustificationType(),
                         juce::jmax(1, (int)((float)area.getHeight() / label.getFont().getHeight())),
                         label.getMinimumHorizontalScale());
    }
}

juce::Font Win95LookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions().withHeight(11.0f));
}

//==============================================================================
// LFOWaveformDisplay
//==============================================================================
LFOWaveformDisplay::LFOWaveformDisplay(LFOChannel& channel) : ch(channel)
{
    startTimerHz(30);
}

void LFOWaveformDisplay::timerCallback()
{
    repaint();
}

void LFOWaveformDisplay::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    draw3DBorder(g, b, false);
    auto inner = b.reduced(2);

    g.setColour(kWhite);
    g.fillRect(inner);

    float midY  = (float)inner.getCentreY();
    float halfH = (float)inner.getHeight() * 0.5f - 3.0f;
    int   pixW  = inner.getWidth();

    // Centre gridline
    g.setColour(juce::Colour(0xFFCCCCCC));
    g.drawHorizontalLine((int)midY, (float)inner.getX(), (float)inner.getRight());

    // Waveform — exactly one cycle across the full width
    juce::Path path;
    for (int x = 0; x < pixW; ++x)
    {
        double phase1 = double(x) / double(pixW);
        float  val    = ch.computeWaveValue(phase1);
        float  px     = (float)(inner.getX() + x);
        float  py     = midY - val * halfH;
        if (x == 0) path.startNewSubPath(px, py);
        else        path.lineTo(px, py);
    }
    g.setColour(ch.active ? kNavy : kGray);
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Red vertical playhead — sweeps left→right as phase goes 0→1
    float playX = (float)inner.getX() + (float)ch.phase * (float)pixW;
    g.setColour(juce::Colours::red);
    g.drawVerticalLine((int)playX, (float)inner.getY(), (float)inner.getBottom());

    // Red threshold line
    if (ch.noteEnabled && ch.active)
    {
        float thrY = midY - ch.peakThreshold * halfH;
        g.setColour(juce::Colours::red.withAlpha(0.5f));
        g.drawHorizontalLine((int)thrY, (float)inner.getX(), (float)inner.getRight());
    }
}

//==============================================================================
// LFOChannelPanel
//==============================================================================
LFOChannelPanel::LFOChannelPanel(LFOChannel& channel, int channelIndex)
    : ch(channel), index(channelIndex), waveDisplay(channel)
{
    setOpaque(true);
    addAndMakeVisible(waveDisplay);

    activeBtn.setToggleState(ch.active, juce::dontSendNotification);
    activeBtn.onClick = [this]{ ch.active = activeBtn.getToggleState(); };
    addAndMakeVisible(activeBtn);

    for (int i = 0; i < static_cast<int>(WaveShape::NumShapes); ++i)
        waveSelector.addItem(waveShapeName(static_cast<WaveShape>(i)), i + 1);
    waveSelector.setSelectedId(static_cast<int>(ch.waveShape) + 1, juce::dontSendNotification);
    waveSelector.onChange = [this]{ ch.waveShape = static_cast<WaveShape>(waveSelector.getSelectedId() - 1); };
    addAndMakeVisible(waveSelector);

    noteEnableBtn.setToggleState(ch.noteEnabled, juce::dontSendNotification);
    noteEnableBtn.onClick = [this]{ ch.noteEnabled = noteEnableBtn.getToggleState(); };
    addAndMakeVisible(noteEnableBtn);

    ccEnableBtn.setToggleState(ch.ccEnabled, juce::dontSendNotification);
    ccEnableBtn.onClick = [this]{ ch.ccEnabled = ccEnableBtn.getToggleState(); };
    addAndMakeVisible(ccEnableBtn);

    // ── Setup helpers ────────────────────────────────────────────────────────
    auto setupTitle = [this](juce::Label& lbl, const juce::String& text)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
        lbl.setColour(juce::Label::textColourId,       kBlack);
        lbl.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(lbl);
    };

    auto setupBox = [this](juce::Label& box, const juce::String& text, bool editable = true)
    {
        box.setText(text, juce::dontSendNotification);
        box.setJustificationType(juce::Justification::centred);
        box.setFont(juce::Font(juce::FontOptions().withName("Courier New").withHeight(12.0f)));
        box.setColour(juce::Label::backgroundColourId,            kWhite);
        box.setColour(juce::Label::textColourId,                  kBlack);
        box.setColour(juce::Label::outlineColourId,               juce::Colours::transparentBlack);
        box.setColour(juce::Label::backgroundWhenEditingColourId, kWhite);
        box.setColour(juce::Label::textWhenEditingColourId,       kBlack);
        box.setColour(juce::Label::outlineWhenEditingColourId,    kNavy);
        if (editable)
            box.setEditable(false, true, false);
        addAndMakeVisible(box);
    };

    // ── Row 1 ────────────────────────────────────────────────────────────────
    setupTitle(freqLabel,  "HZ");
    setupBox(freqBox, juce::String(ch.frequencyHz, 3));
    freqBox.onTextChange = [this]{
        float v = juce::jlimit(0.01f, 100.0f, freqBox.getText().getFloatValue());
        ch.frequencyHz = v;
        freqBox.setText(juce::String(v, 3), juce::dontSendNotification);
    };

    setupTitle(phaseLabel, "PHASE");
    setupBox(phaseBox, juce::String(juce::roundToInt(ch.phaseOffset * 360.0f)) + juce::String::charToString(0x00B0));
    phaseBox.onTextChange = [this]{
        float deg = juce::jlimit(0.0f, 360.0f,
            phaseBox.getText().retainCharacters("0123456789.-").getFloatValue());
        ch.phaseOffset = deg / 360.0f;
        phaseBox.setText(juce::String(juce::roundToInt(deg)) + juce::String::charToString(0x00B0),
                         juce::dontSendNotification);
    };

    setupTitle(noteLabel,  "NOTE");
    setupBox(noteBox, midiNoteName(ch.midiNote));
    noteBox.onTextChange = [this]{
        int n = parseNoteInput(noteBox.getText());
        if (n >= 0) { ch.midiNote = n; noteBox.setText(midiNoteName(n), juce::dontSendNotification); }
        else        { noteBox.setText(midiNoteName(ch.midiNote),         juce::dontSendNotification); }
    };

    setupTitle(chanLabel,  "CHAN");
    setupBox(chanBox, juce::String(ch.midiChannel));
    chanBox.onTextChange = [this]{
        int v = juce::jlimit(1, 16, chanBox.getText().getIntValue());
        ch.midiChannel = v;
        chanBox.setText(juce::String(v), juce::dontSendNotification);
    };

    setupTitle(threshLabel, "THR");
    setupBox(thrBox, juce::String(ch.peakThreshold, 2));
    thrBox.onTextChange = [this]{
        float v = juce::jlimit(0.0f, 1.0f, thrBox.getText().getFloatValue());
        ch.peakThreshold = v;
        thrBox.setText(juce::String(v, 2), juce::dontSendNotification);
    };

    // ── Row 2 ────────────────────────────────────────────────────────────────
    setupTitle(ccLabel,      "CC#");
    setupBox(ccBox, juce::String(ch.ccNumber));
    ccBox.onTextChange = [this]{
        int v = juce::jlimit(0, 127, ccBox.getText().getIntValue());
        ch.ccNumber = v;
        ccBox.setText(juce::String(v), juce::dontSendNotification);
    };

    setupTitle(velMinLabel,  "V MIN");
    setupBox(velMinBox, juce::String(ch.velocityMin));
    velMinBox.onTextChange = [this]{
        int v = juce::jlimit(0, 127, velMinBox.getText().getIntValue());
        ch.velocityMin = v;
        velMinBox.setText(juce::String(v), juce::dontSendNotification);
    };

    setupTitle(velMaxLabel,  "V MAX");
    setupBox(velMaxBox, juce::String(ch.velocityMax));
    velMaxBox.onTextChange = [this]{
        int v = juce::jlimit(0, 127, velMaxBox.getText().getIntValue());
        ch.velocityMax = v;
        velMaxBox.setText(juce::String(v), juce::dontSendNotification);
    };

    setupTitle(noteLenLabel, "LEN MS");
    setupBox(lenBox, juce::String(ch.noteLengthMs));
    lenBox.onTextChange = [this]{
        int v = juce::jlimit(1, 5000, lenBox.getText().getIntValue());
        ch.noteLengthMs = v;
        lenBox.setText(juce::String(v), juce::dontSendNotification);
    };

    setupTitle(velLabel, "VEL");
    setupBox(velBox, "--", false); // live display — updated by tick()
}

void LFOChannelPanel::tick()
{
    if (!velBox.isBeingEdited())
    {
        float norm = (ch.currentValue + 1.0f) * 0.5f;
        int   vel  = ch.velocityMin + static_cast<int>(norm * (float)(ch.velocityMax - ch.velocityMin));
        velBox.setText(juce::String(juce::jlimit(1, 127, vel)), juce::dontSendNotification);
    }
}

void LFOChannelPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    g.setColour(kSilver);
    g.fillRect(bounds);

    draw3DBorder(g, bounds, true);

    // Channel number badge on the left
    auto badge = bounds.reduced(2).removeFromLeft(19);
    g.setColour(ch.active ? kNavy : kGray);
    g.fillRect(badge);
    g.setColour(kWhite);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    g.drawText(juce::String(index + 1), badge, juce::Justification::centred);
}

void LFOChannelPanel::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromLeft(22); // badge

    // Header row
    auto header = bounds.removeFromTop(22);
    activeBtn.setBounds(header.removeFromLeft(40));
    waveSelector.setBounds(header.removeFromLeft(82).reduced(1));
    noteEnableBtn.setBounds(header.removeFromLeft(header.getWidth() / 2));
    ccEnableBtn.setBounds(header);

    // Waveform display
    waveDisplay.setBounds(bounds.removeFromTop(48).reduced(2));

    // Parameter rows
    auto row1 = bounds.removeFromTop(40);
    auto row2 = bounds;

    const int w1 = row1.getWidth() / 5;
    const int w2 = row2.getWidth() / 5;

    auto place = [](juce::Rectangle<int>& strip, int w, juce::Label& title, juce::Label& box)
    {
        auto cell = strip.removeFromLeft(w);
        title.setBounds(cell.removeFromTop(13));
        box.setBounds(cell.reduced(1));
    };

    place(row1, w1, freqLabel,    freqBox);
    place(row1, w1, phaseLabel,   phaseBox);
    place(row1, w1, noteLabel,    noteBox);
    place(row1, w1, chanLabel,    chanBox);
    place(row1, w1, threshLabel,  thrBox);

    place(row2, w2, ccLabel,      ccBox);
    place(row2, w2, velMinLabel,  velMinBox);
    place(row2, w2, velMaxLabel,  velMaxBox);
    place(row2, w2, noteLenLabel, lenBox);
    place(row2, w2, velLabel,     velBox);
}

//==============================================================================
// LFOSequencerEditor
//==============================================================================
LFOSequencerEditor::LFOSequencerEditor(LFOSequencerProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&win95LF);

    for (int i = 0; i < DISPLAY_CHANNELS; ++i)
    {
        auto* panel = new LFOChannelPanel(processorRef.getEngine().getChannel(i), i);
        channelPanels.add(panel);
        addAndMakeVisible(panel);
    }

    auto setupBtn = [this](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId,  kSilver);
        btn.setColour(juce::TextButton::textColourOffId, kBlack);
        addAndMakeVisible(btn);
    };

    setupBtn(syncBtn);
    syncBtn.onClick = [this]{ processorRef.getEngine().syncAll(); };

    setupBtn(spreadBtn);
    spreadBtn.onClick = [this]{ processorRef.getEngine().spreadPhases(); repaint(); };

    setupBtn(resetBtn);
    resetBtn.onClick = [this]{ processorRef.getEngine().resetAll(); };

    startTimerHz(10);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(580, 600, 1000, 900);
    setSize(680, 680);
}

LFOSequencerEditor::~LFOSequencerEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void LFOSequencerEditor::timerCallback()
{
    for (auto* panel : channelPanels)
        panel->tick();
    repaint();
}

void LFOSequencerEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Silver background
    g.fillAll(kSilver);

    // ── Title bar ────────────────────────────────────────────────────────────
    auto titleBar = bounds.removeFromTop(22);
    g.setColour(kNavy);
    g.fillRect(titleBar);

    // Decorative min/max/close boxes (right side)
    auto ctrlArea = titleBar;
    for (int i = 0; i < 3; ++i)
    {
        auto box = ctrlArea.removeFromRight(18).reduced(2, 3);
        g.setColour(kSilver);
        g.fillRect(box);
        draw3DBorder(g, box, true);
    }

    g.setColour(kWhite);
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.drawText("LFO Sequencer  -  Lever Studios",
               titleBar.withTrimmedLeft(6).reduced(0, 2),
               juce::Justification::centredLeft);

    // ── Menu bar ─────────────────────────────────────────────────────────────
    auto menuBar = bounds.removeFromTop(18);
    g.setColour(kSilver);
    g.fillRect(menuBar);
    g.setColour(kBlack);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.drawText("File    Edit    View    Help",
               menuBar.reduced(6, 0), juce::Justification::centredLeft);

    // Sunken border around the channel strip area
    auto channelArea = getLocalBounds().withTrimmedTop(40).withTrimmedBottom(48);
    draw3DBorder(g, channelArea.reduced(2), false);

    // ── Status bar ───────────────────────────────────────────────────────────
    auto statusBar = getLocalBounds().removeFromBottom(18);
    g.setColour(kSilver);
    g.fillRect(statusBar);
    draw3DBorder(g, statusBar, false);
    g.setColour(kBlack);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    g.drawText("4-CH LFO MIDI Sequencer", statusBar.reduced(4, 0), juce::Justification::centredLeft);
    g.drawText("MIDI out active",          statusBar.reduced(4, 0), juce::Justification::centredRight);
}

void LFOSequencerEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(40);    // title bar (22) + menu bar (18)
    bounds.removeFromBottom(18); // status bar
    bounds.reduce(4, 2);

    // Button row at bottom
    auto btnBar = bounds.removeFromBottom(30);
    const int btnW = btnBar.getWidth() / 3;
    syncBtn.setBounds(btnBar.removeFromLeft(btnW).reduced(4, 3));
    spreadBtn.setBounds(btnBar.removeFromLeft(btnW).reduced(4, 3));
    resetBtn.setBounds(btnBar.reduced(4, 3));

    // Channel panels — equal height
    const int panelH = bounds.getHeight() / DISPLAY_CHANNELS;
    for (auto* panel : channelPanels)
        panel->setBounds(bounds.removeFromTop(panelH));
}

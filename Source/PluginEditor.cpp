#include "PluginEditor.h"

//==============================================================================
NSeqArpKeysAudioProcessorEditor::NSeqArpKeysAudioProcessorEditor(NSeqArpKeysAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // ----- Keyboard -----------------------------------------------------------
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);

    // ----- Global: Meter Numerator --------------------------------------------
    meterNumeratorLabel.setText("Meter Num", juce::dontSendNotification);
    addAndMakeVisible(meterNumeratorLabel);

    addAndMakeVisible(meterNumeratorSlider);
    meterNumeratorSlider.setRange(1, 16, 1);
    meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get());
    meterNumeratorSlider.onValueChange = [this]
    {
        audioProcessor.getMeterNumerator()->setValueNotifyingHost(
            audioProcessor.getMeterNumerator()->convertTo0to1(
                static_cast<int>(meterNumeratorSlider.getValue())));
    };

    // ----- Global: Meter Denominator ------------------------------------------
    meterDenominatorLabel.setText("Meter Den", juce::dontSendNotification);
    addAndMakeVisible(meterDenominatorLabel);

    addAndMakeVisible(meterDenominatorSlider);
    meterDenominatorSlider.setRange(1, 16, 1);
    meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get());
    meterDenominatorSlider.onValueChange = [this]
    {
        audioProcessor.getMeterDenominator()->setValueNotifyingHost(
            audioProcessor.getMeterDenominator()->convertTo0to1(
                static_cast<int>(meterDenominatorSlider.getValue())));
    };

    // ----- Per-key: Channel ---------------------------------------------------
    channelLabel.setText("Channel", juce::dontSendNotification);
    addAndMakeVisible(channelLabel);

    addAndMakeVisible(channelSlider);
    channelSlider.setRange(1, 16, 1);
    channelSlider.onValueChange = [this]
    {
        audioProcessor.setChannelForKey(audioProcessor.getSelectedKey(),
                                        static_cast<int>(channelSlider.getValue()));
    };

    // ----- Per-key: Octave ----------------------------------------------------
    octaveLabel.setText("Octave", juce::dontSendNotification);
    addAndMakeVisible(octaveLabel);

    addAndMakeVisible(octaveSlider);
    octaveSlider.setRange(0, 10, 1);
    octaveSlider.onValueChange = [this]
    {
        audioProcessor.setOctaveForKey(audioProcessor.getSelectedKey(),
                                       static_cast<int>(octaveSlider.getValue()));
    };

    // ----- Per-key: Gate ------------------------------------------------------
    gateLabel.setText("Gate", juce::dontSendNotification);
    addAndMakeVisible(gateLabel);

    addAndMakeVisible(gateSlider);
    gateSlider.setRange(0.0, 1.0, 0.01);
    gateSlider.onValueChange = [this]
    {
        audioProcessor.setGateForKey(audioProcessor.getSelectedKey(),
                                     static_cast<float>(gateSlider.getValue()));
    };

    // ----- Per-key: Pattern ---------------------------------------------------
    patternLabel.setText("Pattern", juce::dontSendNotification);
    addAndMakeVisible(patternLabel);

    addAndMakeVisible(patternTextEditor);
    patternTextEditor.onTextChange = [this]
    {
        audioProcessor.setPatternForKey(audioProcessor.getSelectedKey(),
                                        patternTextEditor.getText().toStdString());
    };

    // ----- Per-key: Forte number ----------------------------------------------
    forteLabel.setText("Forte Set", juce::dontSendNotification);
    addAndMakeVisible(forteLabel);

    addAndMakeVisible(forteNumberSelector);
    {
        auto forteNumbers = Pcs12::getSortedForteNumbers();
        for (const auto& fn : forteNumbers)
            forteNumberSelector.addItem(fn, forteNumberSelector.getNumItems() + 1);
    }
    forteNumberSelector.onChange = [this]
    {
        audioProcessor.setForteForKey(audioProcessor.getSelectedKey(),
                                      forteNumberSelector.getText().toStdString());
    };

    // ----- Selected key label -------------------------------------------------
    selectedKeyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(selectedKeyLabel);

    // ----- Initial load -------------------------------------------------------
    setSize(640, 440);
    loadAssignmentForKey(audioProcessor.getSelectedKey());
}

NSeqArpKeysAudioProcessorEditor::~NSeqArpKeysAudioProcessorEditor()
{
    keyboardState.removeListener(this);
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
}

void NSeqArpKeysAudioProcessorEditor::resized()
{
    auto area  = getLocalBounds().reduced(8);
    int  w     = area.getWidth();
    int  lblW  = 90;
    int  rowH  = 26;

    keyboardComponent.setBounds(area.removeFromTop(80));
    area.removeFromTop(6);

    // Selected key display
    selectedKeyLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    // Two-column layout for global params
    auto globalRow = area.removeFromTop(rowH);
    meterNumeratorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterNumeratorSlider.setBounds(globalRow.removeFromLeft((w - 2 * lblW) / 2));
    meterDenominatorLabel .setBounds(globalRow.removeFromLeft(lblW));
    meterDenominatorSlider.setBounds(globalRow);

    area.removeFromTop(4);

    // Per-key controls (one per row, label + control)
    auto makeRow = [&](juce::Label& lbl, juce::Component& ctrl)
    {
        auto row = area.removeFromTop(rowH);
        lbl.setBounds(row.removeFromLeft(lblW));
        ctrl.setBounds(row);
        area.removeFromTop(2);
    };

    makeRow(channelLabel, channelSlider);
    makeRow(octaveLabel,  octaveSlider);
    makeRow(gateLabel,    gateSlider);
    makeRow(patternLabel, patternTextEditor);
    makeRow(forteLabel,   forteNumberSelector);
}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*,
                                                    int /*midiChannel*/,
                                                    int midiNoteNumber,
                                                    float /*velocity*/)
{
    audioProcessor.setSelectedKey(midiNoteNumber);
    loadAssignmentForKey(midiNoteNumber);

    juce::String name = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 4);
    selectedKeyLabel.setText("Selected key: " + name
                             + " (MIDI " + juce::String(midiNoteNumber) + ")",
                             juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*,
                                                     int, int, float) {}

//==============================================================================
void NSeqArpKeysAudioProcessorEditor::loadAssignmentForKey(int key)
{
    const auto& a = audioProcessor.getAssignmentForKey(key);

    // Use dontSendNotification where possible to avoid feedback loops.
    // For TextEditor, setText with false (= don't move cursor) does not
    // call onTextChange, so it is safe to call directly.
    patternTextEditor.setText(a.sequenceToString(), false);

    channelSlider.setValue(a.channel, juce::dontSendNotification);
    octaveSlider .setValue(a.octave,  juce::dontSendNotification);
    gateSlider   .setValue(static_cast<double>(a.gate), juce::dontSendNotification);

    if (a.forteString.empty())
        forteNumberSelector.setSelectedId(0, juce::dontSendNotification);
    else
        forteNumberSelector.setText(a.forteString, juce::dontSendNotification);

    juce::String name = juce::MidiMessage::getMidiNoteName(key, true, true, 4);
    selectedKeyLabel.setText("Selected key: " + name
                             + " (MIDI " + juce::String(key) + ")",
                             juce::dontSendNotification);
}

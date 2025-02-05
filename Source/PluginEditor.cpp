#include "PluginEditor.h"

NSeqArpKeysAudioProcessorEditor::NSeqArpKeysAudioProcessorEditor(NSeqArpKeysAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener(this);

    addAndMakeVisible(meterNumeratorSlider);
    meterNumeratorSlider.setRange(1, 16, 1);
    meterNumeratorSlider.setValue(audioProcessor.getMeterNumerator()->get());
    meterNumeratorSlider.onValueChange = [this] { audioProcessor.getMeterNumerator()->setValueNotifyingHost(meterNumeratorSlider.getValue()); };

    addAndMakeVisible(meterDenominatorSlider);
    meterDenominatorSlider.setRange(1, 16, 1);
    meterDenominatorSlider.setValue(audioProcessor.getMeterDenominator()->get());
    meterDenominatorSlider.onValueChange = [this] { audioProcessor.getMeterDenominator()->setValueNotifyingHost(meterDenominatorSlider.getValue()); };

    addAndMakeVisible(octaveNumberSlider);
    octaveNumberSlider.setRange(0, 10, 1);
    octaveNumberSlider.setValue(audioProcessor.getOctaveNumber()->get());
    octaveNumberSlider.onValueChange = [this] { audioProcessor.getOctaveNumber()->setValueNotifyingHost(octaveNumberSlider.getValue()); };

    addAndMakeVisible(gateParameterSlider);
    gateParameterSlider.setRange(0.0, 1.0, 0.01);
    gateParameterSlider.setValue(audioProcessor.getGateParameter()->get());
    gateParameterSlider.onValueChange = [this] { audioProcessor.getGateParameter()->setValueNotifyingHost(gateParameterSlider.getValue()); };

    addAndMakeVisible(channelParameterSlider);
    channelParameterSlider.setRange(1, 16, 1);
    channelParameterSlider.setValue(audioProcessor.getChannelParameter()->get());
    channelParameterSlider.onValueChange = [this] { audioProcessor.getChannelParameter()->setValueNotifyingHost(channelParameterSlider.getValue()); };

    addAndMakeVisible(patternTextEditor);
    patternTextEditor.onTextChange = [this] { audioProcessor.getKeyPatterns()[audioProcessor.getSelectedKeyParameter()->get()] = audioProcessor.parsePattern(patternTextEditor.getText()); };

    addAndMakeVisible(forteNumberSelector);
    auto forteNumbers = Pcs12::getSortedForteNumbers();
    for (const auto& forteNumber : forteNumbers)
    {
        forteNumberSelector.addItem(forteNumber, forteNumberSelector.getNumItems() + 1);
    }
    forteNumberSelector.onChange = [this] { audioProcessor.getKeyForteNumbers()[audioProcessor.getSelectedKeyParameter()->get()] = Pcs12::parseForte(forteNumberSelector.getText().toStdString()); };

    setSize(600, 400);
    updateParametersForKey(audioProcessor.getSelectedKeyParameter()->get());
}

NSeqArpKeysAudioProcessorEditor::~NSeqArpKeysAudioProcessorEditor()
{
    keyboardState.removeListener(this);
}

void NSeqArpKeysAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void NSeqArpKeysAudioProcessorEditor::resized()
{
    keyboardComponent.setBounds(10, 10, getWidth() - 20, 80);
    meterNumeratorSlider.setBounds(10, 100, getWidth() - 20, 20);
    meterDenominatorSlider.setBounds(10, 130, getWidth() - 20, 20);
    octaveNumberSlider.setBounds(10, 160, getWidth() - 20, 20);
    gateParameterSlider.setBounds(10, 190, getWidth() - 20, 20);
    channelParameterSlider.setBounds(10, 220, getWidth() - 20, 20);
    patternTextEditor.setBounds(10, 250, getWidth() - 20, 20);
    forteNumberSelector.setBounds(10, 280, getWidth() - 20, 20);
}

void NSeqArpKeysAudioProcessorEditor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    updateSelectedKey(midiNoteNumber);
}

void NSeqArpKeysAudioProcessorEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) {}

void NSeqArpKeysAudioProcessorEditor::updateParametersForKey(int key)
{
    std::ostringstream oss;
    const auto& pattern = audioProcessor.getKeyPatterns()[key];
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        if (i != 0)
            oss << " ";
        oss << pattern[i];
    }
    patternTextEditor.setText(oss.str()); 
    channelParameterSlider.setValue(audioProcessor.getKeyChannels()[key]);
    forteNumberSelector.setText(audioProcessor.getKeyForteNumbers()[key].toForteNumberString(), juce::dontSendNotification);
}

void NSeqArpKeysAudioProcessorEditor::updateSelectedKey(int key)
{
    audioProcessor.getSelectedKeyParameter()->setValueNotifyingHost(key);
    updateParametersForKey(key);
}

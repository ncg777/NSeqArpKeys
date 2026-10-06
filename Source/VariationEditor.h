#pragma once

#include "PluginProcessor.h"
#include "Domain/PatternVariations.h"
#include <functional>

class PatternBitGrid : public juce::Component
{
public:
    PatternBitGrid();
    void setAssignment(const KeyAssignment& assignment, int firstStep);
    void setPlayhead(int step);
    void resized() override;
private:
    class Canvas : public juce::Component
    {
    public:
        KeyAssignment assignment;
        int firstStep = 0, playhead = -1, low = 0, high = 7;
        void paint(juce::Graphics&) override;
    } canvas;
    juce::Viewport viewport;
};

// A snapshot-based preview: changing controls never edits the live assignment.
class VariationEditor : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    VariationEditor(NSeqArpKeysAudioProcessor& processor, int sourceKey);
    ~VariationEditor() override;
    void resized() override;
    void stopAudition();
    std::function<void(const std::vector<std::pair<int, KeyAssignment>>&)> onApply;
    std::function<bool(const std::vector<KeyAssignment>&, juce::String&)> onSaveFamily;
    std::function<void()> onClose;
private:
    void rebuild();
    void showSelection();
    void audition(bool original);
    void timerCallback() override;
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged(int row) override;
    juce::String variantName(const PatternVariations::Variation&) const;
    KeyAssignment namedAssignment(const PatternVariations::Variation&) const;

    NSeqArpKeysAudioProcessor& processor;
    const int sourceKey;
    const uint64_t restoreRevision;
    const KeyAssignment source;
    PatternVariations::Result result;
    int selected = 0;
    bool auditioning = false, auditionOriginal = false, auditionStarted = false;
    juce::ComboBox operationSelector, polynomialSelector;
    juce::ToggleButton keepRestsButton, inverseButton;
    juce::Slider cubicSlider, quadraticSlider, linearSlider, constantSlider;
    juce::Slider firstKeySlider, keyCountSlider, firstApplicationSlider, strideSlider, gridStartSlider;
    juce::Label titleLabel, operationLabel, polynomialLabel, statusLabel;
    juce::Label cubicLabel, quadraticLabel, linearLabel, constantLabel;
    juce::Label firstKeyLabel, keyCountLabel, firstApplicationLabel, strideLabel;
    juce::Label sourceLabel, variationLabel, gridStartLabel, sequenceLabel;
    juce::Label familyNameLabel, familyTagsLabel, messageLabel;
    juce::TextEditor familyNameEditor, familyTagsEditor;
    juce::ListBox variationsList;
    PatternBitGrid sourceGrid, variationGrid;
    juce::TextButton originalButton, variationButton, stopButton, applyButton, saveFamilyButton, closeButton;
    bool settingCoefficients = false;
};

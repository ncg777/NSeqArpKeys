#include "VariationEditor.h"

namespace
{
juce::String keyName(int key) { return juce::MidiMessage::getMidiNoteName(key, true, true, 4); }
}

PatternBitGrid::PatternBitGrid()
{
    viewport.setViewedComponent(&canvas, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(viewport);
}

void PatternBitGrid::setAssignment(const KeyAssignment& assignment, int firstStep)
{
    canvas.assignment = assignment;
    auto& values = canvas.assignment.sequence;
    if (assignment.reverse) std::reverse(values.begin(), values.end());
    if (!values.empty())
    {
        const int size = static_cast<int>(values.size());
        const int shift = ((assignment.rotation % size) + size) % size;
        std::rotate(values.rbegin(), values.rbegin() + shift, values.rend());
    }
    canvas.firstStep = firstStep;
    const auto bounds = PatternVariations::occupiedBounds(assignment);
    canvas.low = bounds.second < bounds.first ? 0 : bounds.first;
    canvas.high = bounds.second < bounds.first ? std::min(7, PatternVariations::cellCount(assignment) - 1) : bounds.second;
    resized();
    canvas.repaint();
}

void PatternBitGrid::setPlayhead(int step)
{
    if (canvas.playhead != step) { canvas.playhead = step; canvas.repaint(); }
}

void PatternBitGrid::resized()
{
    viewport.setBounds(getLocalBounds());
    canvas.setSize(std::max(100, getWidth() - viewport.getScrollBarThickness()),
                   std::max(getHeight(), 26 + (canvas.high - canvas.low + 1) * 16));
}

void PatternBitGrid::Canvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff142330));
    const float labelWidth = 40.0f;
    const float columnWidth = (getWidth() - labelWidth - 2.0f) / 16.0f;
    const bool melodic = assignment.mode == KeyAssignment::Mode::melodic;
    const auto tint = juce::Colour::fromString("ff" + juce::String(assignment.colour).trimCharactersAtStart("#"));
    g.setFont(juce::FontOptions(11.0f));
    for (int column = 0; column < 16; ++column)
    {
        const int step = firstStep + column;
        if (step >= static_cast<int>(assignment.sequence.size())) break;
        const float x = labelWidth + column * columnWidth;
        if (step == playhead)
        {
            g.setColour(juce::Colour(0xfff4bd68).withAlpha(0.28f));
            g.fillRect(x, 0.0f, columnWidth, static_cast<float>(getHeight()));
        }
        g.setColour(juce::Colour(0xffc8d9e2));
        g.drawText(juce::String(step + 1), static_cast<int>(x), 1, static_cast<int>(columnWidth), 20,
                   juce::Justification::centred);
    }
    for (int cell = high; cell >= low; --cell)
    {
        const int y = 24 + (high - cell) * 16;
        g.setColour(juce::Colour(0xffc8d9e2));
        g.drawText((melodic ? "b" : "L") + juce::String(melodic ? cell : cell + 1), 2, y, 34, 16,
                   juce::Justification::centredRight);
        for (int column = 0; column < 16; ++column)
        {
            const int step = firstStep + column;
            if (step >= static_cast<int>(assignment.sequence.size())) break;
            const auto& value = assignment.sequence[static_cast<size_t>(step)];
            const auto level = PatternVariations::cellValue(assignment, value, cell);
            const uint32_t maximum = melodic ? 1u : (1u << std::clamp(assignment.drumVelocityBits, 1, 7)) - 1;
            g.setColour(level == 0 ? juce::Colour(0xff263d4c)
                                  : (melodic && value.negative ? juce::Colour(0xffb99bff) : tint)
                                    .withAlpha(0.35f + 0.65f * static_cast<float>(level) / maximum));
            g.fillRoundedRectangle(labelWidth + column * columnWidth + 1, static_cast<float>(y + 2),
                                   columnWidth - 2, 12.0f, 2.0f);
        }
    }
}

VariationEditor::VariationEditor(NSeqArpKeysAudioProcessor& p, int key)
    : processor(p), sourceKey(key), restoreRevision(p.getStateRestoreRevision()),
      source(p.getAssignmentForKey(key)), variationsList("Variations", this)
{
    auto label = [this](juce::Label& component, const juce::String& text)
    {
        component.setText(text, juce::dontSendNotification);
        component.setFont(juce::FontOptions(13.0f));
        addAndMakeVisible(component);
    };
    label(titleLabel, "Generate variations from " + keyName(key) + " - "
          + (source.name.empty() ? "Untitled pattern" : juce::String(source.name)));
    titleLabel.setFont(juce::FontOptions(17.0f));
    label(operationLabel, "Operator");
    label(polynomialLabel, "Polynomial");
    label(cubicLabel, "Cubic (x^3)"); label(quadraticLabel, "Quadratic (x^2)");
    label(linearLabel, "Linear (x)"); label(constantLabel, "Constant");
    label(firstKeyLabel, "First key"); label(keyCountLabel, "Number of keys");
    label(firstApplicationLabel, "Starting application"); label(strideLabel, "Applications between keys");
    label(sourceLabel, "Original - bit positions (purple = negative direction)");
    label(variationLabel, "Selected variation");
    label(gridStartLabel, "First visible step"); label(sequenceLabel, "");
    label(statusLabel, ""); label(messageLabel, "");
    label(familyNameLabel, "Family name"); label(familyTagsLabel, "Tags");
    if (source.mode == KeyAssignment::Mode::rhythmic)
        sourceLabel.setText("Original - drum lanes (brightness = velocity)", juce::dontSendNotification);
    operationSelector.addItem("Polynomial mapping", 1);
    operationSelector.addItem("Vertical flip", 2);
    operationSelector.setSelectedId(1, juce::dontSendNotification);
    polynomialSelector.addItem("Quadratic", 1);
    polynomialSelector.addItem("Cubic", 2);
    polynomialSelector.addItem("Rotation", 3);
    polynomialSelector.addItem("Custom", 4);
    polynomialSelector.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(operationSelector); addAndMakeVisible(polynomialSelector);
    keepRestsButton.setButtonText("Keep zero steps in place");
    keepRestsButton.setTooltip("Map only nonzero positions and keep zeros fixed. The modulus is the number of nonzero positions.");
    inverseButton.setButtonText("Inverse permutation");
    inverseButton.setTooltip("Available when the mapping uses every source position exactly once.");
    inverseButton.setComponentID("variation-inverse");
    statusLabel.setComponentID("variation-status");
    addAndMakeVisible(keepRestsButton); addAndMakeVisible(inverseButton);
    auto slider = [this](juce::Slider& component, double minimum, double maximum, double value)
    {
        component.setRange(minimum, maximum, 1);
        component.setSliderStyle(juce::Slider::LinearHorizontal);
        component.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 24);
        component.setScrollWheelEnabled(false);
        component.setValue(value, juce::dontSendNotification);
        addAndMakeVisible(component);
        component.onValueChange = [this] { rebuild(); };
    };
    slider(cubicSlider, -64, 64, 0); slider(quadraticSlider, -64, 64, 2);
    slider(linearSlider, -64, 64, 1); slider(constantSlider, -64, 64, 0);
    slider(firstKeySlider, 0, 127, key); slider(keyCountSlider, 1, 128 - key, std::min(8, 128 - key));
    slider(firstApplicationSlider, 0, 65536, 0); slider(strideSlider, 1, 4096, 1);
    firstKeySlider.setComponentID("variation-first-key");
    keyCountSlider.setComponentID("variation-key-count");
    firstApplicationSlider.setComponentID("variation-first-application");
    strideSlider.setComponentID("variation-stride");
    operationSelector.setComponentID("variation-operation");
    linearSlider.setComponentID("variation-linear");
    quadraticSlider.setComponentID("variation-quadratic");
    firstKeySlider.textFromValueFunction = [](double value) { return keyName(static_cast<int>(value)); };
    firstKeySlider.valueFromTextFunction = [](const juce::String& text)
    {
        const auto input = text.trim();
        for (int midi = 0; midi < 128; ++midi)
            if (keyName(midi).equalsIgnoreCase(input)) return static_cast<double>(midi);
        return static_cast<double>(input.getIntValue());
    };
    firstKeySlider.updateText();
    slider(gridStartSlider, 1, std::max(1, static_cast<int>(source.sequence.size()) - 15), 1);
    gridStartSlider.onValueChange = [this] { showSelection(); };
    auto coefficientChanged = [this]
    {
        if (settingCoefficients) return;
        polynomialSelector.setSelectedId(4, juce::dontSendNotification);
        rebuild();
    };
    for (auto* coefficient : { &cubicSlider, &quadraticSlider, &linearSlider, &constantSlider })
        coefficient->onValueChange = coefficientChanged;
    polynomialSelector.onChange = [this]
    {
        const int preset = polynomialSelector.getSelectedId();
        if (preset == 4) return;
        settingCoefficients = true;
        cubicSlider.setValue(preset == 2 ? 2 : 0, juce::dontSendNotification);
        quadraticSlider.setValue(preset == 1 ? 2 : 0, juce::dontSendNotification);
        linearSlider.setValue(preset == 2 ? 3 : 1, juce::dontSendNotification);
        constantSlider.setValue(preset == 3 ? 1 : 0, juce::dontSendNotification);
        settingCoefficients = false;
        rebuild();
    };
    operationSelector.onChange = [this] { rebuild(); };
    keepRestsButton.onClick = [this] { rebuild(); };
    inverseButton.onClick = [this] { rebuild(); };
    firstKeySlider.onValueChange = [this]
    {
        keyCountSlider.setRange(1, 128 - static_cast<int>(firstKeySlider.getValue()), 1);
        rebuild();
    };
    for (auto* editor : { &familyNameEditor, &familyTagsEditor }) addAndMakeVisible(*editor);
    familyNameEditor.setInputRestrictions(64);
    familyTagsEditor.setInputRestrictions(160);
    familyNameEditor.setText(source.name.empty() ? "Variations" : juce::String(source.name).substring(0, 64));
    familyTagsEditor.setText(juce::String(source.tags).substring(0, 160));
    familyNameEditor.onTextChange = [this] { variationsList.repaint(); showSelection(); };
    familyTagsEditor.onTextChange = [this] { messageLabel.setText({}, juce::dontSendNotification); };
    for (auto* button : { &originalButton, &variationButton, &stopButton, &applyButton, &saveFamilyButton, &closeButton })
        addAndMakeVisible(*button);
    originalButton.setButtonText("Audition original");
    variationButton.setButtonText("Audition variation");
    stopButton.setButtonText("Stop audition");
    applyButton.setButtonText("Assign variations");
    saveFamilyButton.setButtonText("Save family to bank");
    closeButton.setButtonText("Back to Editor");
    originalButton.onClick = [this] { audition(true); };
    variationButton.onClick = [this] { audition(false); };
    stopButton.onClick = [this] { stopAudition(); };
    closeButton.onClick = [this] { stopAudition(); if (onClose) onClose(); };
    applyButton.onClick = [this]
    {
        if (!result.valid() || !onApply) return;
        if (processor.getStateRestoreRevision() != restoreRevision)
        { messageLabel.setText("Project restored. Reopen variations to use its current pattern.", juce::dontSendNotification); return; }
        std::vector<std::pair<int, KeyAssignment>> assignments;
        for (const auto& variation : result.variations) assignments.emplace_back(variation.key, namedAssignment(variation));
        stopAudition();
        onApply(assignments);
    };
    saveFamilyButton.onClick = [this]
    {
        if (!result.valid() || !onSaveFamily) return;
        if (processor.getStateRestoreRevision() != restoreRevision)
        { messageLabel.setText("Project restored. Reopen variations to use its current pattern.", juce::dontSendNotification); return; }
        std::vector<KeyAssignment> assignments;
        for (const auto& variation : result.variations) assignments.push_back(namedAssignment(variation));
        juce::String error;
        if (onSaveFamily(assignments, error))
            messageLabel.setText("Saved " + juce::String(assignments.size()) + " patterns to the user bank.", juce::dontSendNotification);
        else messageLabel.setText(error, juce::dontSendNotification);
    };
    variationsList.setRowHeight(27);
    addAndMakeVisible(variationsList);
    addAndMakeVisible(sourceGrid); addAndMakeVisible(variationGrid);
    rebuild();
    startTimerHz(20);
}

VariationEditor::~VariationEditor()
{
    stopTimer();
    stopAudition();
    variationsList.setModel(nullptr);
}

void VariationEditor::resized()
{
    auto area = getLocalBounds().reduced(6);
    auto row = [&](int height = 28) { auto r = area.removeFromTop(height); area.removeFromTop(4); return r; };
    titleLabel.setBounds(row());
    auto operators = row();
    operationLabel.setBounds(operators.removeFromLeft(64));
    operationSelector.setBounds(operators.removeFromLeft(190));
    operators.removeFromLeft(10);
    keepRestsButton.setBounds(operators.removeFromLeft(215));
    inverseButton.setBounds(operators);
    auto polynomial = row();
    polynomialLabel.setBounds(polynomial.removeFromLeft(80));
    polynomialSelector.setBounds(polynomial.removeFromLeft(170));
    const int column = (area.getWidth() - 18) / 4;
    auto labels = row(20), coefficients = row();
    for (const auto pair : { std::make_pair(&cubicLabel, &cubicSlider), std::make_pair(&quadraticLabel, &quadraticSlider),
                             std::make_pair(&linearLabel, &linearSlider), std::make_pair(&constantLabel, &constantSlider) })
    {
        pair.first->setBounds(labels.removeFromLeft(column)); labels.removeFromLeft(6);
        pair.second->setBounds(coefficients.removeFromLeft(column)); coefficients.removeFromLeft(6);
    }
    statusLabel.setBounds(row(34));
    labels = row(20);
    auto destinations = row();
    for (const auto pair : { std::make_pair(&firstKeyLabel, &firstKeySlider), std::make_pair(&keyCountLabel, &keyCountSlider),
                             std::make_pair(&firstApplicationLabel, &firstApplicationSlider), std::make_pair(&strideLabel, &strideSlider) })
    {
        pair.first->setBounds(labels.removeFromLeft(column)); labels.removeFromLeft(6);
        pair.second->setBounds(destinations.removeFromLeft(column)); destinations.removeFromLeft(6);
    }
    variationsList.setBounds(row(130));
    auto gridTitles = row(22);
    const int half = (area.getWidth() - 12) / 2;
    sourceLabel.setBounds(gridTitles.removeFromLeft(half)); gridTitles.removeFromLeft(12);
    variationLabel.setBounds(gridTitles);
    auto grids = row(128);
    sourceGrid.setBounds(grids.removeFromLeft(half)); grids.removeFromLeft(12);
    variationGrid.setBounds(grids);
    auto visibleSteps = row();
    gridStartLabel.setBounds(visibleSteps.removeFromLeft(135));
    gridStartSlider.setBounds(visibleSteps);
    sequenceLabel.setBounds(row(24));
    auto auditions = row();
    originalButton.setBounds(auditions.removeFromLeft(155)); auditions.removeFromLeft(6);
    variationButton.setBounds(auditions.removeFromLeft(155)); auditions.removeFromLeft(6);
    stopButton.setBounds(auditions.removeFromLeft(140));
    auto name = row(); familyNameLabel.setBounds(name.removeFromLeft(100)); familyNameEditor.setBounds(name);
    auto tags = row(); familyTagsLabel.setBounds(tags.removeFromLeft(100)); familyTagsEditor.setBounds(tags);
    auto actions = row();
    applyButton.setBounds(actions.removeFromLeft(160)); actions.removeFromLeft(6);
    saveFamilyButton.setBounds(actions.removeFromLeft(180)); actions.removeFromLeft(6);
    closeButton.setBounds(actions.removeFromLeft(150));
    messageLabel.setBounds(row(32));
}

void VariationEditor::rebuild()
{
    stopAudition();
    if (processor.getStateRestoreRevision() != restoreRevision)
    {
        applyButton.setEnabled(false); saveFamilyButton.setEnabled(false);
        originalButton.setEnabled(false); variationButton.setEnabled(false);
        statusLabel.setText("Project restored. Reopen variations to use the current source.", juce::dontSendNotification);
        return;
    }
    PatternVariations::Options options;
    options.operation = operationSelector.getSelectedId() == 2 ? PatternVariations::Operation::verticalFlip
                                                              : PatternVariations::Operation::polynomial;
    options.polynomial = { static_cast<int>(cubicSlider.getValue()), static_cast<int>(quadraticSlider.getValue()),
                           static_cast<int>(linearSlider.getValue()), static_cast<int>(constantSlider.getValue()) };
    options.keepRests = keepRestsButton.getToggleState(); options.inverse = inverseButton.getToggleState();
    options.firstKey = static_cast<int>(firstKeySlider.getValue());
    options.keyCount = static_cast<int>(keyCountSlider.getValue());
    options.firstApplication = static_cast<int>(firstApplicationSlider.getValue());
    options.applicationsBetweenKeys = static_cast<int>(strideSlider.getValue());
    result = PatternVariations::build(source, options);
    const bool polynomial = options.operation == PatternVariations::Operation::polynomial;
    if (polynomial && options.inverse && !result.permutation)
    {
        inverseButton.setToggleState(false, juce::dontSendNotification);
        options.inverse = false;
        result = PatternVariations::build(source, options);
    }
    polynomialSelector.setEnabled(polynomial); keepRestsButton.setEnabled(polynomial);
    inverseButton.setEnabled(polynomial && result.valid() && result.permutation);
    for (auto* coefficient : { &cubicSlider, &quadraticSlider, &linearSlider, &constantSlider }) coefficient->setEnabled(polynomial);
    applyButton.setEnabled(result.valid()); saveFamilyButton.setEnabled(result.valid()); variationButton.setEnabled(result.valid());
    auto status = result.valid() ? juce::String(result.distinctPatterns) + " distinct patterns across "
                               + juce::String(result.variations.size()) + " keys. " : juce::String(result.error);
    if (result.valid())
    {
        if (polynomial && !result.permutation)
        {
            status += "Repeats/omits source steps. From application " + juce::String(result.transientApplications)
                    + (result.order ? ", cycle length " + juce::String(*result.order) + "."
                                    : ", cycle length exceeds 64-bit counting.");
        }
        else
        {
            if (polynomial) status += "Permutation. ";
            status += result.order ? "Order repeats after " + juce::String(*result.order) + " applications."
                                   : "Permutation order exceeds 64-bit counting.";
        }
    }
    statusLabel.setColour(juce::Label::textColourId, result.valid() ? juce::Colour(0xffc8d9e2) : juce::Colour(0xffff9292));
    statusLabel.setText(status, juce::dontSendNotification); statusLabel.setTooltip(status);
    messageLabel.setText({}, juce::dontSendNotification);
    variationsList.updateContent();
    selected = std::clamp(selected, 0, std::max(0, getNumRows() - 1));
    variationsList.selectRow(result.valid() ? selected : -1);
    showSelection();
}

juce::String VariationEditor::variantName(const PatternVariations::Variation& variation) const
{
    const auto name = familyNameEditor.getText().trim();
    return (name.isEmpty() ? juce::String("Variations") : name).substring(0, 60)
         + " #" + juce::String(variation.application);
}

KeyAssignment VariationEditor::namedAssignment(const PatternVariations::Variation& variation) const
{
    auto assignment = variation.assignment;
    assignment.name = variantName(variation).substring(0, 80).toStdString();
    assignment.tags = (familyTagsEditor.getText().trim() + " variation application-" + juce::String(variation.application))
                        .trim().substring(0, 200).toStdString();
    assignment.rootKey = sourceKey;
    return assignment;
}

int VariationEditor::getNumRows() { return static_cast<int>(result.variations.size()); }

void VariationEditor::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool isSelected)
{
    if (row < 0 || row >= getNumRows()) return;
    const auto& variation = result.variations[static_cast<size_t>(row)];
    g.fillAll(isSelected ? juce::Colour(0xff294b57) : juce::Colour(0xff172431));
    g.setColour(juce::Colour(0xffedf6fb)); g.setFont(juce::FontOptions(13.0f));
    auto label = keyName(variation.key) + "   Application " + juce::String(variation.application)
               + "   " + variantName(variation);
    if (variation.repeatsKey >= 0) label += "   (repeats " + keyName(variation.repeatsKey) + ")";
    g.drawText(label, 6, 0, width - 12, height, juce::Justification::centredLeft, true);
}

void VariationEditor::selectedRowsChanged(int row)
{
    if (row < 0) return;
    const bool restart = auditioning && !auditionOriginal;
    selected = row;
    showSelection();
    if (restart) audition(false);
}

void VariationEditor::showSelection()
{
    const int first = static_cast<int>(gridStartSlider.getValue()) - 1;
    sourceGrid.setAssignment(source, first);
    if (selected >= 0 && selected < getNumRows())
    {
        const auto& variation = result.variations[static_cast<size_t>(selected)];
        variationGrid.setAssignment(variation.assignment, first);
        variationLabel.setText(keyName(variation.key) + " - " + variantName(variation), juce::dontSendNotification);
        const auto text = juce::String(variation.assignment.sequenceToString());
        sequenceLabel.setText("Sequence: " + text, juce::dontSendNotification);
        sequenceLabel.setTooltip(text);
    }
    else
    {
        auto empty = source; empty.sequence.clear();
        variationGrid.setAssignment(empty, first);
        variationLabel.setText("No valid variation", juce::dontSendNotification);
        sequenceLabel.setText({}, juce::dontSendNotification);
    }
}

void VariationEditor::audition(bool original)
{
    if (processor.getStateRestoreRevision() != restoreRevision) return;
    if (!original && (!result.valid() || selected < 0 || selected >= getNumRows())) return;
    stopAudition();
    processor.auditionPattern(sourceKey, original ? source : result.variations[static_cast<size_t>(selected)].assignment);
    auditioning = true; auditionOriginal = original; auditionStarted = false;
    stopButton.setEnabled(true);
}

void VariationEditor::stopAudition()
{
    if (auditioning) processor.requestStopKey(sourceKey);
    auditioning = false; auditionStarted = false;
    sourceGrid.setPlayhead(-1); variationGrid.setPlayhead(-1);
    stopButton.setEnabled(false);
}

void VariationEditor::timerCallback()
{
    if (processor.getStateRestoreRevision() != restoreRevision)
    {
        stopAudition(); applyButton.setEnabled(false); saveFamilyButton.setEnabled(false);
        originalButton.setEnabled(false); variationButton.setEnabled(false);
        statusLabel.setText("Project restored. Reopen variations to use the current source.", juce::dontSendNotification);
        return;
    }
    const int step = auditioning ? processor.getPlaybackStep(sourceKey) : -1;
    if (step >= 0) auditionStarted = true;
    if (step < 0 && auditionStarted) { stopAudition(); return; }
    if (step >= 0 && (step < static_cast<int>(gridStartSlider.getValue()) - 1
                     || step >= static_cast<int>(gridStartSlider.getValue()) + 15))
        gridStartSlider.setValue((step / 16) * 16 + 1);
    sourceGrid.setPlayhead(auditionOriginal ? step : -1);
    variationGrid.setPlayhead(auditionOriginal ? -1 : step);
}

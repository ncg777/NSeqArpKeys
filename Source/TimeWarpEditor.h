#pragma once

#include <JuceHeader.h>
#include "Domain/TimeWarp.h"

class TimeWarpEditor : public juce::Component
{
public:
    TimeWarpEditor()
    {
        setComponentID("time-warp-editor");
        enabled.setButtonText("Time warp");
        lengths.setButtonText("Warp note lengths");
        lengths.setTooltip("Warp both ends inside the onset's chunk, using their absolute distance (minimum 0.5 ms). Turn off to preserve Gate + Fixed Steps durations.");
        for (auto* button : { &enabled, &lengths })
        {
            addAndMakeVisible(button);
            button->onClick = [this] { changed(); };
        }
        enabled.setComponentID("time-warp-enabled");
        lengths.setComponentID("time-warp-lengths");
        int id = 1;
        std::string group;
        for (const auto& entry : TimeWarp::curves())
        {
            if (group != entry.group) { group = entry.group; curve.addSectionHeading(group); }
            curve.addItem(entry.name, id++);
        }
        curve.addItem("Custom expression", id);
        for (auto* combo : { &curve, &quantize })
        {
            addAndMakeVisible(combo);
            combo->onChange = [this] { changed(); };
        }
        curve.setComponentID("time-warp-curve");
        quantize.setComponentID("time-warp-quantize");
        quantize.addItem("Off", 1);
        quantize.addItem("1 / step", 2); quantize.addItem("2 / step", 3);
        quantize.addItem("4 / step", 4); quantize.addItem("8 / step", 5);
        for (auto* slider : { &amount, &repeats })
        {
            slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 46, 24);
            slider->setScrollWheelEnabled(false);
            addAndMakeVisible(slider);
            slider->onValueChange = [this] { changed(); };
        }
        amount.setRange(0, 100, 1); amount.setTextValueSuffix("%");
        repeats.setRange(1, 64, 1);
        amount.setComponentID("time-warp-amount"); repeats.setComponentID("time-warp-repeats");
        repeats.setTooltip("Split the pattern into 1-64 equal chunks; apply the same curve locally in each chunk.");
        for (auto* label : { &curveLabel, &amountLabel, &repeatsLabel, &quantizeLabel, &expressionLabel, &status }) addAndMakeVisible(label);
        curveLabel.setText("Curve", juce::dontSendNotification);
        amountLabel.setText("Amount", juce::dontSendNotification);
        repeatsLabel.setText("Repeats", juce::dontSendNotification);
        quantizeLabel.setText("Quantize", juce::dontSendNotification);
        expressionLabel.setText("Expression", juce::dontSendNotification);
        status.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(expression); addAndMakeVisible(preview);
        expression.setInputRestrictions(512);
        expression.setComponentID("time-warp-expression");
        expression.setTooltip("T, PI, E; + - * / % ^; sin/cos/tan, asin/acos/atan, sqrt, pow, abs, min/max, floor/ceil/round, integer, exp/log/sign, seq, wform, stowform, saw_wform, revsaw_wform, triangle_wform. Optional Y = prefix. Examples: T^2 or T + sin(2*PI*T)/8.");
        expression.onTextChange = [this] { changed(); };
        setSettings({});
    }

    std::function<void(const TimeWarp::Settings&)> onChange;

    void setSettings(const TimeWarp::Settings& settings)
    {
        enabled.setToggleState(settings.enabled, juce::dontSendNotification);
        lengths.setToggleState(settings.noteLengths, juce::dontSendNotification);
        int selected = static_cast<int>(TimeWarp::curves().size()) + 1;
        for (size_t i = 0; i < TimeWarp::curves().size(); ++i)
            if (settings.curve == TimeWarp::curves()[i].name) selected = static_cast<int>(i) + 1;
        curve.setSelectedId(selected, juce::dontSendNotification);
        amount.setValue(settings.amount, juce::dontSendNotification);
        repeats.setValue(settings.repeats, juce::dontSendNotification);
        const int divisions[] { 0, 1, 2, 4, 8 };
        for (int i = 0; i < 5; ++i) if (settings.quantize == divisions[i]) quantize.setSelectedId(i + 1, juce::dontSendNotification);
        expression.setText(settings.expression, false);
        update(settings);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6);
        preview.setBounds(area.removeFromRight(150).reduced(4));
        area.removeFromRight(8);
        auto row = [&](int height = 28) { auto r = area.removeFromTop(height); area.removeFromTop(3); return r; };
        auto top = row();
        enabled.setBounds(top.removeFromLeft(124));
        curveLabel.setBounds(top.removeFromLeft(48)); curve.setBounds(top);
        auto controls = row();
        const int third = controls.getWidth() / 3;
        auto a = controls.removeFromLeft(third), r = controls.removeFromLeft(third);
        amountLabel.setBounds(a.removeFromLeft(60)); amount.setBounds(a.reduced(2, 0));
        repeatsLabel.setBounds(r.removeFromLeft(60)); repeats.setBounds(r.reduced(2, 0));
        quantizeLabel.setBounds(controls.removeFromLeft(66)); quantize.setBounds(controls);
        auto custom = row();
        expressionLabel.setBounds(custom.removeFromLeft(80)); expression.setBounds(custom);
        lengths.setBounds(row());
        status.setBounds(area);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff132633));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    }
private:
    class Preview : public juce::Component
    {
    public:
        TimeWarp::Resolution resolution { [](double t) { return t; }, {} };
        double mix = 0;
        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(8);
            g.setColour(juce::Colour(0xff304b5a)); g.drawRect(bounds);
            g.drawLine(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getY(), 1);
            juce::Path path;
            for (int i = 0; i <= 128; ++i)
            {
                const auto t = i / 128.0;
                const auto x = bounds.getX() + bounds.getWidth() * static_cast<float>(t);
                const auto y = bounds.getBottom() - bounds.getHeight() * static_cast<float>(TimeWarp::normalized(t, resolution, mix));
                if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
            }
            g.setColour(juce::Colour(0xff62d6c6)); g.strokePath(path, juce::PathStrokeType(2));
        }
    } preview;
    TimeWarp::Settings settingsFromControls() const
    {
        TimeWarp::Settings settings;
        settings.enabled = enabled.getToggleState(); settings.noteLengths = lengths.getToggleState();
        const auto index = curve.getSelectedId() - 1;
        settings.curve = index >= 0 && index < static_cast<int>(TimeWarp::curves().size()) ? TimeWarp::curves()[static_cast<size_t>(index)].name : "custom";
        settings.expression = expression.getText().toStdString();
        settings.amount = amount.getValue(); settings.repeats = static_cast<int>(repeats.getValue());
        const int divisions[] { 0, 1, 2, 4, 8 };
        settings.quantize = divisions[juce::jlimit(0, 4, quantize.getSelectedId() - 1)];
        return settings;
    }
    bool update(const TimeWarp::Settings& settings)
    {
        preview.resolution = TimeWarp::resolve(settings);
        preview.mix = settings.enabled ? settings.amount / 100 : 0;
        preview.repaint();
        const bool custom = settings.curve == "custom";
        expression.setEnabled(custom);
        const bool valid = preview.resolution.error.empty();
        if (valid) expression.removeColour(juce::TextEditor::outlineColourId);
        else expression.setColour(juce::TextEditor::outlineColourId, juce::Colours::orangered);
        status.setColour(juce::Label::textColourId, valid ? juce::Colour(0xffb7cbd5) : juce::Colour(0xffff9292));
        status.setText(valid ? "T = position 0-1 in each chunk. Try T^2 or 1-T."
                             : "Not applied: " + juce::String(preview.resolution.error), juce::dontSendNotification);
        status.setTooltip(status.getText());
        return valid;
    }
    void changed()
    {
        const auto settings = settingsFromControls();
        const auto valid = update(settings);
        if ((valid || !settings.enabled) && onChange) onChange(settings);
    }
    juce::ToggleButton enabled, lengths;
    juce::ComboBox curve, quantize;
    juce::Slider amount, repeats;
    juce::TextEditor expression;
    juce::Label curveLabel, amountLabel, repeatsLabel, quantizeLabel, expressionLabel, status;
};

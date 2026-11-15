#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

inline constexpr int kBaseWidth = 1634, kBaseHeight = 252;

class VCASSLLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class EditableKnob final : public juce::Slider
{
public:
    std::function<void()> editValue;
    void mouseDoubleClick(const juce::MouseEvent&) override { if (editValue) editValue(); }
};

class FaceplateComponent final : public juce::Component, private juce::Timer
{
public:
    explicit FaceplateComponent(VCASSLProcessor&);
    ~FaceplateComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void paintMeter(juce::Graphics&);
    VCASSLProcessor& processor;
    VCASSLLookAndFeel look;
    std::array<EditableKnob,5> knobs;
    std::array<juce::Label,5> valueLabels;
    std::array<juce::ToggleButton,4> switches;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<std::unique_ptr<SA>,5> knobAttachments;
    std::array<std::unique_ptr<BA>,4> switchAttachments;
    float shownGr = 0, shownOut = -100;
    juce::TooltipWindow tooltips { this, 500 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaceplateComponent)
};

class VCASSLEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VCASSLEditor(VCASSLProcessor&);
    void resized() override;
private:
    VCASSLProcessor& processor;
    FaceplateComponent faceplate;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VCASSLEditor)
};

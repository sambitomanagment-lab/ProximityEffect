#pragma once

#include "PluginProcessor.h"
#include "ComicLookAndFeel.h"

// The guy on the office chair: drag him toward the mic to crank the proximity effect.
class ChairGuy : public juce::Component
{
public:
    ChairGuy (juce::RangedAudioParameter& distanceParam);

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void setAngry (bool shouldBeAngry);
    void tick(); // animation step, called from the editor timer

    // Horizontal travel of the character, in local coordinates.
    static constexpr float closeX = 165.0f, farX = 400.0f;

private:
    float centreX() const noexcept;
    juce::Rectangle<float> bodyBounds() const noexcept;
    void drawChair (juce::Graphics&, juce::Point<float> base, float squash) const;
    void drawPerson (juce::Graphics&, juce::Point<float> seat, float puff) const;

    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float distance = 60.0f;
    float dragStartDistance = 0.0f;
    bool angry = false;
    juce::Point<float> shake;
    juce::Random random;
};

class ProximityEffectEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ProximityEffectEditor (ProximityEffectProcessor&);
    ~ProximityEffectEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int baseWidth = 960, baseHeight = 640;

private:
    // Everything is laid out on a fixed 960x640 canvas and scaled as a whole.
    class Panel : public juce::Component
    {
    public:
        explicit Panel (ProximityEffectProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;

        ProximityEffectProcessor& processor;
        ChairGuy guy;

        juce::Slider inputGain, lowEnd, pattern, output, gainBoost, mix;
        juce::ToggleButton hpf, air, angry, pad, polarity;

    private:
        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
        std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
        std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

        void drawKnobLabels (juce::Graphics&, const juce::Slider&, const juce::String& title,
                             const juce::String& minText, const juce::String& midText, const juce::String& maxText,
                             const juce::String& badgeText);
        void drawScene (juce::Graphics&);
        void drawBottomBar (juce::Graphics&);
        void drawPatternSelector (juce::Graphics&);
    };

    void timerCallback() override;

    ProximityEffectProcessor& processor;
    ComicLookAndFeel lookAndFeel;
    Panel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProximityEffectEditor)
};

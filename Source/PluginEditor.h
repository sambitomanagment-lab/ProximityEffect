#pragma once

#include "PluginProcessor.h"
#include "ComicLookAndFeel.h"
#include "CharacterArtwork.h"

// The guy on the office chair: drag him toward the mic to crank the proximity effect.
class ChairGuy : public juce::Component
{
public:
    ChairGuy (juce::RangedAudioParameter& distanceParam, juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    // Animation step, called from the editor timer. Reads the plugin's parameters and eases
    // the pose toward them, so the character follows knobs and automation without jumps.
    void tick (float secondsSinceLastTick);

    // Chair pole travel (minus 20 px), in local coordinates: 0 cm at closeX, 100 cm at farX.
    static constexpr float closeX = 190.0f, farX = 440.0f;
    static constexpr float artworkScale = 0.5f, artworkFarInset = 26.0f;

private:
    float centreX() const noexcept;
    juce::Rectangle<float> bodyBounds() const noexcept;
    juce::AffineTransform artworkTransform() const noexcept;
    void drawSteam (juce::Graphics&, const juce::AffineTransform&) const;
    void drawAngerVein (juce::Graphics&, const juce::AffineTransform&) const;
    void drawBubble (juce::Graphics&, juce::Point<float> headTop) const;
    void paintVector (juce::Graphics&);
    void drawChair (juce::Graphics&, juce::Point<float> base, float squash) const;
    void drawPerson (juce::Graphics&, juce::Point<float> seat, float puff) const;

    juce::RangedAudioParameter& param;
    juce::AudioProcessorValueTreeState& apvts;
    juce::ParameterAttachment attachment;
    float distance = 60.0f, shownDistance = 60.0f;
    float dragStartDistance = 0.0f;
    bool angry = false;
    juce::Point<float> shake;
    double time = 0.0;

    CharacterArtwork artwork;
    CharacterArtwork::Pose pose;
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

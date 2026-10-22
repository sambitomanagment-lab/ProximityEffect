#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The character from the original mockup (Assets/character.png), animated by warping
// and tinting the artwork itself: nothing is redrawn, so the illustration keeps its look.
class CharacterArtwork
{
public:
    // Everything the drawing reacts to, all smoothed and in 0..1.
    struct Pose
    {
        float squash = 0.0f;  // chair compression and forward lean (physical closeness)
        float puff = 0.0f;    // face inflation
        float flush = 0.0f;   // redness of the face
        float anger = 0.0f;   // angry expression
        float extreme = 0.0f; // tremble + steam

        bool closeTo (const Pose& o) const noexcept;
    };

    CharacterArtwork();

    bool isValid() const noexcept { return source.isValid(); }
    int getWidth() const noexcept { return source.getWidth(); }
    int getHeight() const noexcept { return source.getHeight(); }

    // Rendered artwork for this pose, at `scale` of the artwork's size (match it to the
    // on-screen size to save work). Cached: re-rendered only when the pose or scale changes.
    const juce::Image& render (const Pose&, float scale = 1.0f);

    // Where a point of the original artwork ends up after the pose's deformation.
    juce::Point<float> mapForward (juce::Point<float> artworkPoint, const Pose&) const;

    // Landmarks in artwork pixels.
    static constexpr float poleX = 345.0f, floorY = 668.0f;
    static inline const juce::Point<float> headCentre { 305.0f, 150.0f }, veinPoint { 344.0f, 102.0f };
    static inline const juce::Point<float> steamLeft { 262.0f, 96.0f }, steamRight { 352.0f, 96.0f };
    static inline const juce::Point<float> headTop { 305.0f, 72.0f };
    static inline const juce::Rectangle<float> figureBounds { 18.0f, 70.0f, 482.0f, 618.0f };

private:
    juce::Point<float> mapInverse (float x, float y, const Pose&) const;
    void buildSkinMask();

    juce::Image source, rendered;
    std::vector<float> skinMask;
    Pose renderedPose;
    float renderedScale = 1.0f;
    bool hasRendered = false;
};

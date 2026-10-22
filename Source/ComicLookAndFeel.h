#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Palette
{
    inline const juce::Colour cream    { 0xffefe2c4 };
    inline const juce::Colour creamDark{ 0xffd8c7a0 };
    inline const juce::Colour ink      { 0xff1e1a16 };
    inline const juce::Colour sage     { 0xff8fa672 };
    inline const juce::Colour knob     { 0xff6e7f55 };
    inline const juce::Colour bar      { 0xff6f8460 };
    inline const juce::Colour brick    { 0xffb23a2b };
    inline const juce::Colour mustard  { 0xffd9a520 };
    inline const juce::Colour metal    { 0xffb9b3a6 };
}

namespace Comic
{
    // Cooper Black when the system has it (common on Windows/macOS with Office), else a heavy fallback.
    inline juce::Font font (float height)
    {
        static const juce::String name = []
        {
            const auto names = juce::Font::findAllTypefaceNames();
            for (auto candidate : { "Cooper Black", "Cooper Std", "Arial Black", "Impact" })
                if (names.contains (candidate))
                    return juce::String (candidate);
            return juce::String();
        }();

        auto options = juce::FontOptions().withHeight (height).withStyle ("Bold");
        if (name.isNotEmpty())
            options = options.withName (name);
        return juce::Font (options);
    }

    // Cartoon text: thick ink outline, offset shadow, coloured fill.
    inline void outlinedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                              float height, juce::Colour fill, float outline = 4.0f,
                              juce::Justification just = juce::Justification::centred)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addFittedText (font (height), text, area.getX(), area.getY(), area.getWidth(), area.getHeight(), just, 1);
        juce::Path p;
        glyphs.createPath (p);

        const juce::PathStrokeType stroke (outline, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.setColour (Palette::ink);
        g.fillPath (p, juce::AffineTransform::translation (outline * 0.6f, outline * 0.8f));
        g.strokePath (p, stroke, juce::AffineTransform::translation (outline * 0.6f, outline * 0.8f));
        g.strokePath (p, stroke);
        g.setColour (fill);
        g.fillPath (p);
    }

    inline void plainText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, float height,
                           juce::Justification just = juce::Justification::centred, juce::Colour colour = Palette::ink)
    {
        g.setColour (colour);
        g.setFont (font (height));
        g.drawFittedText (text, area.toNearestInt(), just, 2, 0.8f);
    }

    // Little pill-shaped caption under a knob ("MORE BOOM").
    inline void badge (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area)
    {
        g.setColour (Palette::creamDark);
        g.fillRoundedRectangle (area, area.getHeight() * 0.3f);
        g.setColour (Palette::ink);
        g.drawRoundedRectangle (area, area.getHeight() * 0.3f, 1.8f);
        plainText (g, text, area.reduced (4, 1), area.getHeight() * 0.62f);
    }

    // Polar pattern r(θ) = |a + (1 - a) cos θ|, front facing up.
    inline void polarIcon (juce::Graphics& g, juce::Point<float> c, float radius, float a, juce::Colour colour)
    {
        juce::Path p;
        for (int i = 0; i <= 120; ++i)
        {
            const float t = juce::MathConstants<float>::twoPi * (float) i / 120.0f;
            const float r = radius * std::abs (a + (1.0f - a) * std::cos (t));
            const juce::Point<float> pt (c.x + r * std::sin (t), c.y - r * std::cos (t));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (2.2f));
    }

    inline void screw (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (Palette::metal);
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (Palette::ink);
        g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 1.6f);
        g.drawLine (c.x - r * 0.6f, c.y - r * 0.6f, c.x + r * 0.6f, c.y + r * 0.6f, 2.0f);
    }

    inline void angryFace (juce::Graphics& g, juce::Point<float> c, float r, bool lit)
    {
        const auto box = juce::Rectangle<float> (r * 2, r * 2).withCentre (c);
        g.setColour (lit ? Palette::brick : Palette::mustard);
        g.fillEllipse (box);
        g.setColour (Palette::ink);
        g.drawEllipse (box, 2.0f);
        g.drawLine (c.x - r * 0.6f, c.y - r * 0.45f, c.x - r * 0.12f, c.y - r * 0.15f, 2.5f);
        g.drawLine (c.x + r * 0.6f, c.y - r * 0.45f, c.x + r * 0.12f, c.y - r * 0.15f, 2.5f);
        g.fillEllipse (c.x - r * 0.42f, c.y - r * 0.05f, r * 0.2f, r * 0.22f);
        g.fillEllipse (c.x + r * 0.22f, c.y - r * 0.05f, r * 0.2f, r * 0.22f);
        juce::Path mouth;
        mouth.startNewSubPath (c.x - r * 0.4f, c.y + r * 0.55f);
        mouth.quadraticTo (c.x, c.y + r * 0.2f, c.x + r * 0.4f, c.y + r * 0.55f);
        g.strokePath (mouth, juce::PathStrokeType (2.2f));
    }
}

class ComicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        using namespace juce;
        const auto bounds = Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const auto c = bounds.getCentre();
        const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float body = radius * 0.78f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const bool pointerStyle = slider.getProperties()["pointer"];

        // Scale ticks.
        g.setColour (Palette::ink);
        const int ticks = pointerStyle ? 5 : 9;
        for (int i = 0; i < ticks; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (ticks - 1);
            const auto p = c.getPointOnCircumference (radius * 0.95f, a);
            g.fillEllipse (Rectangle<float> (4.5f, 4.5f).withCentre (p));
        }

        // Hard cartoon shadow.
        g.setColour (Palette::ink.withAlpha (0.85f));
        g.fillEllipse (Rectangle<float> (body * 2, body * 2).withCentre (c.translated (4, 5)));

        if (pointerStyle)
        {
            // Chicken-head selector.
            g.setColour (Palette::metal.darker (0.3f));
            g.fillEllipse (Rectangle<float> (body * 2, body * 2).withCentre (c));
            g.setColour (Palette::ink);
            g.drawEllipse (Rectangle<float> (body * 2, body * 2).withCentre (c), 3.0f);

            Path head;
            head.addRoundedRectangle (-body * 0.28f, -body * 1.05f, body * 0.56f, body * 2.1f, body * 0.25f);
            head.applyTransform (AffineTransform::rotation (angle).translated (c));
            g.setColour (Palette::ink.brighter (0.15f));
            g.fillPath (head);
            g.setColour (Palette::ink);
            g.strokePath (head, PathStrokeType (2.5f));
            const auto tip = c.getPointOnCircumference (body * 0.75f, angle);
            g.setColour (Palette::cream);
            g.drawLine (Line<float> (c.getPointOnCircumference (body * 0.25f, angle), tip), 4.0f);
            return;
        }

        // Knurled outer ring.
        g.setColour (Palette::ink.brighter (0.1f));
        g.fillEllipse (Rectangle<float> (body * 2, body * 2).withCentre (c));
        g.setColour (Palette::ink.brighter (0.35f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / 36.0f;
            g.drawLine (Line<float> (c.getPointOnCircumference (body * 0.8f, a), c.getPointOnCircumference (body * 0.97f, a)), 1.5f);
        }

        // Green cap with a soft highlight.
        const float cap = body * 0.74f;
        ColourGradient grad (Palette::knob.brighter (0.35f), c.x - cap * 0.5f, c.y - cap * 0.6f,
                             Palette::knob.darker (0.35f), c.x + cap * 0.6f, c.y + cap * 0.7f, true);
        g.setGradientFill (grad);
        g.fillEllipse (Rectangle<float> (cap * 2, cap * 2).withCentre (c));
        g.setColour (Palette::ink);
        g.drawEllipse (Rectangle<float> (cap * 2, cap * 2).withCentre (c), 2.5f);
        g.drawEllipse (Rectangle<float> (body * 2, body * 2).withCentre (c), 3.0f);

        // Pointer.
        const auto from = c.getPointOnCircumference (cap * 0.2f, angle);
        const auto to = c.getPointOnCircumference (body * 0.92f, angle);
        g.setColour (Palette::ink);
        g.drawLine (Line<float> (from, to), 8.0f);
        g.setColour (Palette::cream);
        g.drawLine (Line<float> (from, to), 4.5f);

        // LED that lights up when the knob is pushed hard.
        const auto ledCentre = c.translated (radius * 0.82f, -radius * 0.82f);
        const float hot = jlimit (0.0f, 1.0f, (pos - 0.7f) / 0.3f);
        if (hot > 0.0f)
        {
            g.setColour (Palette::brick.brighter (0.6f).withAlpha (0.45f * hot));
            g.fillEllipse (Rectangle<float> (22.0f, 22.0f).withCentre (ledCentre));
        }
        g.setColour (Palette::brick.darker (0.6f).interpolatedWith (Colour (0xffff3b22), hot));
        g.fillEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (ledCentre));
        g.setColour (Palette::ink);
        g.drawEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (ledCentre), 1.8f);
    }

    // Guitar-style bat toggle with ON/OFF tags.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool) override
    {
        using namespace juce;
        const auto r = b.getLocalBounds().toFloat().reduced (2.0f);
        const bool on = b.getToggleState();
        const bool red = b.getProperties()["red"];

        auto plate = r.reduced (0, 15.0f);
        g.setColour (Palette::ink);
        g.fillRoundedRectangle (plate.translated (3, 4), 7.0f);
        g.setColour (red ? Colour (0xffd9cbb0) : Palette::creamDark);
        g.fillRoundedRectangle (plate, 7.0f);
        g.setColour (Palette::ink);
        g.drawRoundedRectangle (plate, 7.0f, 2.5f);

        auto tag = [&] (Rectangle<float> area, const String& text, bool lit)
        {
            g.setColour (lit ? (red || text == "ON" ? Palette::brick : Palette::ink.brighter (0.2f)) : Palette::creamDark.darker (0.15f));
            g.fillRoundedRectangle (area, 4.0f);
            g.setColour (Palette::ink);
            g.drawRoundedRectangle (area, 4.0f, 1.8f);
            Comic::plainText (g, text, area, area.getHeight() * 0.8f, Justification::centred,
                              lit ? Palette::cream : Palette::ink.withAlpha (0.6f));
        };
        tag (Rectangle<float> (r.getWidth() * 0.66f, 15.0f).withCentre ({ r.getCentreX(), r.getY() + 7.5f }), "ON", on);
        tag (Rectangle<float> (r.getWidth() * 0.66f, 15.0f).withCentre ({ r.getCentreX(), r.getBottom() - 7.5f }), "OFF", ! on);

        // Hex nut + lever.
        const auto c = plate.getCentre();
        g.setColour (Palette::metal);
        g.fillEllipse (Rectangle<float> (22.0f, 22.0f).withCentre (c));
        g.setColour (Palette::ink);
        g.drawEllipse (Rectangle<float> (22.0f, 22.0f).withCentre (c), 2.0f);

        const auto tip = c.translated (on ? -3.0f : 3.0f, on ? -plate.getHeight() * 0.3f : plate.getHeight() * 0.3f);
        g.setColour (Palette::ink);
        g.drawLine (Line<float> (c, tip), 9.0f);
        g.setColour (highlighted ? Colours::white : Palette::metal.brighter (0.3f));
        g.drawLine (Line<float> (c, tip), 5.0f);
        g.setColour (Palette::ink);
        g.fillEllipse (Rectangle<float> (16.0f, 16.0f).withCentre (tip));
        g.setColour (Palette::metal.brighter (0.4f));
        g.fillEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (tip.translated (-1, -1)));
    }
};

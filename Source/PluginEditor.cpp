#include "PluginEditor.h"

using namespace juce;

namespace Layout
{
    constexpr int knob = 96, patternKnob = 80;
    const Point<int> inputGain { 112, 142 }, lowEnd { 112, 296 }, pattern { 100, 480 };
    const Point<int> output { 848, 142 }, gainBoost { 848, 296 }, mix { 848, 455 };
    const Rectangle<int> guy { 300, 140, 500, 380 };
    constexpr float railY = 515.0f, railLeft = 365.0f, railRight = 795.0f;
    const Rectangle<float> bottomBar { 18.0f, 548.0f, 924.0f, 76.0f };
    constexpr int toggleW = 46, toggleH = 72, toggleY = 550, slotW = 146, firstSlotX = 28;

    // Closeness 0..1 from a distance in cm, same curve as the DSP.
    inline float closeness (float cm) { return std::pow (1.0f - jlimit (0.0f, 100.0f, cm) / 100.0f, 1.6f); }
}

//==============================================================================
ChairGuy::ChairGuy (RangedAudioParameter& p)
    : param (p),
      attachment (p, [this] (float v) { distance = v; repaint(); })
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::DraggingHandCursor);
    setRepaintsOnMouseActivity (false);
}

float ChairGuy::centreX() const noexcept
{
    return closeX + (farX - closeX) * distance / 100.0f;
}

Rectangle<float> ChairGuy::bodyBounds() const noexcept
{
    return { centreX() - 100.0f, 30.0f, 200.0f, (float) getHeight() - 30.0f };
}

bool ChairGuy::hitTest (int x, int y)
{
    return bodyBounds().contains ((float) x, (float) y);
}

void ChairGuy::mouseDown (const MouseEvent&)
{
    dragStartDistance = distance;
    attachment.beginGesture();
}

void ChairGuy::mouseDrag (const MouseEvent& e)
{
    const float cmPerPixel = 100.0f / (farX - closeX);
    attachment.setValueAsPartOfGesture (jlimit (0.0f, 100.0f, dragStartDistance + (float) e.getDistanceFromDragStartX() * cmPerPixel));
}

void ChairGuy::mouseUp (const MouseEvent&)
{
    attachment.endGesture();
}

void ChairGuy::mouseDoubleClick (const MouseEvent&)
{
    attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void ChairGuy::setAngry (bool shouldBeAngry)
{
    if (angry != shouldBeAngry)
    {
        angry = shouldBeAngry;
        shake = {};
        repaint();
    }
}

void ChairGuy::tick()
{
    if (! angry)
        return;

    const float intensity = 1.5f + 3.0f * Layout::closeness (distance);
    shake = { (random.nextFloat() - 0.5f) * 2.0f * intensity, (random.nextFloat() - 0.5f) * intensity };
    repaint();
}

void ChairGuy::drawChair (Graphics& g, Point<float> base, float squash) const
{
    const float seatY = base.y - 112.0f * (1.0f - 0.22f * squash);
    const auto stroke = PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded);

    // Star base and wheels.
    g.setColour (Palette::ink);
    g.drawLine (base.x - 58, base.y - 14, base.x + 58, base.y - 14, 9.0f);
    g.drawLine (base.x - 30, base.y - 20, base.x + 30, base.y - 20, 7.0f);
    for (float wx : { -58.0f, -30.0f, 30.0f, 58.0f })
        g.fillEllipse (base.x + wx - 8, base.y - 16, 16, 16);

    // Gas pole, squashed when he leans in.
    g.setColour (Palette::metal.darker (0.4f));
    g.fillRect (Rectangle<float> (base.x - 5, seatY, 10, base.y - 20 - seatY));
    g.setColour (Palette::ink);
    g.drawRect (Rectangle<float> (base.x - 5, seatY, 10, base.y - 20 - seatY), 2.0f);

    // Backrest.
    Path back;
    back.addRoundedRectangle (base.x + 38, seatY - 128, 34, 120, 14);
    back.applyTransform (AffineTransform::rotation (0.12f, base.x + 55, seatY));
    g.setColour (Palette::ink);
    g.drawLine (base.x + 30, seatY - 2, base.x + 52, seatY - 40, 7.0f);
    g.setColour (Palette::brick);
    g.fillPath (back);
    g.setColour (Palette::ink);
    g.strokePath (back, stroke);

    // Seat cushion, wider when squashed.
    const float seatW = 118.0f + 22.0f * squash, seatH = 24.0f * (1.0f - 0.3f * squash);
    const auto seat = Rectangle<float> (seatW, seatH).withCentre ({ base.x + 6, seatY - seatH * 0.4f });
    g.setColour (Palette::brick.darker (0.15f));
    g.fillRoundedRectangle (seat, seatH * 0.5f);
    g.setColour (Palette::ink);
    g.drawRoundedRectangle (seat, seatH * 0.5f, 3.0f);
}

void ChairGuy::drawPerson (Graphics& g, Point<float> seat, float puff) const
{
    const auto stroke = PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded);
    const float floorY = (float) getHeight() - 6.0f;
    const Colour skin (angry ? Colour (0xffe08a6c) : Colour (0xffe9b48f));
    const Colour hoodie (0xff23211f), khaki (0xffcdb489);

    // Legs: thighs forward (toward the mic), shins down.
    auto limb = [&] (Point<float> a, Point<float> b, float w, Colour c)
    {
        Path p;
        p.startNewSubPath (a);
        p.lineTo (b);
        g.setColour (Palette::ink);
        g.strokePath (p, PathStrokeType (w + 5, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (c);
        g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
    };

    // Hoodie torso.
    Path torso;
    torso.addRoundedRectangle (seat.x - 40, seat.y - 140, 96, 130, 30);
    g.setColour (hoodie);
    g.fillPath (torso);
    g.setColour (Palette::ink);
    g.strokePath (torso, stroke);
    Comic::plainText (g, "PROX", { seat.x - 34, seat.y - 128, 84, 26 }, 22.0f, Justification::centred, Palette::cream);
    Comic::plainText (g, "EST. 1931", { seat.x - 30, seat.y - 104, 76, 14 }, 11.0f, Justification::centred, Palette::sage);

    for (float side : { 1.0f, 0.0f })
    {
        const float off = side * 14.0f;
        const Point<float> hip (seat.x + 10 - off, seat.y - 18);
        const Point<float> knee (seat.x - 62 - off, seat.y - 24);
        const Point<float> ankle (seat.x - 66 - off, floorY - 16);
        limb (hip, knee, 28, khaki.darker (side * 0.12f));
        limb (knee, ankle, 24, khaki.darker (side * 0.12f));

        // Red sneaker with white sole.
        auto shoe = Rectangle<float> (ankle.x - 30, floorY - 22, 44, 18);
        g.setColour (Palette::brick);
        g.fillRoundedRectangle (shoe, 8);
        g.setColour (Colours::white);
        g.fillRect (shoe.removeFromBottom (5).reduced (2, 0));
        g.setColour (Palette::ink);
        g.drawRoundedRectangle (Rectangle<float> (ankle.x - 30, floorY - 22, 44, 18), 8, 2.5f);
    }

    // Arms resting on the knees (or fists up when angry).
    if (angry)
    {
        limb ({ seat.x - 30, seat.y - 120 }, { seat.x - 66, seat.y - 160 }, 20, hoodie);
        limb ({ seat.x + 46, seat.y - 120 }, { seat.x + 70, seat.y - 165 }, 20, hoodie);
        for (auto p : { Point<float> (seat.x - 70, seat.y - 168), Point<float> (seat.x + 72, seat.y - 173) })
        {
            g.setColour (skin);
            g.fillEllipse (Rectangle<float> (20, 20).withCentre (p));
            g.setColour (Palette::ink);
            g.drawEllipse (Rectangle<float> (20, 20).withCentre (p), 2.5f);
        }
    }
    else
    {
        limb ({ seat.x - 34, seat.y - 122 }, { seat.x - 46, seat.y - 70 }, 20, hoodie);
        limb ({ seat.x - 46, seat.y - 70 }, { seat.x - 58, seat.y - 34 }, 20, hoodie);
        limb ({ seat.x + 50, seat.y - 118 }, { seat.x + 50, seat.y - 62 }, 20, hoodie);
        limb ({ seat.x + 50, seat.y - 62 }, { seat.x - 20, seat.y - 36 }, 20, hoodie);
        for (auto p : { Point<float> (seat.x - 62, seat.y - 30), Point<float> (seat.x - 24, seat.y - 32) })
        {
            g.setColour (skin);
            g.fillEllipse (Rectangle<float> (20, 16).withCentre (p));
            g.setColour (Palette::ink);
            g.drawEllipse (Rectangle<float> (20, 16).withCentre (p), 2.5f);
        }
    }

    // Head: inflates like a fisheye lens the closer he gets.
    const Point<float> hc (seat.x + 4, seat.y - 172);
    const float rx = 34.0f * puff * 1.08f, ry = 38.0f * (1.0f + (puff - 1.0f) * 0.6f);
    const auto head = Rectangle<float> (rx * 2, ry * 2).withCentre (hc);

    g.setColour (skin);
    g.fillEllipse (head.withSizeKeepingCentre (rx * 2 + 14, ry * 0.6f).translated (0, 2)); // ears
    g.setColour (Palette::ink);
    g.drawEllipse (head.withSizeKeepingCentre (rx * 2 + 14, ry * 0.6f).translated (0, 2), 2.5f);
    g.setColour (skin);
    g.fillEllipse (head);

    // Beard (lower half) and short hair (top cap).
    {
        Graphics::ScopedSaveState s (g);
        Path clip;
        clip.addEllipse (head);
        g.reduceClipRegion (clip);
        g.setColour (Colour (0xff3b2a20));
        g.fillEllipse (head.withTrimmedTop (ry * 1.05f).expanded (6, 0).translated (0, 6));
        g.fillRect (head.withHeight (ry * 0.55f));
        g.setColour (skin);
        g.fillEllipse (Rectangle<float> (rx * 0.9f, ry * 0.42f).withCentre (hc.translated (0, ry * 0.42f)));
    }
    g.setColour (Palette::ink);
    g.drawEllipse (head, 3.0f);

    // Rosy cheeks grow with proximity.
    const float blush = jlimit (0.0f, 1.0f, (puff - 1.0f) * 3.0f);
    g.setColour (Palette::brick.withAlpha (0.35f * blush));
    g.fillEllipse (Rectangle<float> (rx * 0.45f, ry * 0.28f).withCentre (hc.translated (-rx * 0.55f, ry * 0.12f)));
    g.fillEllipse (Rectangle<float> (rx * 0.45f, ry * 0.28f).withCentre (hc.translated (rx * 0.55f, ry * 0.12f)));

    // Eyes and brows.
    const float eyeY = hc.y - ry * 0.12f, eyeDx = rx * 0.38f;
    g.setColour (Palette::ink);
    for (float s : { -1.0f, 1.0f })
    {
        g.fillEllipse (Rectangle<float> (7.0f, angry ? 5.0f : 8.0f).withCentre ({ hc.x + s * eyeDx, eyeY }));
        const Point<float> inner (hc.x + s * eyeDx * 0.35f, eyeY - (angry ? 6.0f : 13.0f));
        const Point<float> outer (hc.x + s * eyeDx * 1.45f, eyeY - (angry ? 16.0f : 14.0f));
        g.setColour (angry ? Palette::brick.darker (0.2f) : Palette::ink);
        g.drawLine (Line<float> (inner, outer), angry ? 6.0f : 4.0f);
        g.setColour (Palette::ink);
    }

    // Mouth.
    const Point<float> mc (hc.x, hc.y + ry * 0.42f);
    Path mouth;
    if (angry)
    {
        mouth.addEllipse (Rectangle<float> (rx * 0.62f, ry * 0.3f).withCentre (mc));
        g.setColour (Colour (0xff6b1d18));
        g.fillPath (mouth);
        g.setColour (Colours::white);
        g.fillRect (Rectangle<float> (rx * 0.4f, 3.5f).withCentre (mc.translated (0, -ry * 0.1f)));
        g.setColour (Palette::ink);
        g.strokePath (mouth, PathStrokeType (2.5f));
    }
    else
    {
        mouth.startNewSubPath (mc.x - rx * 0.36f, mc.y - 3);
        mouth.quadraticTo (mc.x, mc.y + ry * 0.2f, mc.x + rx * 0.36f, mc.y - 3);
        mouth.closeSubPath();
        g.setColour (Colours::white);
        g.fillPath (mouth);
        g.setColour (Palette::ink);
        g.strokePath (mouth, PathStrokeType (2.5f));
    }

    // Steam puffs when angry.
    if (angry)
    {
        g.setColour (Palette::ink);
        for (float s : { -1.0f, 1.0f })
        {
            Path steam;
            const float sx = hc.x + s * (rx + 8), sy = hc.y - ry - 4;
            steam.startNewSubPath (sx, sy + 14);
            steam.cubicTo (sx + s * 8, sy + 6, sx - s * 6, sy, sx + s * 4, sy - 12);
            g.strokePath (steam, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
}

void ChairGuy::paint (Graphics& g)
{
    const float amount = Layout::closeness (distance);
    const float x = centreX() + shake.x;
    const Point<float> base (x + 20.0f, (float) getHeight() - 4.0f + shake.y);

    // Floor shadow.
    g.setColour (Palette::ink.withAlpha (0.18f));
    g.fillEllipse (Rectangle<float> (170, 14).withCentre (base.translated (-20, -4)));

    drawChair (g, base, amount);
    const float seatY = base.y - 112.0f * (1.0f - 0.22f * amount);
    drawPerson (g, { base.x - 4, seatY - 4 }, 1.0f + 0.35f * amount);

    // Speech bubble when things get intense.
    String line;
    if (angry)                 line = "#@%!!";
    else if (distance < 12.0f) line = "MMH, BASSY!";
    else if (distance > 92.0f) line = "CAN YOU HEAR ME?";

    if (line.isNotEmpty())
    {
        const auto bubble = Rectangle<float> (128, 38).withCentre ({ x - 72, 26 });
        Path p;
        p.addRoundedRectangle (bubble, 16);
        p.addTriangle (bubble.getCentreX() + 20, bubble.getBottom() - 2, bubble.getCentreX() + 44, bubble.getBottom() - 2,
                       bubble.getCentreX() + 56, bubble.getBottom() + 18);
        g.setColour (Colours::white);
        g.fillPath (p);
        g.setColour (Palette::ink);
        g.strokePath (p, PathStrokeType (2.5f));
        Comic::plainText (g, line, bubble.reduced (6, 4), 16.0f, Justification::centred,
                          angry ? Palette::brick : Palette::ink);
    }
}

//==============================================================================
ProximityEffectEditor::Panel::Panel (ProximityEffectProcessor& p)
    : processor (p), guy (*p.apvts.getParameter (ParamIDs::distance))
{
    auto setupKnob = [this] (Slider& s, const char* id, bool pointer = false)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (pointer ? -2.0f : -2.4f, pointer ? 2.0f : 2.4f, true);
        s.getProperties().set ("pointer", pointer);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, id, s));
    };

    setupKnob (inputGain, ParamIDs::inputGain);
    setupKnob (lowEnd, ParamIDs::lowEnd);
    setupKnob (pattern, ParamIDs::pattern, true);
    setupKnob (output, ParamIDs::output);
    setupKnob (gainBoost, ParamIDs::gainBoost);
    setupKnob (mix, ParamIDs::mix);
    pattern.onValueChange = [this] { repaint(); };

    auto setupToggle = [this] (ToggleButton& b, const char* id, bool red = false)
    {
        b.getProperties().set ("red", red);
        b.onStateChange = [this] { repaint(); };
        addAndMakeVisible (b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.apvts, id, b));
    };

    setupToggle (hpf, ParamIDs::hpf);
    setupToggle (air, ParamIDs::air);
    setupToggle (angry, ParamIDs::angry, true);
    setupToggle (pad, ParamIDs::pad);
    setupToggle (polarity, ParamIDs::polarity);

    addAndMakeVisible (guy);
}

void ProximityEffectEditor::Panel::resized()
{
    auto place = [] (Slider& s, Point<int> c, int size) { s.setBounds (Rectangle<int> (size, size).withCentre (c)); };
    place (inputGain, Layout::inputGain, Layout::knob);
    place (lowEnd, Layout::lowEnd, Layout::knob);
    place (pattern, Layout::pattern, Layout::patternKnob);
    place (output, Layout::output, Layout::knob);
    place (gainBoost, Layout::gainBoost, Layout::knob);
    place (mix, Layout::mix, Layout::knob);

    int x = Layout::firstSlotX;
    for (auto* b : { &hpf, &air, &angry, &pad, &polarity })
    {
        b->setBounds (x, Layout::toggleY, Layout::toggleW, Layout::toggleH);
        x += Layout::slotW;
    }

    guy.setBounds (Layout::guy);
}

void ProximityEffectEditor::Panel::drawKnobLabels (Graphics& g, const Slider& s, const String& title,
                                                    const String& minText, const String& midText, const String& maxText,
                                                    const String& badgeText)
{
    const auto b = s.getBounds().toFloat();
    Comic::plainText (g, title, { b.getCentreX() - 90, b.getY() - 44, 180, 24 }, 22.0f);
    Comic::plainText (g, midText, { b.getCentreX() - 30, b.getY() - 20, 60, 18 }, 15.0f);
    Comic::plainText (g, minText, { b.getX() - 30, b.getBottom() - 12, 40, 18 }, 15.0f);
    Comic::plainText (g, maxText, { b.getRight() - 10, b.getBottom() - 12, 40, 18 }, 15.0f);
    if (badgeText.isNotEmpty())
        Comic::badge (g, badgeText, Rectangle<float> (110, 22).withCentre ({ b.getCentreX(), b.getBottom() + 20 }));
}

void ProximityEffectEditor::Panel::drawPatternSelector (Graphics& g)
{
    static const char* names[] = { "OMNI", "CARDIOID", "SUPER", "HYPER", "FIG 8" };
    static const float shapes[] = { 1.0f, 0.5f, 0.37f, 0.25f, 0.0f };

    const auto b = pattern.getBounds().toFloat();
    const auto c = b.getCentre();
    Comic::plainText (g, "POLAR PATTERN", { c.x - 80, b.getY() - 56, 190, 24 }, 21.0f);

    const int current = jlimit (0, 4, (int) std::round (pattern.getValue()));
    for (int i = 0; i < 5; ++i)
    {
        const float angle = -2.0f + (float) i;
        const auto p = c.getPointOnCircumference (60.0f, angle);
        const auto area = Rectangle<float> (70, 16).withCentre (p.translated (i == 0 ? 6.0f : (i == 4 ? -6.0f : 0.0f), 0));
        Comic::plainText (g, names[i], area, i == current ? 14.0f : 12.0f, Justification::centred,
                          i == current ? Palette::brick : Palette::ink);
    }

    // Big icon of the selected capsule pattern, like the markings on a real mic.
    const Point<float> iconCentre (c.x + 82, c.y - 4);
    g.setColour (Palette::creamDark);
    g.fillEllipse (Rectangle<float> (50, 50).withCentre (iconCentre));
    g.setColour (Palette::ink);
    g.drawEllipse (Rectangle<float> (50, 50).withCentre (iconCentre), 2.0f);
    Comic::polarIcon (g, iconCentre.translated (0, current == 0 ? 0.0f : 3.0f), current == 4 ? 11.0f : 18.0f,
                      shapes[current], Palette::brick);
}

void ProximityEffectEditor::Panel::drawScene (Graphics& g)
{
    const auto stroke = PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded);
    const auto outline = [&] (const Path& p, Colour fill)
    {
        g.setColour (fill);
        g.fillPath (p);
        g.setColour (Palette::ink);
        g.strokePath (p, stroke);
    };

    // Rail the chair rolls on, with a centimetre scale.
    g.setColour (Palette::ink);
    g.fillRoundedRectangle (Layout::railLeft, Layout::railY - 3, Layout::railRight - Layout::railLeft, 6, 3);
    const float x0 = (float) Layout::guy.getX() + ChairGuy::closeX + 20.0f;
    const float x1 = (float) Layout::guy.getX() + ChairGuy::farX + 20.0f;
    for (int i = 0; i <= 20; ++i)
    {
        const float x = x0 + (x1 - x0) * (float) i / 20.0f;
        g.drawLine (x, Layout::railY + 3, x, Layout::railY + (i % 10 == 0 ? 14.0f : (i % 5 == 0 ? 10.0f : 7.0f)), 2.0f);
    }
    Comic::plainText (g, "0 (VERY CLOSE)", { x0 - 60, Layout::railY + 13, 120, 16 }, 12.0f);
    Comic::plainText (g, "50 cm", { (x0 + x1) / 2 - 40, Layout::railY + 13, 80, 16 }, 12.0f);
    Comic::plainText (g, "100 (TOO FAR)", { x1 - 60, Layout::railY + 13, 120, 16 }, 12.0f);

    // Dashed "move closer" arrow above the rail.
    {
        Path arrow;
        arrow.startNewSubPath (x0 + 30, 172);
        arrow.lineTo (x1 - 30, 172);
        Path dashed;
        const float dashes[] = { 9.0f, 6.0f };
        PathStrokeType (2.5f).createDashedStroke (dashed, arrow, dashes, 2);
        g.setColour (Palette::ink.withAlpha (0.55f));
        g.fillPath (dashed);
        Path heads;
        heads.addTriangle (x0 + 16, 172, x0 + 32, 164, x0 + 32, 180);
        heads.addTriangle (x1 - 16, 172, x1 - 32, 164, x1 - 32, 180);
        g.fillPath (heads);
    }

    // Desk.
    const float deskTop = 400.0f;
    Path desk;
    desk.addRoundedRectangle (196, deskTop, 230, 22, 4);
    Path legs;
    legs.addRectangle (210, deskTop + 22, 18, 104);
    legs.addRectangle (396, deskTop + 22, 18, 104);
    legs.addRectangle (228, deskTop + 22, 168, 54);
    outline (legs, Colour (0xff9a6235));
    outline (desk, Colour (0xffb5763f));
    g.setColour (Palette::ink.withAlpha (0.25f));
    g.drawLine (240, deskTop + 49, 384, deskTop + 49, 2.0f);

    // Monitor speakers.
    for (float sx : { 214.0f, 330.0f })
    {
        Path box;
        box.addRoundedRectangle (sx, 322, 52, 78, 6);
        outline (box, Colour (0xff2d2b29));
        g.setColour (Palette::mustard);
        g.fillEllipse (sx + 8, 352, 36, 36);
        g.setColour (Palette::ink);
        g.drawEllipse (sx + 8, 352, 36, 36, 2.5f);
        g.fillEllipse (sx + 20, 364, 12, 12);
        g.setColour (Palette::metal);
        g.fillEllipse (sx + 19, 330, 14, 14);
    }

    // Coffee mug.
    Path mug;
    mug.addRoundedRectangle (390, 372, 28, 30, 4);
    g.setColour (Palette::ink);
    g.drawEllipse (412, 378, 14, 14, 3.5f);
    outline (mug, Colours::white);
    g.setColour (Palette::brick);
    g.fillRect (392.0f, 382.0f, 24.0f, 7.0f);

    // Mic stand: base on desk, boom arm up to the mic.
    g.setColour (Palette::ink);
    g.drawLine (300, deskTop, 300, 300, 7.0f);
    g.drawLine (300, 300, 268, 228, 7.0f);
    g.drawLine (268, 228, 312, 196, 6.0f);
    g.fillEllipse (292, 292, 16, 16);
    g.fillEllipse (260, 220, 16, 16);
    g.fillRoundedRectangle (282, deskTop - 8, 36, 10, 3);

    // Condenser mic in its shock mount.
    Path body;
    body.addRoundedRectangle (300, 150, 34, 92, 12);
    outline (body, Palette::metal);
    Path grille;
    grille.addRoundedRectangle (300, 150, 34, 44, 14);
    outline (grille, Palette::metal.brighter (0.25f));
    g.setColour (Palette::ink.withAlpha (0.5f));
    for (float gx = 306; gx < 332; gx += 6)
        g.drawLine (gx, 154, gx, 190, 1.2f);
    g.setColour (Palette::ink);
    g.drawEllipse (288, 186, 58, 26, 3.0f);
    g.drawLine (317, 242, 317, 256, 5.0f);

    // Pop filter on a gooseneck.
    Path neck;
    neck.startNewSubPath (300, 270);
    neck.cubicTo (350, 280, 372, 250, 372, 232);
    g.strokePath (neck, PathStrokeType (3.5f));
    const auto pop = Rectangle<float> (16, 64).withCentre ({ 366, 196 });
    g.setColour (Palette::ink.withAlpha (0.35f));
    g.fillEllipse (pop);
    g.setColour (Palette::ink);
    g.drawEllipse (pop, 4.0f);
}

void ProximityEffectEditor::Panel::drawBottomBar (Graphics& g)
{
    const auto bar = Layout::bottomBar;
    g.setColour (Palette::ink);
    g.fillRoundedRectangle (bar.translated (3, 4), 10);
    g.setColour (Palette::bar);
    g.fillRoundedRectangle (bar, 10);
    g.setColour (Palette::ink);
    g.drawRoundedRectangle (bar, 10, 3);

    struct Label { String top, bottom; };
    const Label labels[] = { { "HPF FILTER", "80 Hz" }, { "AIR MODE", "10 kHz" }, { "ANGRY", "MODE" },
                             { "EGO KILLER", "-20 dB PAD" }, { "POLARITY", String (CharPointer_UTF8 ("\xc3\x98 INVERT")) } };

    int x = Layout::firstSlotX + Layout::toggleW + 6;
    for (int i = 0; i < 5; ++i)
    {
        const auto area = Rectangle<float> ((float) x, (float) Layout::toggleY + 14.0f, 92.0f, 44.0f);
        const auto colour = i == 2 ? Colour (0xff3a0e0a) : Palette::ink;
        Comic::plainText (g, labels[i].top, area.withHeight (22), 17.0f, Justification::centredLeft, colour);
        Comic::plainText (g, labels[i].bottom, area.withTrimmedTop (22), 14.0f, Justification::centredLeft, colour);
        if (i == 2)
            Comic::angryFace (g, { area.getX() + 80, area.getY() + 11 }, 11.0f, angry.getToggleState());
        x += Layout::slotW;
    }

    // Slogan plate.
    const auto plate = Rectangle<float> (x - 2.0f, bar.getY() + 10, bar.getRight() - (float) x - 8.0f, bar.getHeight() - 20);
    g.setColour (Palette::ink);
    g.fillRoundedRectangle (plate.translated (3, 3), 6);
    g.setColour (Palette::brick);
    g.fillRoundedRectangle (plate, 6);
    g.setColour (Palette::mustard);
    g.drawRoundedRectangle (plate.reduced (3), 4, 2);
    Comic::plainText (g, "MOVE CLOSER", plate.withTrimmedBottom (plate.getHeight() * 0.45f).reduced (6, 0).translated (0, 4),
                      17.0f, Justification::centred, Palette::cream);
    Comic::plainText (g, "HEAR THE DIFFERENCE", plate.withTrimmedTop (plate.getHeight() * 0.55f).reduced (6, 0).translated (0, -4),
                      12.0f, Justification::centred, Palette::cream);
}

void ProximityEffectEditor::Panel::paint (Graphics& g)
{
    const auto area = getLocalBounds().toFloat();

    g.fillAll (Palette::ink);
    const auto face = area.reduced (6);
    ColourGradient bg (Palette::cream.brighter (0.05f), face.getCentreX(), face.getCentreY(),
                       Palette::creamDark, face.getX(), face.getY(), true);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (face, 18);
    g.setColour (Palette::metal.darker (0.2f));
    g.drawRoundedRectangle (face.reduced (3), 16, 4);

    for (auto p : { face.getTopLeft().translated (22, 22), face.getTopRight().translated (-22, 22),
                    face.getBottomLeft().translated (22, -22), face.getBottomRight().translated (-22, -22) })
        Comic::screw (g, p, 8);

    // Title.
    Comic::outlinedText (g, "PROXIMITY", { 250, 18, 290, 62 }, 56.0f, Palette::sage, 5.0f, Justification::centredRight);
    Comic::outlinedText (g, "EFFECT", { 552, 18, 200, 62 }, 56.0f, Palette::brick, 5.0f, Justification::centredLeft);
    g.setColour (Palette::ink);
    for (float s : { -1.0f, 1.0f })
        for (int i = 0; i < 3; ++i)
        {
            const float cx = s < 0 ? 228.0f : 770.0f, cy = 48.0f;
            const float a = (s < 0 ? MathConstants<float>::pi : 0.0f) + (float) (i - 1) * 0.45f;
            g.drawLine (cx + std::cos (a) * 8 * s * s, cy + std::sin (a) * 14, cx + std::cos (a) * 24, cy + std::sin (a) * 30, 3.0f);
        }
    Comic::badge (g, "GET CLOSER. GET THICKER.", { 366, 86, 228, 26 });

    drawKnobLabels (g, inputGain, "INPUT GAIN", String (CharPointer_UTF8 ("-\xe2\x88\x9e")), "0", "+20", {});
    drawKnobLabels (g, lowEnd, "LOW-END PUSH", "-12", "0", "+12", "MORE BOOM");
    drawKnobLabels (g, output, "OUTPUT", String (CharPointer_UTF8 ("-\xe2\x88\x9e")), "0", "+12", {});
    drawKnobLabels (g, gainBoost, "GAIN BOOST", "0", "+6", "+12", {});
    drawKnobLabels (g, mix, "MIX", "DRY", "50%", "100%", "BLEND IT");
    drawPatternSelector (g);
    drawScene (g);
    drawBottomBar (g);
}

//==============================================================================
ProximityEffectEditor::ProximityEffectEditor (ProximityEffectProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);
    panel.guy.setAngry (panel.angry.getToggleState());
    panel.setSize (baseWidth, baseHeight);

    setResizable (true, true);
    setResizeLimits (baseWidth / 2, baseHeight / 2, baseWidth * 2, baseHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ProximityEffectEditor::~ProximityEffectEditor()
{
    setLookAndFeel (nullptr);
}

void ProximityEffectEditor::paint (Graphics& g)
{
    g.fillAll (Palette::ink);
}

void ProximityEffectEditor::resized()
{
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) baseWidth));
}

void ProximityEffectEditor::timerCallback()
{
    panel.guy.setAngry (panel.angry.getToggleState());
    panel.guy.tick();
}

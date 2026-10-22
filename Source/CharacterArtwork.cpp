#include "CharacterArtwork.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
    // Rows of the artwork (in artwork pixels) used by the chair squash: everything above
    // the knees drops, shins and the chair's gas pole compress, shoes and wheels stay put.
    constexpr float kneeY = 470.0f, shinBottomY = 575.0f;
    constexpr float maxDrop = 16.0f, maxLean = 16.0f;

    // Face inflation (elliptical bulge around the head).
    // Centred low on the face: cheeks and jaw swell most, the forehead and hairline barely move.
    constexpr float bulgeRx = 88.0f, bulgeRy = 85.0f, bulgeX = 0.20f, bulgeY = 0.14f;
    const Point<float> bulgeCentre { 305.0f, 163.0f };
    const Rectangle<int> headBox { 212, 60, 186, 196 };

    // Angry expression: control points on the original face and how far they move.
    struct Handle { float x, y, dx, dy, sigma; };
    constexpr Handle angerHandles[] = {
        { 294, 126,  2.0f,  5.0f, 7.0f }, { 312, 126, -2.0f,  5.0f, 7.0f }, // inner brows down
        { 265, 129,  0.0f, -2.5f, 7.0f }, { 341, 127,  0.0f, -2.5f, 7.0f }, // outer brows up
        { 281, 133,  0.0f,  2.5f, 4.5f }, { 325, 131,  0.0f,  2.5f, 4.5f }, // upper lids: squint
        { 281, 142,  0.0f, -1.5f, 4.0f }, { 325, 140,  0.0f, -1.5f, 4.0f }, // lower lids
        { 281, 172,  3.0f,  6.0f, 8.0f }, { 329, 171, -3.0f,  6.0f, 8.0f }, // mouth corners down
        { 304, 167,  0.0f,  1.0f, 6.0f }, { 304, 181,  0.0f, -2.0f, 6.0f }, // gritted teeth
    };

    float smooth01 (float x) noexcept
    {
        x = jlimit (0.0f, 1.0f, x);
        return x * x * (3.0f - 2.0f * x);
    }
}

bool CharacterArtwork::Pose::closeTo (const Pose& o) const noexcept
{
    constexpr float eps = 0.002f;
    return std::abs (squash - o.squash) < eps && std::abs (puff - o.puff) < eps && std::abs (flush - o.flush) < eps
        && std::abs (anger - o.anger) < eps && std::abs (extreme - o.extreme) < eps;
}

CharacterArtwork::CharacterArtwork()
{
    source = ImageCache::getFromMemory (BinaryData::character_png, BinaryData::character_pngSize)
                 .convertedToFormat (Image::ARGB);
    if (source.isValid())
    {
        rendered = Image (Image::ARGB, source.getWidth(), source.getHeight(), true);
        buildSkinMask();
    }
}

void CharacterArtwork::buildSkinMask()
{
    const int w = source.getWidth(), h = source.getHeight();
    std::vector<float> raw ((size_t) (w * h), 0.0f);
    const Image::BitmapData data (source, Image::BitmapData::readOnly);

    for (int y = headBox.getY(); y < headBox.getBottom(); ++y)
        for (int x = headBox.getX(); x < headBox.getRight(); ++x)
        {
            const auto c = data.getPixelColour (x, y);
            if (c.getAlpha() < 200)
                continue;
            const float r = c.getFloatRed() * 255.0f, g = c.getFloatGreen() * 255.0f, b = c.getFloatBlue() * 255.0f;
            if (! (r > g && g > b))
                continue;
            const float lum = 0.299f * r + 0.587f * g + 0.114f * b;
            if (r - b < 35.0f || lum < 115.0f)
                continue; // hair, beard, brows and shadows keep their colour
            const float ex = ((float) x - headCentre.x) / 66.0f, ey = ((float) y - headCentre.y) / 82.0f;
            const float inside = 1.0f - smooth01 ((std::sqrt (ex * ex + ey * ey) - 0.85f) / 0.15f);

            // A real flush shows first on cheeks, nose and ears.
            float spot = 0.0f;
            for (auto [cx, cy, weight] : { std::tuple { 272.0f, 160.0f, 1.0f }, std::tuple { 337.0f, 159.0f, 1.0f },
                                           std::tuple { 304.0f, 155.0f, 0.7f }, std::tuple { 253.0f, 150.0f, 0.9f },
                                           std::tuple { 359.0f, 148.0f, 0.9f } })
            {
                const float dx = (float) x - cx, dy = (float) y - cy;
                spot = jmax (spot, weight * std::exp (-(dx * dx + dy * dy) / (2.0f * 16.0f * 16.0f)));
            }

            raw[(size_t) (y * w + x)] = jlimit (0.0f, 1.0f, (r - b - 35.0f) / 30.0f)
                                      * jlimit (0.0f, 1.0f, (lum - 115.0f) / 25.0f)
                                      * jlimit (0.0f, 1.0f, (240.0f - lum) / 20.0f)
                                      * inside * (0.45f + 0.55f * spot);
        }

    // One 3x3 blur so the tint has no hard edges.
    skinMask.assign ((size_t) (w * h), 0.0f);
    for (int y = 1; y < h - 1; ++y)
        for (int x = 1; x < w - 1; ++x)
        {
            float sum = 0.0f;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    sum += raw[(size_t) ((y + dy) * w + x + dx)];
            skinMask[(size_t) (y * w + x)] = sum / 9.0f;
        }
}

Point<float> CharacterArtwork::mapInverse (float x, float y, const Pose& pose) const
{
    // 1. Chair squash: undo the drop above the knees and the compression of the shins.
    const float drop = maxDrop * pose.squash;
    float sy = y;
    if (y < kneeY + drop)
        sy = y - drop;
    else if (y < shinBottomY)
        sy = kneeY + (y - kneeY - drop) * (shinBottomY - kneeY) / (shinBottomY - kneeY - drop);

    // 2. Forward lean toward the mic: the higher, the further left.
    float sx = x;
    if (sy < kneeY)
        sx = x + maxLean * pose.squash * (kneeY - sy) / (kneeY - 70.0f);
    // (callers rely on the row shift being independent of x up to here)

    if (! headBox.toFloat().contains (sx, sy))
        return { sx, sy };

    // 3. Face inflation.
    if (pose.puff > 0.0f)
    {
        const float dx = sx - bulgeCentre.x, dy = sy - bulgeCentre.y;
        const float r2 = (dx * dx) / (bulgeRx * bulgeRx) + (dy * dy) / (bulgeRy * bulgeRy);
        if (r2 < 1.0f)
        {
            const float falloff = (1.0f - r2) * (1.0f - r2) * 0.5f + (1.0f - r2) * 0.5f;
            sx = bulgeCentre.x + dx * (1.0f - bulgeX * pose.puff * falloff);
            sy = bulgeCentre.y + dy * (1.0f - bulgeY * pose.puff * falloff);
        }
    }

    // 4. Angry expression.
    if (pose.anger > 0.0f)
    {
        float ox = 0.0f, oy = 0.0f;
        for (const auto& h : angerHandles)
        {
            const float ddx = sx - h.x, ddy = sy - h.y;
            if (ddx * ddx + ddy * ddy > 9.0f * h.sigma * h.sigma)
                continue;
            const float wgt = std::exp (-(ddx * ddx + ddy * ddy) / (2.0f * h.sigma * h.sigma));
            ox += h.dx * wgt;
            oy += h.dy * wgt;
        }
        sx -= ox * pose.anger;
        sy -= oy * pose.anger;
    }

    return { sx, sy };
}

Point<float> CharacterArtwork::mapForward (Point<float> p, const Pose& pose) const
{
    // Fixed-point inversion of mapInverse (the deformation is small and smooth).
    Point<float> o = p;
    for (int i = 0; i < 8; ++i)
        o += p - mapInverse (o.x, o.y, pose);
    return o;
}

const Image& CharacterArtwork::render (const Pose& pose, float scale)
{
    scale = jlimit (0.25f, 1.0f, scale);
    if (! source.isValid() || (hasRendered && pose.closeTo (renderedPose) && scale == renderedScale))
        return rendered;

    const int w = source.getWidth(), h = source.getHeight();
    const int outW = roundToInt ((float) w * scale), outH = roundToInt ((float) h * scale);
    if (rendered.getWidth() != outW || rendered.getHeight() != outH)
        rendered = Image (Image::ARGB, outW, outH, true);
    const float step = (float) w / (float) outW;
    const Image::BitmapData src (source, Image::BitmapData::readOnly);
    Image::BitmapData dst (rendered, Image::BitmapData::writeOnly);
    const float flush = jlimit (0.0f, 1.0f, pose.flush);

    auto px = [&] (int x, int y) -> const PixelARGB&
    {
        return *reinterpret_cast<const PixelARGB*> (src.getPixelPointer (x, y));
    };

    // Bilinear sample (premultiplied) with an optional skin flush, written to `out`.
    auto sample = [&] (float sx, float sy, PixelARGB& out)
    {
        if (sx < 0.0f || sy < 0.0f || sx >= (float) (w - 1) || sy >= (float) (h - 1))
        {
            out.setARGB (0, 0, 0, 0);
            return;
        }
        const int x0 = (int) sx, y0 = (int) sy;
        const float fx = sx - (float) x0, fy = sy - (float) y0;
        const auto& p00 = px (x0, y0); const auto& p10 = px (x0 + 1, y0);
        const auto& p01 = px (x0, y0 + 1); const auto& p11 = px (x0 + 1, y0 + 1);
        const float w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
        float a = w00 * p00.getAlpha() + w10 * p10.getAlpha() + w01 * p01.getAlpha() + w11 * p11.getAlpha();
        if (a < 0.5f)
        {
            out.setARGB (0, 0, 0, 0);
            return;
        }
        float r = w00 * p00.getRed() + w10 * p10.getRed() + w01 * p01.getRed() + w11 * p11.getRed();
        float g = w00 * p00.getGreen() + w10 * p10.getGreen() + w01 * p01.getGreen() + w11 * p11.getGreen();
        float b = w00 * p00.getBlue() + w10 * p10.getBlue() + w01 * p01.getBlue() + w11 * p11.getBlue();

        // Flush: warm the skin toward red, following the face as it deforms.
        if (flush > 0.0f)
        {
            const size_t i = (size_t) (y0 * w + x0);
            const float t = flush * (w00 * skinMask[i] + w10 * skinMask[i + 1] + w01 * skinMask[i + (size_t) w] + w11 * skinMask[i + (size_t) w + 1]);
            if (t > 0.0f)
            {
                r += (a - r) * 0.06f * t;
                g *= 1.0f - 0.22f * t;
                b *= 1.0f - 0.26f * t;
            }
        }
        out.setARGB ((uint8) (a + 0.5f), (uint8) jmin (a, r + 0.5f), (uint8) jmin (a, g + 0.5f), (uint8) jmin (a, b + 0.5f));
    };

    const auto head = headBox.toFloat();
    for (int oy = 0; oy < outH; ++oy)
    {
        auto* line = reinterpret_cast<PixelARGB*> (dst.getLinePointer (oy));
        const float y = ((float) oy + 0.5f) * step - 0.5f;

        // Squash and lean are the same for a whole row: work them out once.
        const auto rowStart = mapInverse (0.0f, y, Pose { pose.squash, 0.0f, 0.0f, 0.0f, 0.0f });
        const float sy = rowStart.y, shift = rowStart.x;
        const bool rowTouchesHead = sy >= head.getY() - 1.0f && sy < head.getBottom() + 1.0f;

        for (int ox = 0; ox < outW; ++ox)
        {
            const float x = ((float) ox + 0.5f) * step - 0.5f;
            const float sx = x + shift;
            if (rowTouchesHead && sx >= head.getX() && sx < head.getRight())
            {
                const auto s = mapInverse (x, y, pose);
                sample (s.x, s.y, line[ox]);
            }
            else
            {
                sample (sx, sy, line[ox]);
            }
        }
    }

    renderedPose = pose;
    renderedScale = scale;
    hasRendered = true;
    return rendered;
}

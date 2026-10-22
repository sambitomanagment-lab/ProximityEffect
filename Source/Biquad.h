#pragma once

#include <cmath>

// Minimal RBJ-cookbook biquad (transposed direct form II), one instance per channel.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() noexcept { z1 = z2 = 0; }

    float process (float x) noexcept
    {
        const double in = x;
        const double out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        return (float) out;
    }

    void setLowShelf (double sr, double freq, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi() * freq / sr;
        const double cs = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        const double sqA = 2.0 * std::sqrt (A) * alpha;
        set (A * ((A + 1) - (A - 1) * cs + sqA),
             2 * A * ((A - 1) - (A + 1) * cs),
             A * ((A + 1) - (A - 1) * cs - sqA),
             (A + 1) + (A - 1) * cs + sqA,
             -2 * ((A - 1) + (A + 1) * cs),
             (A + 1) + (A - 1) * cs - sqA);
    }

    void setHighShelf (double sr, double freq, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi() * freq / sr;
        const double cs = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        const double sqA = 2.0 * std::sqrt (A) * alpha;
        set (A * ((A + 1) + (A - 1) * cs + sqA),
             -2 * A * ((A - 1) + (A + 1) * cs),
             A * ((A + 1) + (A - 1) * cs - sqA),
             (A + 1) - (A - 1) * cs + sqA,
             2 * ((A - 1) - (A + 1) * cs),
             (A + 1) - (A - 1) * cs - sqA);
    }

    void setPeak (double sr, double freq, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi() * freq / sr;
        const double cs = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        set (1 + alpha * A, -2 * cs, 1 - alpha * A,
             1 + alpha / A, -2 * cs, 1 - alpha / A);
    }

    void setHighPass (double sr, double freq, double q) noexcept
    {
        const double w = twoPi() * freq / sr;
        const double cs = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        set ((1 + cs) / 2, -(1 + cs), (1 + cs) / 2,
             1 + alpha, -2 * cs, 1 - alpha);
    }

private:
    static constexpr double twoPi() { return 6.283185307179586; }

    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0;
        a1 = na1 / na0; a2 = na2 / na0;
    }
};

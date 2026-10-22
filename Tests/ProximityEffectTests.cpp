// Automated tests for Proximity Effect.
//
//   ProximityEffectTests                  run all tests, exit code 1 on failure
//   ProximityEffectTests --report         also print the full frequency-response table
//   ProximityEffectTests --snapshots DIR  also render the GUI to PNG files in DIR
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
int failures = 0, checks = 0;

void check (bool ok, const String& what)
{
    ++checks;
    if (! ok)
    {
        ++failures;
        std::printf ("  FAIL: %s\n", what.toRawUTF8());
    }
}

void section (const char* name) { std::printf ("[%s]\n", name); }

void setParam (ProximityEffectProcessor& p, const char* id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

float getParam (ProximityEffectProcessor& p, const char* id)
{
    auto* param = p.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}

bool allFinite (const AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (ch, i)))
                return false;
    return true;
}

// Steady-state RMS gain in dB of a sine at `freq` through the plugin.
float measure (ProximityEffectProcessor& p, float freq, float level = 0.05f, double sr = 48000.0, int block = 512)
{
    p.prepareToPlay (sr, block);
    AudioBuffer<float> buf (p.getTotalNumInputChannels(), block);
    MidiBuffer midi;
    const int blocks = (int) (sr * 1.2 / block) + 2;
    double phase = 0.0, sum = 0.0;
    int count = 0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            const float s = level * (float) std::sin (phase);
            phase += MathConstants<double>::twoPi * freq / sr;
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample (ch, i, s);
        }
        p.processBlock (buf, midi);
        if (b > blocks / 2)
        {
            const float r = buf.getRMSLevel (0, 0, block);
            sum += (double) r * r;
            ++count;
        }
    }
    return Decibels::gainToDecibels ((float) std::sqrt (sum / count) / (level / std::sqrt (2.0f)));
}

// Runs `signal` through the plugin and returns the output (channel 0).
std::vector<float> render (ProximityEffectProcessor& p, const std::vector<float>& signal, double sr, int block,
                           const std::function<void (int blockIndex)>& beforeBlock = {})
{
    p.prepareToPlay (sr, block);
    AudioBuffer<float> buf (p.getTotalNumInputChannels(), block);
    MidiBuffer midi;
    std::vector<float> out;
    for (size_t start = 0, b = 0; start < signal.size(); start += (size_t) block, ++b)
    {
        if (beforeBlock)
            beforeBlock ((int) b);
        buf.clear();
        for (int i = 0; i < block && start + (size_t) i < signal.size(); ++i)
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample (ch, i, signal[start + (size_t) i]);
        p.processBlock (buf, midi);
        for (int i = 0; i < block; ++i)
            out.push_back (buf.getSample (0, i));
    }
    out.resize (signal.size());
    return out;
}

std::vector<float> sine (float freq, float level, double sr, int samples)
{
    std::vector<float> s ((size_t) samples);
    for (int i = 0; i < samples; ++i)
        s[(size_t) i] = level * (float) std::sin (MathConstants<double>::twoPi * freq * i / sr);
    return s;
}

float bassLift (ProximityEffectProcessor& p, float distance, int pattern)
{
    setParam (p, ParamIDs::distance, distance);
    setParam (p, ParamIDs::pattern, (float) pattern);
    return measure (p, 60.0f) - measure (p, 1000.0f);
}

//==============================================================================
void testProximityCalibration()
{
    section ("proximity calibration (bass at 60 Hz relative to 1 kHz)");
    ProximityEffectProcessor p;
    const float fig8 = bassLift (p, 0.0f, 4), card = bassLift (p, 0.0f, 1), omni = bassLift (p, 0.0f, 0);
    std::printf ("  0 cm: figure 8 %+.2f dB, cardioid %+.2f dB, omni %+.2f dB\n", fig8, card, omni);
    check (fig8 > 9.0f && fig8 < 11.0f, "figure 8 bass lift should be about +10 dB, got " + String (fig8, 2));
    check (card > 4.0f && card < 6.0f, "cardioid bass lift should be about +5 dB, got " + String (card, 2));
    check (std::abs (omni) < 0.3f, "omni should add no proximity bass, got " + String (omni, 2));

    const float super = bassLift (p, 0.0f, 2), hyper = bassLift (p, 0.0f, 3);
    check (card < super && super < hyper && hyper < fig8, "lift should grow cardioid < super < hyper < figure 8");

    for (int pat = 0; pat < 5; ++pat)
    {
        const float far = bassLift (p, 100.0f, pat);
        check (std::abs (far) < 0.1f, "at 100 cm the response should be flat (pattern " + String (pat) + "), got " + String (far, 2));
    }

    float previous = -1.0f;
    for (float d : { 100.0f, 75.0f, 50.0f, 25.0f, 10.0f, 0.0f })
    {
        const float lift = bassLift (p, d, 4);
        check (lift >= previous - 0.01f, "bass lift should grow as the distance shrinks (" + String (d) + " cm)");
        previous = lift;
    }
}

void testFlatWhenFar()
{
    section ("100 cm, default controls: unity gain across the band");
    ProximityEffectProcessor p;
    setParam (p, ParamIDs::distance, 100.0f);
    for (float f : { 40.0f, 100.0f, 1000.0f, 5000.0f, 15000.0f })
    {
        const float g = measure (p, f);
        check (std::abs (g) < 0.1f, "gain at " + String (f) + " Hz should be 0 dB, got " + String (g, 2));
    }
}

void testPad()
{
    section ("Ego Killer pad");
    ProximityEffectProcessor p;
    setParam (p, ParamIDs::distance, 100.0f);
    const float off = measure (p, 1000.0f);
    setParam (p, ParamIDs::pad, 1.0f);
    const float on = measure (p, 1000.0f);
    std::printf ("  pad off %+.2f dB, pad on %+.2f dB\n", off, on);
    check (std::abs ((on - off) + 20.0f) < 0.05f, "pad should attenuate by exactly 20 dB, got " + String (on - off, 2));

    // The pad sits before the saturation stages, so a hot signal stops crunching once padded.
    setParam (p, ParamIDs::pad, 0.0f);
    setParam (p, ParamIDs::distance, 0.0f);
    setParam (p, ParamIDs::angry, 1.0f);
    const float hotOff = measure (p, 1000.0f, 0.9f);
    setParam (p, ParamIDs::pad, 1.0f);
    const float hotOn = measure (p, 1000.0f, 0.9f);
    check (hotOn - hotOff > -20.0f, "with Angry on, the pad should reduce drive (less than 20 dB drop at output)");
}

void testToggles()
{
    section ("HPF, Air, Angry, gain controls");
    ProximityEffectProcessor p;
    setParam (p, ParamIDs::distance, 100.0f);
    setParam (p, ParamIDs::hpf, 1.0f);
    const float hp40 = measure (p, 40.0f), hp1k = measure (p, 1000.0f);
    check (hp40 < -9.0f && std::abs (hp1k) < 0.1f, "HPF should cut 40 Hz and leave 1 kHz alone");
    setParam (p, ParamIDs::hpf, 0.0f);

    setParam (p, ParamIDs::air, 1.0f);
    const float air12k = measure (p, 12000.0f), air1k = measure (p, 1000.0f);
    check (air12k > 3.0f && std::abs (air1k) < 0.3f, "Air should lift 12 kHz, got " + String (air12k, 2));
    setParam (p, ParamIDs::air, 0.0f);

    setParam (p, ParamIDs::inputGain, 6.0f);
    check (std::abs (measure (p, 1000.0f) - 6.0f) < 0.1f, "input gain +6 dB");
    setParam (p, ParamIDs::inputGain, -60.0f);
    check (measure (p, 1000.0f) <= -99.0f, "input gain at -inf should mute");
    setParam (p, ParamIDs::inputGain, 0.0f);
    setParam (p, ParamIDs::output, -12.0f);
    check (std::abs (measure (p, 1000.0f) + 12.0f) < 0.1f, "output -12 dB");
    setParam (p, ParamIDs::output, 0.0f);

    setParam (p, ParamIDs::distance, 0.0f);
    setParam (p, ParamIDs::pattern, 0.0f);
    const float boost = measure (p, 1000.0f);
    check (std::abs (boost - 4.0f) < 0.5f, "default gain boost at 0 cm should be about +4 dB, got " + String (boost, 2));
    setParam (p, ParamIDs::mix, 0.0f);
    check (std::abs (measure (p, 1000.0f)) < 0.1f, "mix 0% should be dry");
    setParam (p, ParamIDs::mix, 100.0f);

    setParam (p, ParamIDs::angry, 1.0f);
    std::vector<float> s = sine (1000.0f, 0.3f, 48000.0, 48000);
    auto y = render (p, s, 48000.0, 512);
    // Distortion adds harmonics: the output is no longer a scaled sine.
    double dot = 0, yy = 0, ss = 0;
    for (size_t i = 24000; i < y.size(); ++i)
    {
        const double ref = std::sin (MathConstants<double>::twoPi * 1000.0 * (double) (i - (size_t) p.getLatencySamples()) / 48000.0);
        dot += y[i] * ref; yy += (double) y[i] * y[i]; ss += ref * ref;
    }
    const double correlation = std::abs (dot) / std::sqrt (yy * ss);
    check (correlation < 0.995, "Angry should distort the signal (correlation " + String (correlation, 4) + ")");
}

void testLatencyPolarityBypass()
{
    section ("latency, polarity, bypass");
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        ProximityEffectProcessor p;
        setParam (p, ParamIDs::distance, 100.0f);
        std::vector<float> impulse (4096, 0.0f);
        impulse[100] = 1.0f;
        auto y = render (p, impulse, sr, 256);
        const int latency = p.getLatencySamples();
        const auto peakAt = (int) std::distance (y.begin(), std::max_element (y.begin(), y.end(), [] (float a, float b) { return std::abs (a) < std::abs (b); }));
        check (peakAt == 100 + latency, "reported latency should match the real delay at " + String (sr) + " Hz (reported "
                                            + String (latency) + ", measured " + String (peakAt - 100) + ")");

        setParam (p, ParamIDs::polarity, 1.0f);
        auto inv = render (p, impulse, sr, 256);
        check (inv[(size_t) (100 + latency)] < -0.5f, "polarity invert should flip the impulse");
        setParam (p, ParamIDs::polarity, 0.0f);

        // Bypass: the untouched input, delayed by the same latency, even with heavy settings.
        setParam (p, ParamIDs::distance, 0.0f);
        setParam (p, ParamIDs::angry, 1.0f);
        setParam (p, ParamIDs::bypass, 1.0f);
        auto s = sine (440.0f, 0.5f, sr, 8192);
        auto by = render (p, s, sr, 256);
        float err = 0.0f;
        for (size_t i = (size_t) latency; i < by.size(); ++i)
            err = jmax (err, std::abs (by[i] - s[i - (size_t) latency]));
        check (err < 1.0e-6f, "bypass should be the latency-aligned input, max error " + String (err));

        // Host-driven bypass without the parameter (processBlockBypassed).
        setParam (p, ParamIDs::bypass, 0.0f);
        p.prepareToPlay (sr, 256);
        AudioBuffer<float> buf (2, 256);
        MidiBuffer midi;
        float hostErr = 0.0f;
        for (int b = 0; b < 31; ++b)
        {
            for (int i = 0; i < 256; ++i)
                buf.setSample (0, i, s[(size_t) (b * 256 + i)]), buf.setSample (1, i, s[(size_t) (b * 256 + i)]);
            p.processBlockBypassed (buf, midi);
            if (b > 24) // after the 50 ms bypass crossfade
                for (int i = 0; i < 256; ++i)
                    hostErr = jmax (hostErr, std::abs (buf.getSample (0, i) - s[(size_t) (b * 256 + i - latency)]));
        }
        check (hostErr < 1.0e-6f, "processBlockBypassed should be the latency-aligned input");
    }
}

void testSampleRatesAndBlockSizes()
{
    section ("sample rates, block sizes, silence");
    for (double sr : { 22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
        for (int block : { 1, 7, 64, 512, 2048 })
        {
            ProximityEffectProcessor p;
            setParam (p, ParamIDs::distance, 0.0f);
            setParam (p, ParamIDs::pattern, 4.0f);
            setParam (p, ParamIDs::lowEnd, 12.0f);
            setParam (p, ParamIDs::gainBoost, 12.0f);
            setParam (p, ParamIDs::angry, 1.0f);
            setParam (p, ParamIDs::air, 1.0f);
            p.prepareToPlay (sr, block);
            AudioBuffer<float> buf (2, block);
            MidiBuffer midi;
            Random rnd (42);
            bool finite = true;
            float peak = 0.0f;
            for (int b = 0; b < (int) (sr * 0.2) / block + 1; ++b)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block; ++i)
                        buf.setSample (ch, i, rnd.nextFloat() * 2.0f - 1.0f);
                p.processBlock (buf, midi);
                finite = finite && allFinite (buf);
                peak = jmax (peak, buf.getMagnitude (0, block));
            }
            check (finite, "output must stay finite at " + String (sr) + " Hz, block " + String (block));
            check (peak < 8.0f, "output must stay bounded at " + String (sr) + " Hz, block " + String (block));
        }

    // Host sends a bigger block than announced in prepareToPlay.
    {
        ProximityEffectProcessor p;
        p.prepareToPlay (48000.0, 128);
        AudioBuffer<float> buf (2, 1000);
        MidiBuffer midi;
        for (int i = 0; i < 1000; ++i)
            buf.setSample (0, i, 0.5f), buf.setSample (1, i, 0.5f);
        p.processBlock (buf, midi);
        check (allFinite (buf), "oversized block should be processed safely");
    }

    // Silence in, silence out (also with Angry, whose waveshaper is DC-compensated).
    {
        ProximityEffectProcessor p;
        setParam (p, ParamIDs::angry, 1.0f);
        setParam (p, ParamIDs::distance, 0.0f);
        auto y = render (p, std::vector<float> (48000, 0.0f), 48000.0, 512);
        float peak = 0.0f;
        for (auto v : y) peak = jmax (peak, std::abs (v));
        check (peak < 1.0e-6f, "silence should stay silent, got " + String (peak));
    }
}

void testMonoLayout()
{
    section ("mono and stereo layouts");
    ProximityEffectProcessor p;
    AudioProcessor::BusesLayout mono, stereo, mixed, surround;
    mono.inputBuses.add (AudioChannelSet::mono());
    mono.outputBuses.add (AudioChannelSet::mono());
    stereo.inputBuses.add (AudioChannelSet::stereo());
    stereo.outputBuses.add (AudioChannelSet::stereo());
    mixed.inputBuses.add (AudioChannelSet::mono());
    mixed.outputBuses.add (AudioChannelSet::stereo());
    surround.inputBuses.add (AudioChannelSet::create5point1());
    surround.outputBuses.add (AudioChannelSet::create5point1());
    check (p.checkBusesLayoutSupported (mono), "mono -> mono supported");
    check (p.checkBusesLayoutSupported (stereo), "stereo -> stereo supported");
    check (! p.checkBusesLayoutSupported (mixed), "mono -> stereo rejected");
    check (! p.checkBusesLayoutSupported (surround), "5.1 rejected");

    check (p.setBusesLayout (mono), "switch to mono");
    setParam (p, ParamIDs::distance, 0.0f);
    setParam (p, ParamIDs::pattern, 4.0f);
    const float lift = measure (p, 60.0f) - measure (p, 1000.0f);
    check (lift > 9.0f && lift < 11.0f, "mono should behave like stereo, lift " + String (lift, 2));
}

void testSmoothAutomation()
{
    section ("automation without clicks");
    const double sr = 48000.0;
    const int block = 64;
    const auto s = sine (150.0f, 0.25f, sr, (int) sr * 3);

    // The largest step a smooth 150 Hz sine can take, times the largest gain the plugin can apply here.
    auto maxStep = [] (const std::vector<float>& y, size_t from)
    {
        float m = 0.0f;
        for (size_t i = from + 1; i < y.size(); ++i)
            m = jmax (m, std::abs (y[i] - y[i - 1]));
        return m;
    };

    struct Case { const char* name; std::function<void (ProximityEffectProcessor&, int)> automate; };
    const Case cases[] = {
        { "distance jumps 100 -> 0 -> 100", [] (auto& p, int b) { setParam (p, ParamIDs::distance, (b / 150) % 2 ? 0.0f : 100.0f); } },
        { "pattern jumps omni <-> figure 8", [] (auto& p, int b) { setParam (p, ParamIDs::distance, 0.0f); setParam (p, ParamIDs::pattern, (b / 150) % 2 ? 4.0f : 0.0f); } },
        { "pad toggling", [] (auto& p, int b) { setParam (p, ParamIDs::pad, (float) ((b / 150) % 2)); } },
        { "polarity toggling", [] (auto& p, int b) { setParam (p, ParamIDs::polarity, (float) ((b / 150) % 2)); } },
        { "bypass toggling", [] (auto& p, int b) { setParam (p, ParamIDs::distance, 0.0f); setParam (p, ParamIDs::bypass, (float) ((b / 150) % 2)); } },
        { "HPF / Air toggling", [] (auto& p, int b) { setParam (p, ParamIDs::hpf, (float) ((b / 150) % 2)); setParam (p, ParamIDs::air, (float) ((b / 150) % 2)); } },
        { "distance sweep (drag)", [] (auto& p, int b) { setParam (p, ParamIDs::distance, 100.0f * std::abs (std::sin ((float) b * 0.01f))); } },
    };

    // Reference: steady state at the loudest setting reached in these cases (0 cm, figure 8).
    ProximityEffectProcessor ref;
    setParam (ref, ParamIDs::distance, 0.0f);
    setParam (ref, ParamIDs::pattern, 4.0f);
    const float steadyStep = maxStep (render (ref, s, sr, block), (size_t) sr);

    for (const auto& c : cases)
    {
        ProximityEffectProcessor p;
        auto y = render (p, s, sr, block, [&] (int b) { c.automate (p, b); });
        const float step = maxStep (y, (size_t) block * 4);
        check (step < steadyStep * 1.25f, String (c.name) + ": max step " + String (step, 4) + " vs steady " + String (steadyStep, 4));
    }
}

void testStatePersistence()
{
    section ("state save / restore");
    ProximityEffectProcessor a;
    Random rnd (1234);
    for (auto* param : a.getParameters())
        param->setValueNotifyingHost (dynamic_cast<AudioParameterBool*> (param) != nullptr ? (rnd.nextBool() ? 1.0f : 0.0f)
                                                                                           : rnd.nextFloat());

    MemoryBlock state;
    a.getStateInformation (state);

    ProximityEffectProcessor b;
    b.setStateInformation (state.getData(), (int) state.getSize());
    const auto& pa = a.getParameters();
    const auto& pb = b.getParameters();
    check (pa.size() == pb.size() && pa.size() == 13, "13 parameters expected");
    for (int i = 0; i < pa.size(); ++i)
        check (std::abs (pa[i]->getValue() - pb[i]->getValue()) < 1.0e-5f,
               "parameter '" + pa[i]->getName (64) + "' should survive save/restore");

    // Garbage state must not crash nor change anything.
    const char junk[] = "not a plugin state";
    b.setStateInformation (junk, (int) sizeof (junk));
    check (std::abs (getParam (b, ParamIDs::distance) - getParam (a, ParamIDs::distance)) < 0.01f, "junk state ignored");

    // Every parameter is automatable and has a readable name.
    for (auto* param : a.getParameters())
        check (param->isAutomatable() && param->getName (64).isNotEmpty(), "parameter should be automatable and named");
    check (a.getBypassParameter() != nullptr, "bypass parameter exposed to the host");
}

void testEditor()
{
    section ("editor create / resize / destroy");
    ProximityEffectProcessor p;
    for (int round = 0; round < 3; ++round)
    {
        std::unique_ptr<AudioProcessorEditor> editor (p.createEditor());
        check (editor != nullptr && editor->getWidth() == 960 && editor->getHeight() == 640, "editor default size 960x640");
        editor->setSize (480, 320);
        editor->setSize (1920, 1280);
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        check (image.isValid(), "editor renders");
    }
}

void testCharacterArtwork()
{
    section ("character artwork");
    CharacterArtwork art;
    check (art.isValid() && art.getWidth() == 510 && art.getHeight() == 700, "artwork embedded and loaded (510x700)");
    if (! art.isValid())
        return;

    auto snapshot = [] (const Image& im) { return im.createCopy(); };
    auto diff = [] (const Image& a, const Image& b, Rectangle<int> area)
    {
        double sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto ca = a.getPixelAt (x, y), cb = b.getPixelAt (x, y);
                sum += std::abs (ca.getRed() - cb.getRed()) + std::abs (ca.getGreen() - cb.getGreen())
                     + std::abs (ca.getBlue() - cb.getBlue()) + std::abs (ca.getAlpha() - cb.getAlpha());
            }
        return sum / (area.getWidth() * area.getHeight());
    };

    CharacterArtwork::Pose rest;
    const auto restImage = snapshot (art.render (rest));
    const Rectangle<int> head (230, 80, 150, 160), legs (60, 480, 300, 100);

    // At rest the artwork is reproduced untouched.
    {
        CharacterArtwork reference;
        const Image::BitmapData raw (ImageCache::getFromMemory (BinaryData::character_png, BinaryData::character_pngSize)
                                         .convertedToFormat (Image::ARGB), Image::BitmapData::readOnly);
        double sum = 0.0;
        for (int y = 0; y < restImage.getHeight(); y += 3)
            for (int x = 0; x < restImage.getWidth(); x += 3)
                sum += std::abs ((int) restImage.getPixelAt (x, y).getAlpha() - (int) raw.getPixelColour (x, y).getAlpha());
        check (sum < 1.0, "rest pose should equal the original artwork");
    }

    CharacterArtwork::Pose p;
    p.puff = 1.0f;
    const auto puffed = snapshot (art.render (p));
    check (diff (puffed, restImage, head) > 2.0 && diff (puffed, restImage, legs) < 0.01, "puff changes the face only");

    p = {}; p.flush = 1.0f;
    const auto flushed = snapshot (art.render (p));
    double redGain = 0.0, greenLoss = 0.0;
    for (int y = 150; y < 170; ++y)
        for (int x = 265; x < 280; ++x)
        {
            redGain += flushed.getPixelAt (x, y).getRed() - restImage.getPixelAt (x, y).getRed();
            greenLoss += restImage.getPixelAt (x, y).getGreen() - flushed.getPixelAt (x, y).getGreen();
        }
    check (redGain >= 0.0 && greenLoss > 0.0, "flush makes the cheek redder");
    check (diff (flushed, restImage, { 200, 300, 250, 100 }) < 0.01, "flush leaves the hoodie alone");

    p = {}; p.anger = 1.0f;
    check (diff (art.render (p), restImage, head) > 1.0, "anger changes the face");

    p = {}; p.squash = 1.0f;
    const auto squashed = snapshot (art.render (p));
    check (diff (squashed, restImage, { 0, 600, 510, 100 }) < 0.5, "shoes and wheels stay on the floor");
    check (diff (squashed, restImage, head) > 1.0, "the upper body sinks with the chair");

    // Small pose changes give small image changes (no jumps between frames).
    // The image must change in proportion to the pose: a 2% step changes about half as much as a 4% one.
    auto posed = [] (float v) { CharacterArtwork::Pose q; q.squash = q.puff = q.flush = q.anger = v; return q; };
    const auto ia = snapshot (art.render (posed (0.50f)));
    const auto ib = snapshot (art.render (posed (0.52f)));
    const auto ic = snapshot (art.render (posed (0.54f)));
    const Rectangle<int> all (0, 0, 510, 700);
    const double small = diff (ib, ia, all), big = diff (ic, ia, all);
    check (small > 0.0 && small < 0.75 * big, "pose changes should be gradual (2%: " + String (small, 3) + ", 4%: " + String (big, 3) + ")");

    // Forward mapping lands where the inverse mapping comes from.
    p = {}; p.squash = 1.0f; p.puff = 1.0f; p.anger = 1.0f;
    const auto top = art.mapForward (CharacterArtwork::headTop, p);
    check (top.y > CharacterArtwork::headTop.y && top.x < CharacterArtwork::headTop.x,
           "head top moves down (chair) and toward the mic (lean)");

    // Render cost per changed frame.
    const auto start = Time::getMillisecondCounterHiRes();
    for (int i = 0; i < 30; ++i)
    {
        CharacterArtwork::Pose q;
        q.squash = q.puff = q.flush = q.anger = (float) i / 30.0f;
        art.render (q);
    }
    const double ms = (Time::getMillisecondCounterHiRes() - start) / 30.0;
    std::printf ("  render: %.2f ms per changed frame\n", ms);
    check (ms < 40.0, "artwork render should be fast enough for the 60 Hz UI timer, got " + String (ms, 2) + " ms");
}

void printReport()
{
    section ("report: gain in dB by distance and pattern");
    ProximityEffectProcessor p;
    const char* patterns[] = { "Omni", "Cardioid", "Super", "Hyper", "Fig8" };
    std::printf ("  latency @48k: %d samples\n", (p.prepareToPlay (48000.0, 512), p.getLatencySamples()));
    for (float dist : { 100.0f, 50.0f, 20.0f, 0.0f })
        for (int pat = 0; pat < 5; ++pat)
        {
            setParam (p, ParamIDs::distance, dist);
            setParam (p, ParamIDs::pattern, (float) pat);
            std::printf ("  %5.1f cm %-8s  60Hz %+6.2f  150Hz %+6.2f  1kHz %+6.2f  8kHz %+6.2f\n", dist, patterns[pat],
                         measure (p, 60), measure (p, 150), measure (p, 1000), measure (p, 8000));
        }
}

void writeSnapshots (const File& dir)
{
    section ("snapshots");
    dir.createDirectory();
    struct Shot { const char* name; float distance; bool angry; bool pushed; int pattern = 1; };
    for (auto shot : { Shot { "ui-far.png", 100.0f, false, false }, Shot { "ui-default.png", 60.0f, false, false },
                       Shot { "ui-mid.png", 50.0f, false, false }, Shot { "ui-30cm.png", 30.0f, false, false },
                       Shot { "ui-10cm.png", 10.0f, false, false }, Shot { "ui-close.png", 5.0f, false, false },
                       Shot { "ui-0cm-omni.png", 0.0f, false, false, 0 }, Shot { "ui-0cm-cardioid.png", 0.0f, false, false, 1 },
                       Shot { "ui-max.png", 0.0f, false, true, 4 }, Shot { "ui-angry.png", 40.0f, true, false },
                       Shot { "ui-angry-far.png", 90.0f, true, false } })
    {
        ProximityEffectProcessor p;
        setParam (p, ParamIDs::distance, shot.distance);
        setParam (p, ParamIDs::angry, shot.angry ? 1.0f : 0.0f);
        setParam (p, ParamIDs::pattern, (float) shot.pattern);
        if (shot.pushed)
        {
            setParam (p, ParamIDs::pattern, 4.0f);
            setParam (p, ParamIDs::lowEnd, 12.0f);
            setParam (p, ParamIDs::gainBoost, 12.0f);
        }
        std::unique_ptr<AudioProcessorEditor> editor (p.createEditor());
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto out = dir.getChildFile (shot.name);
        out.deleteFile();
        FileOutputStream stream (out);
        PNGImageFormat().writeImageToStream (image, stream);
        std::printf ("  wrote %s\n", out.getFullPathName().toRawUTF8());
    }
}
} // namespace

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add (argv[i]);

    testProximityCalibration();
    testFlatWhenFar();
    testPad();
    testToggles();
    testLatencyPolarityBypass();
    testSampleRatesAndBlockSizes();
    testMonoLayout();
    testSmoothAutomation();
    testStatePersistence();
    testEditor();
    testCharacterArtwork();

    if (args.contains ("--report"))
        printReport();
    if (const int i = args.indexOf ("--snapshots"); i >= 0 && i + 1 < args.size())
        writeSnapshots (File::getCurrentWorkingDirectory().getChildFile (args[i + 1]));

    std::printf ("\n%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}

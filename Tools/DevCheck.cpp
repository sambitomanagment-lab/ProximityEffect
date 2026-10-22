// Developer check: renders the editor to PNG and measures the DSP response.
// Built only with -DPE_BUILD_TOOLS=ON.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

using namespace juce;

static void setParam (ProximityEffectProcessor& p, const char* id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

// Steady-state gain in dB of a sine at `freq` through the plugin.
static float measure (ProximityEffectProcessor& p, float freq, float level = 0.05f)
{
    const double sr = 48000.0;
    const int block = 512, blocks = 60;
    p.prepareToPlay (sr, block);
    AudioBuffer<float> buf (2, block);
    MidiBuffer midi;
    double phase = 0.0;
    double sumOut = 0.0;
    int count = 0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            const float s = level * (float) std::sin (phase);
            phase += MathConstants<double>::twoPi * freq / sr;
            buf.setSample (0, i, s);
            buf.setSample (1, i, s);
        }
        p.processBlock (buf, midi);
        if (b > blocks / 2)
        {
            sumOut += buf.getRMSLevel (0, 0, block) * buf.getRMSLevel (0, 0, block);
            ++count;
        }
    }
    return Decibels::gainToDecibels ((float) std::sqrt (sumOut / count) / (level / std::sqrt (2.0f)));
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const String outDir = argc > 1 ? String (argv[1]) : File::getCurrentWorkingDirectory().getFullPathName();

    {
        ProximityEffectProcessor p;
        const char* patterns[] = { "Omni", "Cardioid", "Super", "Hyper", "Fig8" };
        for (float dist : { 100.0f, 50.0f, 20.0f, 0.0f })
            for (int pat = 0; pat < 5; ++pat)
            {
                setParam (p, ParamIDs::distance, dist);
                setParam (p, ParamIDs::pattern, (float) pat);
                std::printf ("dist %5.1f %-8s  60Hz %+6.2f  150Hz %+6.2f  1kHz %+6.2f  8kHz %+6.2f dB\n", dist, patterns[pat],
                             measure (p, 60), measure (p, 150), measure (p, 1000), measure (p, 8000));
            }

        setParam (p, ParamIDs::distance, 100.0f);
        setParam (p, ParamIDs::pattern, 1.0f);
        setParam (p, ParamIDs::pad, 1.0f);
        std::printf ("pad on:      1kHz %+6.2f dB\n", measure (p, 1000));
        setParam (p, ParamIDs::pad, 0.0f);
        setParam (p, ParamIDs::hpf, 1.0f);
        std::printf ("hpf on:      40Hz %+6.2f dB, 1kHz %+6.2f dB\n", measure (p, 40), measure (p, 1000));
        setParam (p, ParamIDs::hpf, 0.0f);
        setParam (p, ParamIDs::air, 1.0f);
        std::printf ("air on:      12kHz %+6.2f dB\n", measure (p, 12000));
        setParam (p, ParamIDs::air, 0.0f);
        setParam (p, ParamIDs::angry, 1.0f);
        std::printf ("latency @48k: %d samples\n", p.getLatencySamples());
        std::printf ("angry on:    1kHz %+6.2f dB (level 0.05), %+6.2f dB (level 0.5)\n", measure (p, 1000), measure (p, 1000, 0.5f));
    }

    for (auto [name, dist, angry] : { std::tuple<const char*, float, bool> { "ui-far.png", 80.0f, false },
                                      { "ui-close.png", 5.0f, false }, { "ui-angry.png", 30.0f, true } })
    {
        ProximityEffectProcessor p;
        setParam (p, ParamIDs::distance, dist);
        setParam (p, ParamIDs::angry, angry ? 1.0f : 0.0f);
        std::unique_ptr<AudioProcessorEditor> editor (p.createEditor());
        editor->setVisible (true);
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        File out = File (outDir).getChildFile (name);
        out.deleteFile();
        FileOutputStream stream (out);
        PNGImageFormat().writeImageToStream (image, stream);
        std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
    }
    return 0;
}

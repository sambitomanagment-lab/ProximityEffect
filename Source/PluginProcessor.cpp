#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float minusInfDb = -60.0f;

    float dbToGain (float db) noexcept
    {
        return db <= minusInfDb ? 0.0f : juce::Decibels::decibelsToGain (db);
    }

    // 0 cm = glued to the pop filter, 100 cm = too far. The proximity boost grows
    // roughly with 1/r, so most of the action happens in the last centimetres.
    float closenessFromDistance (float cm) noexcept
    {
        return std::pow (1.0f - juce::jlimit (0.0f, 100.0f, cm) / 100.0f, 1.6f);
    }
}

ProximityEffectProcessor::ProximityEffectProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pDistance = apvts.getRawParameterValue (ParamIDs::distance);
    pPattern = apvts.getRawParameterValue (ParamIDs::pattern);
    pInput = apvts.getRawParameterValue (ParamIDs::inputGain);
    pOutput = apvts.getRawParameterValue (ParamIDs::output);
    pLowEnd = apvts.getRawParameterValue (ParamIDs::lowEnd);
    pBoost = apvts.getRawParameterValue (ParamIDs::gainBoost);
    pMix = apvts.getRawParameterValue (ParamIDs::mix);
    pHpf = apvts.getRawParameterValue (ParamIDs::hpf);
    pAir = apvts.getRawParameterValue (ParamIDs::air);
    pAngry = apvts.getRawParameterValue (ParamIDs::angry);
    pPad = apvts.getRawParameterValue (ParamIDs::pad);
    pPolarity = apvts.getRawParameterValue (ParamIDs::polarity);
    pBypass = apvts.getRawParameterValue (ParamIDs::bypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout ProximityEffectProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto dbAttr = [] (int decimals)
    {
        return AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([decimals] (float v, int)
            {
                return v <= minusInfDb ? String ("-inf") : String (v, decimals);
            });
    };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::distance, 1 }, "Distance",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f,
                                                       AudioParameterFloatAttributes().withLabel ("cm")));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::pattern, 1 }, "Polar Pattern",
                                                        StringArray { "Omni", "Cardioid", "Supercardioid", "Hypercardioid", "Figure 8" }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::inputGain, 1 }, "Input Gain",
                                                       NormalisableRange<float> (minusInfDb, 20.0f, 0.1f, 2.0f), 0.0f, dbAttr (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                                                       NormalisableRange<float> (minusInfDb, 12.0f, 0.1f, 2.5f), 0.0f, dbAttr (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::lowEnd, 1 }, "Low-End Push",
                                                       NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dbAttr (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::gainBoost, 1 }, "Gain Boost",
                                                       NormalisableRange<float> (0.0f, 12.0f, 0.1f), 4.0f, dbAttr (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::hpf, 1 }, "HPF 80 Hz", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::air, 1 }, "Air Mode", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::angry, 1 }, "Angry", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::pad, 1 }, "Ego Killer -20dB", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::polarity, 1 }, "Polarity Invert", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::bypass, 1 }, "Bypass", false));

    return layout;
}

float ProximityEffectProcessor::patternFactor (int index) noexcept
{
    // Omni is pure pressure (no proximity), figure 8 is pure gradient (maximum).
    static constexpr float factors[] = { 0.0f, 0.48f, 0.6f, 0.72f, 1.0f };
    return factors[juce::jlimit (0, 4, index)];
}

bool ProximityEffectProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void ProximityEffectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseRate = sampleRate;
    preparedBlockSize = juce::jmax (1, samplesPerBlock);
    const auto numChannels = (size_t) juce::jmax (1, getTotalNumOutputChannels());

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        numChannels, oversamplingFactorLog2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling->initProcessing ((size_t) preparedBlockSize);
    osRate = baseRate * (double) oversampling->getOversamplingFactor();
    latency = (int) std::round (oversampling->getLatencyInSamples());
    setLatencySamples (latency);
    dryDelay.setSize ((int) numChannels, latency + 1);
    dryDelay.clear();
    delayedScratch.setSize ((int) numChannels, preparedBlockSize);
    dryDelayWrite = 0;

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        hpf[ch].setHighPass (baseRate, 80.0, 0.707);
        air[ch].setHighShelf (baseRate, juce::jmin (10000.0, baseRate * 0.4), 0.7, 5.0);
        angryPreEmph[ch].setPeak (osRate, 1600.0, 0.8, 6.0);
        angryTame[ch].setHighShelf (osRate, 5000.0, 0.7, -8.0);

        for (auto* f : { &hpf[ch], &air[ch], &proximityShelf[ch], &chestBump[ch], &angryPreEmph[ch], &angryTame[ch] })
            f->reset();
    }

    auto prime = [] (juce::SmoothedValue<float>& s, double rate, float value)
    {
        s.reset (rate, 0.05);
        s.setCurrentAndTargetValue (value);
    };

    const float inGain = dbToGain (pInput->load()) * (pPad->load() > 0.5f ? 0.1f : 1.0f)
                       * (pPolarity->load() > 0.5f ? -1.0f : 1.0f);
    prime (inputGainSm, baseRate, inGain);
    prime (outputGainSm, baseRate, dbToGain (pOutput->load()));
    prime (hpfBlend, baseRate, pHpf->load());
    prime (airBlend, baseRate, pAir->load());
    prime (bypassBlend, baseRate, pBypass->load());

    prime (amountSm, osRate, closenessFromDistance (pDistance->load()));
    prime (patternSm, osRate, patternFactor ((int) pPattern->load()));
    prime (lowEndSm, osRate, pLowEnd->load());
    prime (boostSm, osRate, pBoost->load());
    prime (mixSm, osRate, pMix->load() / 100.0f);
    prime (angryBlend, osRate, pAngry->load());

    updateWetFilters (amountSm.getCurrentValue(), patternSm.getCurrentValue(), lowEndSm.getCurrentValue());
}

void ProximityEffectProcessor::updateWetFilters (float amount, float pattern, float lowEndDb)
{
    // The bass corner climbs from ~90 Hz to ~250 Hz as the source gets closer.
    const double shelfFreq = 90.0 + 160.0 * amount;
    // Low-End Push at 0 gives ~+10 dB of bass (figure 8) / ~+5 dB (cardioid) at the pop filter.
    const double shelfGain = amount * pattern * (11.5 + lowEndDb);
    const double bumpGain = amount * pattern * 3.0;

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        proximityShelf[ch].setLowShelf (osRate, shelfFreq, 0.6, shelfGain);
        chestBump[ch].setPeak (osRate, 200.0, 1.0, bumpGain);
    }
}

void ProximityEffectProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    process (buffer, false);
}

void ProximityEffectProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    process (buffer, true);
}

juce::AudioProcessorParameter* ProximityEffectProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParamIDs::bypass);
}

void ProximityEffectProcessor::process (juce::AudioBuffer<float>& buffer, bool forceBypass)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels, dryDelay.getNumChannels());
    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (numChannels == 0 || oversampling == nullptr)
        return;

    inputGainSm.setTargetValue (dbToGain (pInput->load()) * (pPad->load() > 0.5f ? 0.1f : 1.0f)
                                * (pPolarity->load() > 0.5f ? -1.0f : 1.0f));
    outputGainSm.setTargetValue (dbToGain (pOutput->load()));
    hpfBlend.setTargetValue (pHpf->load());
    airBlend.setTargetValue (pAir->load());
    amountSm.setTargetValue (closenessFromDistance (pDistance->load()));
    patternSm.setTargetValue (patternFactor ((int) pPattern->load()));
    lowEndSm.setTargetValue (pLowEnd->load());
    boostSm.setTargetValue (pBoost->load());
    mixSm.setTargetValue (pMix->load() / 100.0f);
    angryBlend.setTargetValue (pAngry->load());
    bypassBlend.setTargetValue (forceBypass || pBypass->load() > 0.5f ? 1.0f : 0.0f);

    float peak = 0.0f;
    const int maxChunk = preparedBlockSize; // the oversampler can't take more than it was prepared for

    for (int start = 0; start < buffer.getNumSamples(); start += maxChunk)
    {
        const int n = juce::jmin (maxChunk, buffer.getNumSamples() - start);
        float* channels[maxChannels] = {};
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch] = buffer.getWritePointer (ch, start);

        // Keep a latency-aligned copy of the untouched input for bypass.
        float* delayed[maxChannels] = {};
        for (int ch = 0; ch < numChannels; ++ch)
        {
            delayed[ch] = delayedScratch.getWritePointer (ch);
            float* ring = dryDelay.getWritePointer (ch);
            int w = dryDelayWrite;
            for (int i = 0; i < n; ++i)
            {
                ring[w] = channels[ch][i];
                w = (w + 1) % dryDelay.getNumSamples();
                delayed[ch][i] = ring[w];
            }
        }
        dryDelayWrite = (dryDelayWrite + n) % dryDelay.getNumSamples();

        // --- Input stage (base rate): pad, input gain, polarity, HPF ---
        for (int i = 0; i < n; ++i)
        {
            const float g = inputGainSm.getNextValue();
            const float h = hpfBlend.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float x = channels[ch][i] * g;
                const float filtered = hpf[ch].process (x);
                channels[ch][i] = x + h * (filtered - x);
            }
        }

        // --- Proximity + saturation (oversampled) ---
        juce::dsp::AudioBlock<float> block (channels, (size_t) numChannels, (size_t) n);
        auto osBlock = oversampling->processSamplesUp (block);
        const int osN = (int) osBlock.getNumSamples();

        for (int i = 0; i < osN; ++i)
        {
            const float amount = amountSm.getNextValue();
            const float pattern = patternSm.getNextValue();
            const float lowEnd = lowEndSm.getNextValue();
            const float boostDb = boostSm.getNextValue();
            const float mix = mixSm.getNextValue();
            const float angry = angryBlend.getNextValue();

            if (i % coeffUpdateInterval == 0)
                updateWetFilters (amount, pattern, lowEnd);

            const float boost = juce::Decibels::decibelsToGain (amount * boostDb);
            const float k = 1.0f + 1.5f * amount;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* d = osBlock.getChannelPointer ((size_t) ch);
                const float dry = d[i];

                float wet = chestBump[ch].process (proximityShelf[ch].process (dry)) * boost;
                // Gentle "too close" saturation that fades in with proximity.
                wet += amount * (std::tanh (k * wet) / k - wet);

                // Angry: someone yelling into the capsule.
                const float emph = angryPreEmph[ch].process (wet) * 8.0f;
                const float smashed = angryTame[ch].process ((std::tanh (emph + 0.25f) - std::tanh (0.25f)) * 0.28f);
                wet += angry * (smashed - wet);

                d[i] = dry + mix * (wet - dry);
            }
        }

        oversampling->processSamplesDown (block);

        // --- Output stage (base rate): air, output gain ---
        for (int i = 0; i < n; ++i)
        {
            const float a = airBlend.getNextValue();
            const float g = outputGainSm.getNextValue();
            const float bp = bypassBlend.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float x = channels[ch][i];
                float y = (x + a * (air[ch].process (x) - x)) * g;
                y += bp * (delayed[ch][i] - y);
                channels[ch][i] = y;
                peak = juce::jmax (peak, std::abs (y));
            }
        }
    }

    outputPeak.store (juce::jmax (peak, outputPeak.load() * 0.9f));
}

juce::AudioProcessorEditor* ProximityEffectProcessor::createEditor()
{
    return new ProximityEffectEditor (*this);
}

void ProximityEffectProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ProximityEffectProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProximityEffectProcessor();
}

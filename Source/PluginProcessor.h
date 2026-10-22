#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Biquad.h"

namespace ParamIDs
{
    inline constexpr auto distance  = "distance";
    inline constexpr auto pattern   = "pattern";
    inline constexpr auto inputGain = "inputGain";
    inline constexpr auto output    = "output";
    inline constexpr auto lowEnd    = "lowEnd";
    inline constexpr auto gainBoost = "gainBoost";
    inline constexpr auto mix       = "mix";
    inline constexpr auto hpf       = "hpf";
    inline constexpr auto air       = "air";
    inline constexpr auto angry     = "angry";
    inline constexpr auto pad       = "pad";
    inline constexpr auto polarity  = "polarity";
    inline constexpr auto bypass    = "bypass";
}

class ProximityEffectProcessor : public juce::AudioProcessor
{
public:
    ProximityEffectProcessor();
    ~ProximityEffectProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // How much proximity a pattern produces (pressure-gradient share of the capsule).
    static float patternFactor (int index) noexcept;

    juce::AudioProcessorValueTreeState apvts;

    // Peak level after processing, for the GUI (0..1+).
    std::atomic<float> outputPeak { 0.0f };

private:
    static constexpr int maxChannels = 2;
    static constexpr int oversamplingFactorLog2 = 2; // 4x for the saturation stages
    static constexpr int coeffUpdateInterval = 16;

    void updateWetFilters (float amount, float pattern, float lowEndDb);
    void process (juce::AudioBuffer<float>&, bool forceBypass);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    double baseRate = 44100.0, osRate = 176400.0;
    int preparedBlockSize = 512;

    // Input delayed by the oversampling latency, so bypass stays time-aligned.
    juce::AudioBuffer<float> dryDelay, delayedScratch;
    int dryDelayWrite = 0, latency = 0;

    Biquad hpf[maxChannels], air[maxChannels];
    Biquad proximityShelf[maxChannels], chestBump[maxChannels];
    Biquad angryPreEmph[maxChannels], angryTame[maxChannels];

    juce::SmoothedValue<float> inputGainSm, outputGainSm, hpfBlend, airBlend;
    juce::SmoothedValue<float> bypassBlend;
    juce::SmoothedValue<float> amountSm, patternSm, lowEndSm, boostSm, mixSm, angryBlend;

    std::atomic<float>* pDistance = nullptr;
    std::atomic<float>* pPattern = nullptr;
    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pLowEnd = nullptr;
    std::atomic<float>* pBoost = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pHpf = nullptr;
    std::atomic<float>* pAir = nullptr;
    std::atomic<float>* pAngry = nullptr;
    std::atomic<float>* pPad = nullptr;
    std::atomic<float>* pPolarity = nullptr;
    std::atomic<float>* pBypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProximityEffectProcessor)
};

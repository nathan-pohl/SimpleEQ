/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings {
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

// Use a bunch of aliases here to shorten down all the JUCE namespaces
using Filter = juce::dsp::IIR::Filter<float>;
using Coefficients = Filter::CoefficientsPtr;
// each fiter type has a response type of 12db/Oct. If we want a filter to do 48db/Oct, then we need to chain together 4 filters.
// This will require using a dsp processor chain to process all the audio as if it were a 48db/Oct filter.
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
// First CutFilter is lowpass, mid filter is the bandpass (peak filter), last filter is the highpass
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions {
    LowCut,
    Peak,
    HighCut
};

void updateCoefficients(Coefficients& old, const Coefficients& replacements);
template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& lowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                       sampleRate,
                                                                                       2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                      sampleRate,
                                                                                      2 * (chainSettings.lowCutSlope + 1));
}
//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout()};

private:
    
    // must create two chains, one for left and right audio for full stereo
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings& chainSettings);
    
    void updateLowCutFilter(const ChainSettings& chainSettings);
    void updateHighCutFilter(const ChainSettings& chainSettings);

    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};

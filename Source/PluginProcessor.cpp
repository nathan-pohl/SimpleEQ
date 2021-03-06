/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    // prepare the filters for each side
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    // Oscillator for testing spectrum analyzer
    //osc.initialise([](float x) { return std::sin(x); });
    //spec.numChannels = getTotalNumOutputChannels();
    //osc.prepare(spec);
    //osc.setFrequency(10000);
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    int totalNumInputChannels  = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    // Use with the oscillator to test the spectrum analysis
    //buffer.clear();
    //juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    //osc.process(stereoContext);

    juce::dsp::AudioBlock<float> leftBlock = block.getSingleChannelBlock(0);
    juce::dsp::AudioBlock<float> rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    // save the state of the plugin
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // retrieve state of plugin from memory
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateFilters();
    }
    
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue(LOW_CUT_FREQ_NAME)->load();
    settings.highCutFreq = apvts.getRawParameterValue(HIGH_CUT_FREQ_NAME)->load();
    settings.peakFreq = apvts.getRawParameterValue(PEAK_FREQ_NAME)->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue(PEAK_GAIN_NAME)->load();
    settings.peakQuality = apvts.getRawParameterValue(PEAK_QUALITY_NAME)->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue(LOW_CUT_SLOPE_NAME)->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue(HIGH_CUT_SLOPE_NAME)->load());
    settings.lowCutBypassed = apvts.getRawParameterValue(LOW_CUT_BYPASS_NAME)->load() > 0.5f; // If greater than .5, then true, else false
    settings.peakBypassed = apvts.getRawParameterValue(PEAK_BYPASS_NAME)->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue(HIGH_CUT_BYPASS_NAME)->load() > 0.5f;

    return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peakFreq,
                                                               chainSettings.peakQuality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels)); // helper function to convert the value of peak gain to decibels
}


void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings) {
    Coefficients peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    // link to the middle link in the chain (ChainPositions::Peak), the peak filter
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    // the IIR::Coefficients helper functions return instances allocated on the heap, so they must be dereferenced
    *old = *replacements;
}

void SimpleEQAudioProcessor::updateLowCutFilter(const ChainSettings& chainSettings) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    CutFilter& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    CutFilter& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void SimpleEQAudioProcessor::updateHighCutFilter(const ChainSettings& chainSettings) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    CutFilter& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    CutFilter& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters() {
    ChainSettings chainSettings = getChainSettings(apvts);
    updateLowCutFilter(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilter(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(LOW_CUT_FREQ_NAME,
                                                           LOW_CUT_FREQ_NAME,
                                                           juce::NormalisableRange<float>(FILTER_MIN_HZ, FILTER_MAX_HZ, FILTER_FREQUENCY_INTERVAL, FILTER_FREQUENCY_SKEW_FACTOR), 
                                                           LOW_CUT_FILTER_DEFAULT));

    layout.add(std::make_unique<juce::AudioParameterFloat>(HIGH_CUT_FREQ_NAME,
                                                           HIGH_CUT_FREQ_NAME,
                                                           juce::NormalisableRange<float>(FILTER_MIN_HZ, FILTER_MAX_HZ, FILTER_FREQUENCY_INTERVAL, FILTER_FREQUENCY_SKEW_FACTOR),
                                                           HIGH_CUT_FILTER_DEFAULT));

    // mid band EQ
    layout.add(std::make_unique<juce::AudioParameterFloat>(PEAK_FREQ_NAME,
                                                           PEAK_FREQ_NAME,
                                                           juce::NormalisableRange<float>(FILTER_MIN_HZ, FILTER_MAX_HZ, FILTER_FREQUENCY_INTERVAL, FILTER_FREQUENCY_SKEW_FACTOR), 
                                                           PEAK_FILTER_DEFAULT));

    layout.add(std::make_unique<juce::AudioParameterFloat>(PEAK_GAIN_NAME,
                                                           PEAK_GAIN_NAME,
                                                           juce::NormalisableRange<float>(PEAK_GAIN_MIN_DB, PEAK_GAIN_MAX_DB, PEAK_GAIN_INTERVAL, DEFAULT_SKEW_FACTOR),
                                                           PEAK_GAIN_DEFAULT));
    
    // "Q" factor of mid band
    layout.add(std::make_unique<juce::AudioParameterFloat>(PEAK_QUALITY_NAME,
                                                           PEAK_QUALITY_NAME,
                                                           juce::NormalisableRange<float>(PEAK_QUALITY_MIN, PEAK_QUALITY_MAX, PEAK_QUALITY_INTERVAL, DEFAULT_SKEW_FACTOR),
                                                           PEAK_QUALITY_DEFAULT));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) {
        juce::String str;
        str << (12 + i * 12);
        str << " " << DB_PER_OCT;
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>(LOW_CUT_SLOPE_NAME, LOW_CUT_SLOPE_NAME, stringArray, SLOPE_DEFAULT_POS));
    layout.add(std::make_unique<juce::AudioParameterChoice>(HIGH_CUT_SLOPE_NAME, HIGH_CUT_SLOPE_NAME, stringArray, SLOPE_DEFAULT_POS));

    layout.add(std::make_unique<juce::AudioParameterBool>(LOW_CUT_BYPASS_NAME, LOW_CUT_BYPASS_NAME, BYPASS_DEFAULT));
    layout.add(std::make_unique<juce::AudioParameterBool>(PEAK_BYPASS_NAME, PEAK_BYPASS_NAME, BYPASS_DEFAULT));
    layout.add(std::make_unique<juce::AudioParameterBool>(HIGH_CUT_BYPASS_NAME, HIGH_CUT_BYPASS_NAME, BYPASS_DEFAULT));
    layout.add(std::make_unique<juce::AudioParameterBool>(ANALYZER_ENABLED_NAME, ANALYZER_ENABLED_NAME, ENABLED_DEFAULT));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}

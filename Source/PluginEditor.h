/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder {
    // Splits spectrum of 20Hz - 20000Hz into N equally sized frequency bins
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};
// Fast Fourier Transform for converting audio buffer data into FastFourierTransform DataBlocks
// According to the course:
    // Host Buffer has x samples -> SingleChannelSampleFifo -> Fixed size Blocks -> Fast Fourier Transform DataGenerator ->
    // FastFourierTransform DataBlocks -> PathProducer -> Juce::Path -> which is consumed by the GUI to draw the Spectrum Analysis Curve
template<typename BlockType>
struct FFTDataGenerator {
    /**
    Produces the FFT data from an audio buffer.
    */
    void produceFFtDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity) {
        const int fftSize = getFFtSize();
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);        // [1]
        // then render our FFT data
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());   // [2]

        int numBins = (int)fftSize / 2;
        // TODO combine the below into the same for loop, probably makes no difference
        //normalize the fft values
        for (int i = 0; i < numBins; ++i) {
            fftData[i] /= (float)numBins;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i) {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder) {
        // when you change order, recreate the window, forwardFFT, fifo, fftData
        // also reset the fifoIndex
        // things that need recreating should be created on the heap via std::make_unique<>
        order = newOrder;
        int fftSize = getFFtSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);
        fftDataFifo.prepare(fftData.size());
    }

    //==============================================================================
    int getFFtSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }

private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

// Generates the Path data for the Spectrum Analysis by being fed the fft data
template<typename PathType>
struct AnalyzerPathGenerator {
    // Converts 'renderData[]' into a juce::Path
    void generatePath(const std::vector<float>& renderData, juce::Rectangle<float> fftBounds, int fftSize, float binWidth, float negativeInfinity) {
        float top = fftBounds.getY();
        float bottom = fftBounds.getHeight();
        float width = fftBounds.getWidth();
        int numBins = (int)fftSize / 2;
        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v) {
            return juce::jmap(v, negativeInfinity, 0.f, float(bottom), top);
        };

        auto y = map(renderData[0]);
        // assert that y is a number and is not infinite
        jassert(!std::isnan(y) && !std::isinf(y));
        // start the path here
        p.startNewSubPath(0, y);
        const int pathResolution = 2; // you can draw line-to's every 'pathResolution' pixels.

        // create the rest of the path here
        for (int binNum = 1; binNum < numBins; binNum += pathResolution) {
            y = map(renderData[binNum]);
            // assert that y is a number and is not infinite
            jassert(!std::isnan(y) && !std::isinf(y));

            if (!std::isnan(y) && !std::isinf(y)) {
                float binFreq = binNum * binWidth;
                float normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }
        pathFifo.push(p);
    }

    int getNumPathsAvailable() const {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path) {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4 {
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g, 
                          juce::ToggleButton& toggleButton, 
                          bool shouldDrawButtonAsHighlighted, 
                          bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider {
    RotarySliderWithLabels(juce::RangedAudioParameter& param, const juce::String suffixStr) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&param),
        suffix(suffixStr)
    {
        setLookAndFeel(&lookAndFeel);
    }

    ~RotarySliderWithLabels() {
        setLookAndFeel(nullptr);
    }

    struct LabelPos {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lookAndFeel;

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct PathProducer {
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf) : channelFifo(&scsf) {
        // use order of 2048 for average resolution in lower end of spectrum
        // e.g. 48000 sample rate / 2048 order = 23Hz resolution
        // meaning not a lot of resolution at bass frequencies, but high resolution at high frequencies
        // using higher order rates gives better resolution at lower frequencies, at the expense of more CPU
        fftDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, fftDataGenerator.getFFtSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return fftPath; }
private:
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* channelFifo;
    juce::AudioBuffer<float> monoBuffer;
    FFTDataGenerator<std::vector<float>> fftDataGenerator;
    AnalyzerPathGenerator<juce::Path> pathProducer;
    juce::Path fftPath;
};

struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
public:
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { };
    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;

    void updateChain();

    juce::Image background;
    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;
};

//==============================================================================
// We have two different kinds of buttons we want to use, but want to use the same LookAndFeel functions between them.
// Using these inherited classes to make it easier to determine between the two
struct PowerButton : juce::ToggleButton { };
struct AnalyzerButton : juce::ToggleButton{ };
//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    RotarySliderWithLabels peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, highCutSlopeSlider;
    PowerButton lowCutBypassButton, peakBypassButton, highCutBypassButton;
    AnalyzerButton analyzerEnabledButton;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    Attachment peakFreqSliderAttachment, 
               peakGainSliderAttachment, 
               peakQualitySliderAttachment, 
               lowCutFreqSliderAttachment, 
               highCutFreqSliderAttachment, 
               lowCutSlopeSliderAttachment, 
               highCutSlopeSliderAttachment;

    ButtonAttachment lowCutBypassButtonAttachment, 
                     peakBypassButtonAttachment, 
                     highCutBypassButtonAttachment, 
                     analyzerEnabledButtonAttachment;

    std::vector<juce::Component*> getComponents();

    LookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};

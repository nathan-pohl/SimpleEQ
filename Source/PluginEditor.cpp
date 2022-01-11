/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;
    bool enabled = slider.isEnabled();

    // background of the Slider
    Rectangle<float> bounds = Rectangle<float>(x, y, width, height);
    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey); // Purple background of ellipse, dark grey if slider is diabled
    g.fillEllipse(bounds);
    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey); // Orange border of ellipse, grey if slider is disabled
    g.drawEllipse(bounds, ELLIPSE_DEFAULT_THICKNESS);

    if (RotarySliderWithLabels* rswl = dynamic_cast<RotarySliderWithLabels*> (&slider)) {
        jassert(rotaryStartAngle < rotaryEndAngle); // Check to make sure the angles are set right

        // Draw the rectangle that forms the pointer of the rotary dial // TODO it is not drawing properly
        Point<float> center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX() - SLIDER_X_PADDING); // Two pixels left of center
        r.setRight(center.getX() + SLIDER_X_PADDING); // Two pixels to right of center
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        p.addRoundedRectangle(r, ROUNDED_RECTANGLE_THICKNESS);

        float sliderAngleRadians = jmap(sliderPosProportional, SLIDER_MIN_RANGE, SLIDER_MAX_RANGE, rotaryStartAngle, rotaryEndAngle); // Map the slider angle to be between the bounding angles
        p.applyTransform(AffineTransform().rotated(sliderAngleRadians, center.getX(), center.getY()));
        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        juce::String text = rswl->getDisplayString();
        int strWidth = g.getCurrentFont().getStringWidth(text);

        // Draw bounding box for text
        r.setSize(strWidth + TEXT_BOUNDING_BOX_ADD_WIDTH, rswl->getTextHeight() + TEXT_BOUNDING_BOX_ADD_HEIGHT);
        r.setCentre(center);
        g.setColour(enabled ? Colours::black : Colours::darkgrey); // Text background, use darkgrey if slider is disabled
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey); // Text color, use light grey if slider is disabled
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, NUMBER_OF_LINES_TEXT);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    using namespace juce;

    if (PowerButton* pb = dynamic_cast<PowerButton*>(&toggleButton)) {
        Path powerButton;
        Rectangle<int> bounds = toggleButton.getLocalBounds();
        int size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        Rectangle<float> rect = bounds.withSizeKeepingCentre(size, size).toFloat();

        float angle = 30.f;
        size -= 6;

        // These lines draw on the button to create the typical power button symbol. The arc creates that incomplete circle within the button, and the subPath and lineTo draw the line from top to center 
        powerButton.addCentredArc(rect.getCentreX(), rect.getCentreY(), size * 0.5, size * 0.5, 0.f, degreesToRadians(angle), degreesToRadians(360.f - angle), true);
        powerButton.startNewSubPath(rect.getCentreX(), rect.getY());
        powerButton.lineTo(rect.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        // if button is on, then band is bypassed and we should use grey, otherwise, use green to indicate the band is engaged
        juce::Colour color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(color);
        g.strokePath(powerButton, pst);
        // One thing that could be added to the button is making sure the toggle only happens when clicked inside where the button is actually drawn, 
        // rather than it's bounding box which is much bigger than the ellipse drawn here
        g.drawEllipse(rect, ELLIPSE_THICKER_OUTLINE);
    }
    else if (AnalyzerButton* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton)) {
        // Analyzer button works opposite to power button so if button is off, then the analyzer is on and we should use green
        Colour color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(color);

        Rectangle<int> bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        g.strokePath(analyzerButton->randomPath, PathStrokeType(PATH_STROKE_THICKNESS));
    }
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g) {
    using namespace juce;
    // 7 o'clock is where slider draws value of zero, 5 o'clock is where slider draws value of one
    float startAngle = degreesToRadians(180.f + 45.f); // 7 o'clock basically
    float endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi; // 5 o'clock basically - needs a full rotation to put it at the right angle though, otherwise slider will go the wrong way

    Range<double> range = getRange();
    Rectangle<int> sliderBounds = getSliderBounds();
    // Draw bounds of the slider
    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);
    getLookAndFeel().drawRotarySlider(g, 
                                      sliderBounds.getX(), 
                                      sliderBounds.getY(), 
                                      sliderBounds.getWidth(), 
                                      sliderBounds.getHeight(), 
                                      jmap(getValue(), range.getStart(), range.getEnd(), SLIDER_MIN_RANGE_DOUBLE, SLIDER_MAX_RANGE_DOUBLE),  // normalize values in the range
                                      startAngle, 
                                      endAngle, 
                                      *this);

    Point<float> center = sliderBounds.toFloat().getCentre();
    float radius = sliderBounds.getWidth() * UI_BOUNDS_HALF;
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    
    int numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i) {
        float pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        // This logic determines where to place the labels for the bounding points for each slider (e.g. 20 Hz to 20KHz)
        float angle = jmap(pos, 0.f, 1.f, startAngle, endAngle);
        Point<float> c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, angle);
        Rectangle<float> r;
        String str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, NUMBER_OF_LINES_TEXT);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    juce::Rectangle<int> bounds = getLocalBounds();
    int size = juce::jmin(bounds.getWidth(), bounds.getHeight()); // Make our sliders square by getting the minimum size from the width and height
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    return r;
}

//int RotarySliderWithLabels::getTextHeight() const { return 14; }
juce::String RotarySliderWithLabels::getDisplayString() const {
    // If the parameter is a choice parameter, show the choice name e.g. 12 db/Oct
    if (juce::AudioParameterChoice* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false; // For Kilohertz

    // Only Float parameters are supported here
    if (juce::AudioParameterFloat* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
        float val = getValue();
        if (val > 999.f) {
            val /= 1000.f;
            addK = true; // Use KHz for units
        }
        str = juce::String(val, (addK ? 2 : 0)); // If we are using KHz, only use 2 decimal places
        if (suffix.isNotEmpty()) {
            str << " ";
            if (addK) {
                str << "K";
            }
            str << suffix;
        }

        return str;
    }
    else {
        jassertfalse;
    }
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : 
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo) {
    const juce::Array <juce::AudioProcessorParameter*> &params = audioProcessor.getParameters();
    for (juce::AudioProcessorParameter* param : params) {
        param->addListener(this);
    }
    // update the monochain
    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const juce::Array <juce::AudioProcessorParameter*>& params = audioProcessor.getParameters();
    for (juce::AudioProcessorParameter* param : params) {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

// This is where we need to coordinate the SingleChannelSampleFifo, FastFourierTransform Data generator, Path Producer, and GUI for Spectrum Analysis
void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate) {
    juce::AudioBuffer<float> tempIncomingBuffer;

    // When consuming the buffer, we take a number of sampled points of the sample size, run the FFT algorithm on that block, 
    // then shift the buffer forward by that sample size to take on the next block of sample points
    while (channelFifo->getNumcompleteBuffersAvailable() > 0) {
        if (channelFifo->getAudioBuffer(tempIncomingBuffer)) {
            // first shift everything in the monoBuffer forward by however many samples are in the temp buffer
            int size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);

            fftDataGenerator.produceFFtDataForRendering(monoBuffer, ABSOLUTE_MINIMUM_GAIN); // Our scale only goes to -48dB, so we'll use that as our "negative infinity" for now
        }
    }

    // If there are FFT data buffers to pull, if we can pull a buffer, generate a path
    const auto fftSize = fftDataGenerator.getFFtSize();
    // 48000 sample rate / 2048 order = 23Hz resolution <- this is the bin width
    const double binWidth = sampleRate / (double)fftSize;

    while (fftDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
        std::vector<float> fftData;
        if (fftDataGenerator.getFFTData(fftData)) {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, ABSOLUTE_MINIMUM_GAIN); // Our sepctrum graph only goes to -48dB, so ue that as the "negative infinity" for now
        }
    }

    // Pull the most recent path that has been produced, since that will be the most recent data to use - this is in case we can't pull the paths as fast as we make them
    while (pathProducer.getNumPathsAvailable()) {
        pathProducer.getPath(fftPath);
    }
}

void ResponseCurveComponent::timerCallback() {
    if (shouldShowFFTAnlaysis) {
        juce::Rectangle<float> fftBounds = getAnalysisArea().toFloat();
        double sampleRate = audioProcessor.getSampleRate();

        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if (parametersChanged.compareAndSetBool(false, true)) {
        DBG("params changed");
        // update the monochain
        updateChain();
        // signal a repaint
        //repaint();
    }
    // Now need to repaint all the time since we have new spectrum analysis paths being made all the time
    repaint();
}

void ResponseCurveComponent::updateChain() {
    ChainSettings chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    Coefficients peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g) {
    // doing this so we don't have to write `juce::` everywhere in this function
    using namespace juce;
    g.fillAll(Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    Rectangle<int> responseArea = getAnalysisArea();

    int width = responseArea.getWidth();
    CutFilter& lowcut = monoChain.get<ChainPositions::LowCut>();
    Filter& peak = monoChain.get<ChainPositions::Peak>();
    CutFilter& highcut = monoChain.get<ChainPositions::HighCut>();

    double sampleRate = audioProcessor.getSampleRate();
    std::vector<double> magnitudes;
    magnitudes.resize(width);

    for (int i = 0; i < width; ++i) {
        double magnitude = 1.f;
        double freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>()) {
            magnitude *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        // check each filter in the CutChains
        if (!monoChain.isBypassed<ChainPositions::LowCut>()) {
            if (!lowcut.isBypassed<0>()) {
                magnitude *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!lowcut.isBypassed<1>()) {
                magnitude *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!lowcut.isBypassed<2>()) {
                magnitude *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!lowcut.isBypassed<3>()) {
                magnitude *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>()) {
            if (!highcut.isBypassed<0>()) {
                magnitude *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!highcut.isBypassed<1>()) {
                magnitude *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!highcut.isBypassed<2>()) {
                magnitude *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
            if (!highcut.isBypassed<3>()) {
                magnitude *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            }
        }

        magnitudes[i] = Decibels::gainToDecibels(magnitude);
    }

    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input) {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));

    for (size_t i = 1; i < magnitudes.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }

    if (shouldShowFFTAnlaysis) {
        Path leftChannelFFTPath = leftPathProducer.getPath();
        Path rightChannelFFTPath = rightPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

        // draw the left path using sky blue
        g.setColour(Colours::skyblue);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
        // draw the right path using light yellow
        g.setColour(Colours::lightyellow);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized() {
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);
    g.setColour(Colours::white);
    juce::Rectangle<int> renderArea = getAnalysisArea();
    int left = renderArea.getX();
    int right = renderArea.getRight();
    int top = renderArea.getY();
    int bottom = renderArea.getBottom();
    int width = renderArea.getWidth();

    Array<float> freqs{
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };

    // cache the x value position into an array
    Array<float> xs;
    for (float f : freqs) {
        float normX = mapFromLog10(f, FILTER_MIN_HZ, FILTER_MAX_HZ);
        xs.add(left + width * normX);
    }

    g.setColour(Colours::dimgrey);
    for (float x : xs) {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain{
        -24, -12, 0, 12, 24
    };

    for (float gDb : gain) {
        float y = jmap(gDb, PEAK_GAIN_MIN_DB, PEAK_GAIN_MAX_DB, float(getHeight()), 0.f);
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u): Colours::darkgrey); // If gain is 0dB draw a green line, otherwise use dark grey
        g.drawHorizontalLine(y, left, right);
    }

    // Draw labels
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    for (int i = 0; i < freqs.size(); ++i) {
        float f = freqs[i];
        float x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f) {
            addK = true;
            f /= 1000.f;
        }
        str << f;
        if (addK) {
            str << "K";
        }
        str << "Hz";
        int textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, NUMBER_OF_LINES_TEXT);
    }

    for (float gDb : gain) {
        float y = jmap(gDb, PEAK_GAIN_MIN_DB, PEAK_GAIN_MAX_DB, float(bottom), float(top));
        String str;
        if (gDb > 0) {
            str << "+";
        }
        str << gDb;

        int textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey); // If gain is 0dB draw a green line, otherwise use light grey
        g.drawFittedText(str, r, juce::Justification::centred, NUMBER_OF_LINES_TEXT);

        // draw labels on left side of response curve for the spectrum analyzer
        // the range here needs to go from 0dB to -48dB, so we can simply subtract 24dB from our existing gain array to get these numbers
        str.clear();
        str << (gDb - 24.f);
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, NUMBER_OF_LINES_TEXT);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {
    juce::Rectangle<int> bounds = getLocalBounds();

    /*bounds.reduce(JUCE_LIVE_CONSTANT(10),
        JUCE_LIVE_CONSTANT(8));*/
    //bounds.reduce(10, 8);
    bounds.removeFromTop(RESPONSE_CURVE_TOP_REMOVAL);
    bounds.removeFromBottom(RESPONSE_CURVE_BOTTOM_REMOVAL);
    bounds.removeFromLeft(RESPONSE_CURVE_SIDE_REMOVAL);
    bounds.removeFromRight(RESPONSE_CURVE_SIDE_REMOVAL);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea() {
    juce::Rectangle<int> bounds = getRenderArea();
    bounds.removeFromTop(ANALYSIS_AREA_PADDING);
    bounds.removeFromBottom(ANALYSIS_AREA_PADDING);
    return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    // Need to initialize all RotarySliderWithLabels before creating response curve and attachments
    peakFreqSlider(*audioProcessor.apvts.getParameter(PEAK_FREQ_NAME), HZ),
    peakGainSlider(*audioProcessor.apvts.getParameter(PEAK_GAIN_NAME), DB),
    peakQualitySlider(*audioProcessor.apvts.getParameter(PEAK_QUALITY_NAME), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter(LOW_CUT_FREQ_NAME), HZ),
    highCutFreqSlider(*audioProcessor.apvts.getParameter(HIGH_CUT_FREQ_NAME), HZ),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter(LOW_CUT_SLOPE_NAME), DB_PER_OCT),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter(HIGH_CUT_SLOPE_NAME), DB_PER_OCT),
    // Create attachments
    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, PEAK_FREQ_NAME, peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, PEAK_GAIN_NAME, peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, PEAK_QUALITY_NAME, peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, LOW_CUT_FREQ_NAME, lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, HIGH_CUT_FREQ_NAME, highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, LOW_CUT_SLOPE_NAME, lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, HIGH_CUT_SLOPE_NAME, highCutSlopeSlider),
    lowCutBypassButtonAttachment(audioProcessor.apvts, LOW_CUT_BYPASS_NAME, lowCutBypassButton),
    peakBypassButtonAttachment(audioProcessor.apvts, PEAK_BYPASS_NAME, peakBypassButton),
    highCutBypassButtonAttachment(audioProcessor.apvts, HIGH_CUT_BYPASS_NAME, highCutBypassButton),
    analyzerEnabledButtonAttachment(audioProcessor.apvts, ANALYZER_ENABLED_NAME, analyzerEnabledButton)
{
    // Min/max range labels for each slider
    peakFreqSlider.labels.add({ SLIDER_MIN_RANGE, MIN_FREQ_LABEL });
    peakFreqSlider.labels.add({ SLIDER_MAX_RANGE, MAX_FREQ_LABEL });
    peakGainSlider.labels.add({ SLIDER_MIN_RANGE, MIN_GAIN_LABEL });
    peakGainSlider.labels.add({ SLIDER_MAX_RANGE, MAX_GAIN_LABEL });
    peakQualitySlider.labels.add({ SLIDER_MIN_RANGE, MIN_QUALITY_LABEL });
    peakQualitySlider.labels.add({ SLIDER_MAX_RANGE, MAX_QUALITY_LABEL });
    lowCutFreqSlider.labels.add({ SLIDER_MIN_RANGE, MIN_FREQ_LABEL });
    lowCutFreqSlider.labels.add({ SLIDER_MAX_RANGE, MAX_FREQ_LABEL });
    highCutFreqSlider.labels.add({ SLIDER_MIN_RANGE, MIN_FREQ_LABEL });
    highCutFreqSlider.labels.add({ SLIDER_MAX_RANGE, MAX_FREQ_LABEL });
    lowCutSlopeSlider.labels.add({ SLIDER_MIN_RANGE, MIN_SLOPE_LABEL });
    lowCutSlopeSlider.labels.add({ SLIDER_MAX_RANGE, MAX_SLOPE_LABEL });
    highCutSlopeSlider.labels.add({ SLIDER_MIN_RANGE, MIN_SLOPE_LABEL });
    highCutSlopeSlider.labels.add({ SLIDER_MAX_RANGE, MAX_SLOPE_LABEL });

    for (juce::Component* comp : getComponents()) {
        addAndMakeVisible(comp);
    }

    lowCutBypassButton.setLookAndFeel(&lnf);
    peakBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);

    // Disable sliders when bypass buttons are disabled, and spectrum analysis when analyzer button is disabled
    // Use a SafePointer when using the onclick lambdas to make sure this class still exists when these functions are called
    juce::Component::SafePointer<SimpleEQAudioProcessorEditor> safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
    lowCutBypassButton.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) {
            bool bypassed = comp->lowCutBypassButton.getToggleState();
            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };
    peakBypassButton.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) {
            bool bypassed = comp->peakBypassButton.getToggleState();
            comp->peakFreqSlider.setEnabled(!bypassed);
            comp->peakGainSlider.setEnabled(!bypassed);
            comp->peakQualitySlider.setEnabled(!bypassed);
        }
    };
    highCutBypassButton.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) {
            bool bypassed = comp->highCutBypassButton.getToggleState();
            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };
    analyzerEnabledButton.onClick = [safePtr]() {
        if (auto* comp = safePtr.getComponent()) {
            bool enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };

    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    lowCutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // doing this so we don't have to write `juce::` everywhere in this function
    using namespace juce;
    g.fillAll (Colours::black);
}

void SimpleEQAudioProcessorEditor::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds();

    juce::Rectangle<int> analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(ANALYZER_ENABLED_BUTTON_WIDTH);
    analyzerEnabledArea.setX(ANALYZER_ENABLED_BUTTON_X);
    analyzerEnabledArea.removeFromTop(ANALYZER_ENABLED_BUTTON_TOP_REMOVAL);

    analyzerEnabledButton.setBounds(analyzerEnabledArea);
    bounds.removeFromTop(DEFAULT_PADDING);

    float heightRatio = 25.0f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f; //(use this to dial in the size while running plugin)
    juce::Rectangle<int> responseArea = bounds.removeFromTop(bounds.getHeight() * heightRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(DEFAULT_PADDING); // Create a little space between the response curve and dials
    juce::Rectangle<int> lowCutArea = bounds.removeFromLeft(bounds.getWidth() * UI_BOUNDS_THIRD);
    juce::Rectangle<int> highCutArea = bounds.removeFromRight(bounds.getWidth() * UI_BOUNDS_HALF);

    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(BYPASS_BUTTON_HEIGHT));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * UI_BOUNDS_HALF));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutBypassButton.setBounds(highCutArea.removeFromTop(BYPASS_BUTTON_HEIGHT));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * UI_BOUNDS_HALF));
    highCutSlopeSlider.setBounds(highCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(BYPASS_BUTTON_HEIGHT));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * UI_BOUNDS_THIRD));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * UI_BOUNDS_HALF));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComponents() {
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        &lowCutBypassButton,
        &peakBypassButton,
        &highCutBypassButton,
        &analyzerEnabledButton
    };
}

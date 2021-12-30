/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;

    // background of the Slider
    auto bounds = Rectangle<float>(x, y, width, height);
    g.setColour(Colour(97u, 18u, 167u)); // Purple background of ellipse
    g.fillEllipse(bounds);
    g.setColour(Colour(255u, 154u, 1u)); // Orange border of ellipse
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*> (&slider)) {
        jassert(rotaryStartAngle < rotaryEndAngle); // Check to make sure the angles are set right

        // Draw the rectangle that forms the pointer of the rotary dial // TODO it is not drawing properly
        auto center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX() - 2); // Two pixels left of center
        r.setRight(center.getX() + 2); // Two pixels to right of center
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        p.addRoundedRectangle(r, 2.f);

        auto sliderAngleRadians = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle); // Map the slider angle to be between the bounding angles
        p.applyTransform(AffineTransform().rotated(sliderAngleRadians, center.getX(), center.getY()));
        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        // Draw bounding box for text
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);
        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g) {
    using namespace juce;
    // 7 o'clock is where slider draws value of zero, 5 o'clock is where slider draws value of one
    auto startAngle = degreesToRadians(180.f + 45.f); // 7 o'clock basically
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi; // 5 o'clock basically - needs a full rotation to put it at the right angle though, otherwise slider will go the wrong way

    auto range = getRange();
    auto sliderBounds = getSliderBounds();
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
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),  // normalize values in the range
                                      startAngle, 
                                      endAngle, 
                                      *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i) {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        // This logic determines where to place the labels for the bounding points for each slider (e.g. 20 Hz to 20KHz)
        auto angle = jmap(pos, 0.f, 1.f, startAngle, endAngle);
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, angle);
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()); // Make our sliders square by getting the minimum size from the width and height
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
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false; // For Kilohertz

    // Only Float parameters are supported here
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
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
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p) {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->addListener(this);
    }
    // update the monochain
    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        DBG("params changed");
        // update the monochain
        updateChain();
        // signal a repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain() {
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
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

    auto responseArea = getAnalysisArea();

    auto width = responseArea.getWidth();
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();
    std::vector<double> magnitudes;
    magnitudes.resize(width);

    for (int i = 0; i < width; ++i) {
        double magnitude = 1.f;
        auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>()) {
            magnitude *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        // check each filter in the CutChains
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
        20, 30, 40, 50, 100,
        200, 300, 400, 500, 1000,
        2000, 3000, 4000, 5000, 10000,
        20000
    };

    // cache the x value position into an array
    Array<float> xs;
    for (float f : freqs) {
        float normX = mapFromLog10(f, 20.f, 20000.f);
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
        float y = jmap(gDb, -24.f, 24.f, float(getHeight()), 0.f);
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u): Colours::darkgrey); // If gain is 0dB draw a green line, otherwise use dark grey
        g.drawHorizontalLine(y, left, right);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {
    auto bounds = getLocalBounds();

    /*bounds.reduce(JUCE_LIVE_CONSTANT(10),
        JUCE_LIVE_CONSTANT(8));*/
    //bounds.reduce(10, 8);
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea() {
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    // Need to initialize all RotarySliderWithLabels before creating response curve and attachments
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    // Create attachments
    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Min/max range labels for each slider
    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20KHz" });
    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "24dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20KHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20KHz" });
    lowCutSlopeSlider.labels.add({ 0.f, "12dB/Oct" });
    lowCutSlopeSlider.labels.add({ 1.f, "48dB/Oct" });
    highCutSlopeSlider.labels.add({ 0.f, "12dB/Oct" });
    highCutSlopeSlider.labels.add({ 1.f, "48dB/Oct" });

    for (auto* comp : getComponents()) {
        addAndMakeVisible(comp);
    }

    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
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
    auto bounds = getLocalBounds();
    float heightRatio = 25.0f / 100.f; // JUCE_LIVE_CONSTANT(33) / 100.f; //(use this to dial in the size while running plugin)
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * heightRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5); // Create a little space between the response curve and dials
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
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
        &responseCurveComponent
    };
}

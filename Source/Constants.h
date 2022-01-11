/*
  ==============================================================================

    Constants.h
    Created: 10 Jan 2022 6:20:19pm
    Author:  Nate

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// APVTS Names
// LowCut Filter
const juce::String LOW_CUT_FREQ_NAME = "LowCut Freq";
const juce::String LOW_CUT_SLOPE_NAME = "LowCut Slope";
const juce::String LOW_CUT_BYPASS_NAME = "LowCut Bypassed";

// HighCut Filter
const juce::String HIGH_CUT_FREQ_NAME = "HighCut Freq";
const juce::String HIGH_CUT_SLOPE_NAME = "HighCut Slope";
const juce::String HIGH_CUT_BYPASS_NAME = "HighCut Bypassed";

// Peak Filter
const juce::String PEAK_FREQ_NAME = "Peak Freq";
const juce::String PEAK_GAIN_NAME = "Peak Gain";
const juce::String PEAK_QUALITY_NAME = "Peak Quality";
const juce::String PEAK_BYPASS_NAME = "Peak Bypassed";

// Analyzer
const juce::String ANALYZER_ENABLED_NAME = "Analyzer Enabled";


//==============================================================================
// Ranges
const float SLIDER_MIN_RANGE = 0.f;
const double SLIDER_MIN_RANGE_DOUBLE = 0.0;
const float SLIDER_MAX_RANGE = 1.f;
const double SLIDER_MAX_RANGE_DOUBLE = 1.0;
const float FILTER_MIN_HZ = 20.f;
const juce::String MIN_FREQ_LABEL = "20Hz";
const float FILTER_MAX_HZ = 20000.f;
const juce::String MAX_FREQ_LABEL = "20KHz";
const float LOW_CUT_FILTER_DEFAULT = 20.f;
const float PEAK_FILTER_DEFAULT = 750.f;
const float HIGH_CUT_FILTER_DEFAULT = 20000.f;
const float PEAK_GAIN_DEFAULT = 0.0f;
const float PEAK_QUALITY_DEFAULT = 1.f;
const float PEAK_GAIN_MIN_DB = -24.f;
const juce::String MIN_GAIN_LABEL = "-24dB";
const float PEAK_GAIN_MAX_DB = 24.f;
const juce::String MAX_GAIN_LABEL = "24db";
const float PEAK_QUALITY_MIN = 0.1f;
const juce::String MIN_QUALITY_LABEL = "0.1";
const float PEAK_QUALITY_MAX = 10.f;
const juce::String MAX_QUALITY_LABEL = "10.0";
const juce::String MIN_SLOPE_LABEL = "12dB/Oct";
const juce::String MAX_SLOPE_LABEL = "48dB/Oct";

const float FILTER_FREQUENCY_INTERVAL = 1.f;
const float PEAK_GAIN_INTERVAL = 0.5f;
const float PEAK_QUALITY_INTERVAL = 0.05f;

const float FILTER_FREQUENCY_SKEW_FACTOR = 0.25f;
const float DEFAULT_SKEW_FACTOR = 1.f;

const int SLOPE_DEFAULT_POS = 0;
const bool BYPASS_DEFAULT = false;
const bool ENABLED_DEFAULT = true;

const float ABSOLUTE_MINIMUM_GAIN = -48.f; // Scale only goes to -48dB


//==============================================================================
// Units
const juce::String HZ = "Hz";
const juce::String KILO_HZ = "KHz";
const juce::String DB = "dB";
const juce::String DB_PER_OCT = "db/Oct";


//==============================================================================
// UI Values
const float UI_BOUNDS_HALF = 0.5f;
const float UI_BOUNDS_THIRD = 0.33f;
const int BYPASS_BUTTON_HEIGHT = 25;

const int RESPONSE_CURVE_TOP_REMOVAL = 12;
const int RESPONSE_CURVE_BOTTOM_REMOVAL = 2;
const int RESPONSE_CURVE_SIDE_REMOVAL = 20;

const int ANALYZER_ENABLED_BUTTON_WIDTH = 100;
const int ANALYZER_ENABLED_BUTTON_X = 5;
const int ANALYZER_ENABLED_BUTTON_TOP_REMOVAL = 2;

const int DEFAULT_PADDING = 5;
const int ANALYSIS_AREA_PADDING = 4;
const int SLIDER_X_PADDING = 2;

const int NUMBER_OF_LINES_TEXT = 1;

const float ELLIPSE_DEFAULT_THICKNESS = 1.f;
const float ELLIPSE_THICKER_OUTLINE = 2.f;
const float ROUNDED_RECTANGLE_THICKNESS = 2.f;
const float PATH_STROKE_THICKNESS = 1.f;

const int TEXT_BOUNDING_BOX_ADD_WIDTH = 4;
const int TEXT_BOUNDING_BOX_ADD_HEIGHT = 2;
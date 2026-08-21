//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_classifier.h
// description: Classifies value/duration pairs into dots and dashes
// License: MIT
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

const int BIN_WIDTH = 10;
const int BIN_MAX = 500;
// e.g. 4 means 3 parts historic to 1 part recent data
// higher values adapt more slowly to change but are more tollerant of outliers.
// smaller values adapt more quickly to change but are less tollerant to outliers.
const int HISTORIC_WEIGHTING = 10;

class c_morse_timing_classifier
{
public:
  c_morse_timing_classifier(int channel_number);

  // Element ON durations (dots/dashes)
  void update_on_model(const float* durations, size_t count);
  void update_off_model(const float* durations, size_t count);

  void classify_on(float d, float& logp_dot, float& logp_dash, float& logp_dotdot,
                   float& logp_dotdash, float& logp_dashdash);
  void classify_off(float duration_ms, float* logProbs);

  float get_dot_length() const { return dot_mu; }
  float get_WPM() const { return 1200.0f / get_dot_length(); }
  bool good_estimates;

  void prescale_histograms();
  void postscale_histograms();
  void reset();

private:
  int m_channel_number;
  int on_histogram[BIN_MAX / BIN_WIDTH];
  int off_histogram[BIN_MAX / BIN_WIDTH];
  float dot_mu, dot_sigma;
  float dash_mu, dash_sigma;
  float gap1_mu, gap1_sigma;
  float gap3_mu, gap3_sigma;
  float gap7_mu, gap7_sigma;
};

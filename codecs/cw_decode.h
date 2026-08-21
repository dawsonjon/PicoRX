#ifndef __CW_DECODE_H__
#define __CW_DECODE_H__

#include "cw_classifier.h"
#include <string>

struct s_observation
{
  bool mark;
  float duration;
};

struct s_candidate
{
  std::string text;
  std::string word;
  std::string pattern;
  float logp;
};

static const int BEAM_WIDTH = 3;
class c_cw_decoder
{

private:
  c_morse_timing_classifier classifier;
  s_candidate beam[BEAM_WIDTH];
  int items_in_beam;
  int m_channel_number;

public:
  void decode(s_observation signal[], int num_observations);
  std::string get_text();
  std::string get_text_partial();
  void reset() { classifier.reset(); }
  float get_WPM() { return classifier.get_WPM(); }

  c_cw_decoder(int channel_number) : classifier(channel_number), m_channel_number(channel_number)
  {
    beam[0] = {"", "", "", 0.0f};
    items_in_beam = 1;
  }
};

#endif

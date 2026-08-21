//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_decode.cpp
// description: CW Decoder using beam decoder with autocorrect
// License: MIT
//

#include "cw_decode.h"
#include "cw_data.h"
#include "cw_dsp.h"
#include "dictionary.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

// #define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
#include <Arduino.h>
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#include <cstdio>
#define DEBUG_PRINTF(...) std::printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(...)
#endif

// return the log likelihood x in a normal distribution mu/sigma
float log_gaussian(float x, float mu, float sigma)
{
  return -0.5f * pow((x - mu) / sigma, 2.0f);
}

// Is this pattern part of a morse symbol?
bool is_start_of_code(std::string& pattern)
{
  int span = 128;
  int morse_index = 0;

  for (int i = 0; i < 7; i++) {
    span >>= 1;
    assert(i < strlen(pattern.c_str()) + 1);
    if (pattern.c_str()[i] == 0) {
      return MORSE[morse_index] != '~';
    } else if (pattern.c_str()[i] == '.') {
      morse_index++;
    } else if (pattern.c_str()[i] == '-') {
      morse_index += span;
    }
  }
  return false;
}

// Is this pattern an exact match to a morse symbol?
bool is_code(std::string& pattern)
{
  int span = 128;
  int morse_index = 0;

  for (int i = 0; i < 7; ++i) {
    span >>= 1;
    assert(i < strlen(pattern.c_str()) + 1);
    if (pattern.c_str()[i] == 0) {
      return MORSE[morse_index] != '#' && MORSE[morse_index] != '~';
    } else if (pattern.c_str()[i] == '.') {
      morse_index++;
    } else if (pattern.c_str()[i] == '-') {
      morse_index += span;
    }
  }
  return false;
}

// What letter does this code correspond to?
char get_letter_from_code(std::string& pattern)
{
  int span = 128;
  int morse_index = 0;

  for (int i = 0; i < 7; i++) {
    span >>= 1;
    assert(i < strlen(pattern.c_str()) + 1);
    if (pattern.c_str()[i] == 0) {
      return MORSE[morse_index];
    } else if (pattern.c_str()[i] == '.') {
      morse_index++;
    } else if (pattern.c_str()[i] == '-') {
      morse_index += span;
    }
  }
  return '#';
}

// What is the log probability that this is the start of a word?
float word_prefix_log_prob(std::string& word)
{
  if (word.size() < 2)
    return 0;
  if (binary_search_prefix(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, word))
    return 1.0f;
  return 0;
}

bool is_valid_callsign(const std::string& s)
{
  size_t i = 0;
  size_t n = s.size();

  // 1–2 starting letters
  int letters1 = 0;
  while (i < n && std::isalpha(s[i]) && letters1 < 2) {
    ++i;
    ++letters1;
  }
  if (letters1 == 0 || i >= n)
    return false;

  // one digit
  if (!std::isdigit(s[i++]))
    return false;
  if (i >= n)
    return false;

  // 1–3 trailing letters
  int letters2 = 0;
  while (i < n && std::isalpha(s[i]) && letters2 < 3) {
    ++i;
    ++letters2;
  }
  if (letters2 == 0)
    return false;

  // optional suffix like /P, /M, /MM, etc.
  if (i < n) {
    if (s[i++] != '/' || i >= n)
      return false;
    int suf = 0;
    while (i < n && std::isalpha(s[i]) && suf < 3) {
      ++i;
      ++suf;
    }
    if (suf == 0)
      return false;
  }

  // must end exactly here
  return i == n;
}

// What is the log probability that this is a word?
float language_log_prob(std::string& word)
{
  if (binary_search_word(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, word))
    return 4.0f;
  if (is_valid_callsign(word))
    return 2.0f;
  return 0;
}

void replace_all(std::string& str, const std::string& from, const std::string& to)
{
  if (from.empty())
    return; // avoid infinite loop
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length();
  }
}

void replace_prosigns(std::string& str)
{
  for (int i = 0; i < NUM_PROSIGNS; ++i) {
    char char_to_replace[] = {static_cast<char>(0x80 + i), 0};
    std::string from = std::string(char_to_replace);
    std::string to = std::string(PROSIGNS[i]);
    replace_all(str, from, to);
  }
}

bool looks_like_gibberish(const std::string& text)
{
  int letters = 0;
  int e_or_i = 0;

  for (char c : text) {
    if (!isalpha(c))
      continue;
    c = std::tolower(c);
    letters++;
    if (c == 'e' || c == 'i')
      e_or_i++;
  }

  if ((float)e_or_i / letters > 0.5f)
    return true; // mostly E/I (typical Morse junk)

  return false; // looks plausible enough
}

void autocorrect_text(std::string& text)
{
  std::string word = "";
  std::string new_text = "";

  for (unsigned int i = 0; i < text.size(); ++i) {
    if (isalpha(text[i])) {
      word += text[i];
    } else {
      if (word.size() > 2)
        autocorrect(word);
      // if(looks_like_gibberish(word)) word = "";
      new_text += word + text[i];
      word = "";
    }
  }
  if (word.size() > 2)
    autocorrect(word);
  new_text += word;

  text = new_text;
}

// get the text we are currently decoding
std::string c_cw_decoder ::get_text_partial()
{
  std::string letter = std::string("") + get_letter_from_code(beam[0].pattern);
  std::string text = beam[0].word + letter;
  replace_prosigns(text);
  return text;
}

// get the text we are committed to.
std::string c_cw_decoder ::get_text()
{
  std::string text = beam[0].text;

  // prune candidates that don't share the same prefix
  int filtered_candidate_index = 1;
  for (int candidate_index = 1; candidate_index < items_in_beam; ++candidate_index) {
    if (beam[candidate_index].text == beam[0].text) {
      beam[filtered_candidate_index] = beam[candidate_index];
      beam[filtered_candidate_index].text = "";
      filtered_candidate_index++;
    }
  }
  beam[0].text = "";
  items_in_beam = filtered_candidate_index;
  autocorrect_text(text);
  replace_prosigns(text);
  return text;
}

// filter unreasonable dot and dash lengths
static void pre_filter_observations(s_observation signal[], int& num_observations)
{

  // set some hard limits on minimum and maximum sizes
  float dot_min_ms = 30.0f;
  float dash_max_ms = 720.0f;
  float min_hard = std::max(dot_min_ms * 0.5f, 8.0f);
  float max_hard = dash_max_ms * 2.0f;

  // remove low signals that are too small
  int filtered_index = 0;
  for (int index = 0; index < num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while ((index + 2 < num_observations) && (signal[index + 1].mark == false) &&
           (signal[index + 1].duration < min_hard)) {
      signal[filtered_index].duration += signal[index + 1].duration + signal[index + 2].duration;
      index += 2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  // remove high signals that are too small
  filtered_index = 0;
  for (int index = 0; index < num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while ((index + 2 < num_observations) && (signal[index + 1].mark == true) &&
           (signal[index + 1].duration < min_hard)) {
      signal[filtered_index].duration += signal[index + 1].duration + signal[index + 2].duration;
      index += 2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  // remove high signals that are too big
  filtered_index = 0;
  for (int index = 0; index < num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while ((index + 2 < num_observations) && (signal[index + 1].mark == true) &&
           (signal[index + 1].duration > max_hard)) {
      signal[filtered_index].duration += signal[index + 1].duration + signal[index + 2].duration;
      index += 2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;

  // remove high signals are in the middle of two long spaces
  filtered_index = 0;
  for (int index = 0; index < num_observations; ++index) {
    signal[filtered_index] = signal[index];
    while ((index + 2 < num_observations) && (signal[index + 1].mark == true) &&
           (signal[index].duration > max_hard) && (signal[index + 2].duration > max_hard)) {
      signal[filtered_index].duration += signal[index + 1].duration + signal[index + 2].duration;
      index += 2;
    }
    filtered_index++;
  }
  num_observations = filtered_index;
}

void print_durations(s_observation signal[], int num_observations)
{
  (void) signal;
  (void) num_observations;
  DEBUG_PRINTF("Duration: \n");
  for (int idx = 0; idx < num_observations; idx++) {
    DEBUG_PRINTF("%u %u, %f\n", idx, signal[idx].mark, signal[idx].duration);
  }
  DEBUG_PRINTF("\n");
}

// find the n most likely decodes
void c_cw_decoder ::decode(s_observation signal[], int num_observations)
{
  DEBUG_PRINTF("decoding channel %u\n", m_channel_number);

  // prefilter signal to remove obvious errors
  print_durations(signal, num_observations);
  pre_filter_observations(signal, num_observations);
  print_durations(signal, num_observations);

  // update adaptive clissifier with latest on and off durations
  float on_durations[OBSERVATION_BUFFER_SIZE] = {0};
  float off_durations[OBSERVATION_BUFFER_SIZE] = {0};
  int on_count = 0;
  int off_count = 0;
  DEBUG_PRINTF("num observations %u\n", num_observations);
  for (int idx = 0; idx < num_observations; ++idx) {
    DEBUG_PRINTF("signal %u %u\n", idx, signal[idx].mark);
    if (signal[idx].mark)
      on_durations[on_count++] = signal[idx].duration;
    if (!signal[idx].mark)
      off_durations[off_count++] = signal[idx].duration;
  }

  DEBUG_PRINTF("on count: %u\n", on_count);
  DEBUG_PRINTF("off count: %u\n", off_count);

  // ON (key down) times:
  classifier.update_on_model(on_durations, on_count);
  if (!classifier.good_estimates)
    return;

  // OFF (silence) times:
  classifier.update_off_model(off_durations, off_count);

  DEBUG_PRINTF("classify_observations %u\n", num_observations);
  for (int i = 0; i < num_observations; ++i) {

    float duration = signal[i].duration;
    float logp_dot, logp_dash, logp_dotdot, logp_dotdash, logp_dashdash;
    classifier.prescale_histograms();
    classifier.classify_on(duration, logp_dot, logp_dash, logp_dotdot, logp_dotdash, logp_dashdash);
    if (signal[i].mark){
      DEBUG_PRINTF("duration: %f dot: %f dash: %f\n", duration, logp_dot, logp_dash);
    }

    float logp[3];
    classifier.classify_off(duration, logp);
    classifier.postscale_histograms();
    float logp_gap1 = logp[0];
    float logp_gap3 = logp[1];
    float logp_gap7 = logp[2];
    if (!signal[i].mark) {
      DEBUG_PRINTF("duration: %f symbol: %f letter: %f space: %f\n", duration, logp_gap1, logp_gap3,
                   logp_gap7);
    }

    static s_candidate candidates[BEAM_WIDTH * 6]; // max 6 new predictions for each item in beam
    int num_candidates = 0;

    for (int j = 0; j < items_in_beam; j++) {
      std::string& text = beam[j].text;
      std::string& word = beam[j].word;
      std::string& pattern = beam[j].pattern;
      float& beam_logp = beam[j].logp;

      if (signal[i].mark) {

        char letter = get_letter_from_code(pattern);
        bool pattern_is_code = letter != '#' && letter != '~';

        // Case 1: dot
        std::string dot_pattern = pattern + '.';
        if (is_start_of_code(dot_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + '.';
          candidates[num_candidates].logp = beam_logp + logp_dot;
          DEBUG_PRINTF("%u dot \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = ".";
          candidates[num_candidates].logp = beam_logp + logp_dot;
          DEBUG_PRINTF("%u dot2 \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

        // Case 2: dash
        std::string dash_pattern = pattern + '-';
        if (is_start_of_code(dash_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + '-';
          candidates[num_candidates].logp = beam_logp + logp_dash;
          DEBUG_PRINTF("%u dash \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = "-";
          candidates[num_candidates].logp = beam_logp + logp_dash;
          DEBUG_PRINTF("%u dash2 \"%s\" \"%s\" %s %f\n", i, candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

        // Case 3: dotdash
        std::string dotdash_pattern = pattern + ".-";
        if (is_start_of_code(dotdash_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + ".-";
          candidates[num_candidates].logp = beam_logp + logp_dotdash;
          DEBUG_PRINTF("%u dotdash \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = ".-";
          candidates[num_candidates].logp = beam_logp + logp_dotdash;
          DEBUG_PRINTF("%u dotdash \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

        // Case 4: dashdot
        std::string dashdot_pattern = pattern + "-.";
        if (is_start_of_code(dashdot_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + "-.";
          candidates[num_candidates].logp = beam_logp + logp_dotdash;
          DEBUG_PRINTF("%u dashdot \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = "-.";
          candidates[num_candidates].logp = beam_logp + logp_dotdash;
          DEBUG_PRINTF("%u dashdot \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

        // Case 5: dashdash
        std::string dashdash_pattern = pattern + "--";
        if (is_start_of_code(dashdash_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + "--";
          candidates[num_candidates].logp = beam_logp + logp_dashdash;
          DEBUG_PRINTF("%u dashdash \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = "--";
          candidates[num_candidates].logp = beam_logp + logp_dashdash;
          DEBUG_PRINTF("%u dashdash \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

        // Case 6: dotdot
        std::string dotdot_pattern = pattern + "..";
        if (is_start_of_code(dotdot_pattern)) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word;
          candidates[num_candidates].pattern = pattern + "..";
          candidates[num_candidates].logp = beam_logp + logp_dotdot - 2;
          DEBUG_PRINTF("%u dotdot \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        } else if (pattern_is_code) {
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].text = text;
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].pattern = "..";
          candidates[num_candidates].logp = beam_logp + logp_dotdot - 2;
          DEBUG_PRINTF("%u dotdot \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }

      } else {
        // Case 3: symbol gap
        assert(num_candidates < BEAM_WIDTH * 6);
        candidates[num_candidates].text = text;
        candidates[num_candidates].word = word;
        candidates[num_candidates].pattern = pattern;
        candidates[num_candidates].logp = beam_logp + logp_gap1;
        DEBUG_PRINTF("%u symbol_gap \"%s\" \"%s\" %s %f\n", i,
                     candidates[num_candidates].text.c_str(),
                     candidates[num_candidates].word.c_str(),
                     candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
        num_candidates++;

        char letter = get_letter_from_code(pattern);
        bool pattern_is_code = letter != '#' && letter != '~';
        if (pattern_is_code) {
          std::string last_word = word + letter;
          float language_bonus =
              language_log_prob(last_word); // bonus for being a whole word, favours a word gap.
          float prefix_bonus = word_prefix_log_prob(
              last_word); // prefix of a word, favours a letter gap over a symbol gap

          // Case 4: letter gap
          assert(num_candidates < BEAM_WIDTH * 6);
          candidates[num_candidates].word = word + letter;
          candidates[num_candidates].text = text;
          candidates[num_candidates].pattern = "";
          candidates[num_candidates].logp = beam_logp + logp_gap3 + prefix_bonus;
          DEBUG_PRINTF("%u letter_gap \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;

          // Case 5: word gap
          assert(num_candidates < BEAM_WIDTH * 6);
          std::string new_word = word + letter;
          candidates[num_candidates].text = text + new_word + ' ';
          candidates[num_candidates].word = "";
          candidates[num_candidates].pattern = "";
          candidates[num_candidates].logp = beam_logp + logp_gap7 + language_bonus;
          DEBUG_PRINTF("language bonus%f\n", language_bonus);
          DEBUG_PRINTF("%u word_gap \"%s\" \"%s\" %s %f\n", i,
                       candidates[num_candidates].text.c_str(),
                       candidates[num_candidates].word.c_str(),
                       candidates[num_candidates].pattern.c_str(), candidates[num_candidates].logp);
          num_candidates++;
        }
      }
    }

    // find best candidates to propagate forward
    items_in_beam = std::min(BEAM_WIDTH, num_candidates);
    std::vector<int> best_indices(num_candidates);
    std::iota(best_indices.begin(), best_indices.end(), 0);
    std::partial_sort(
        best_indices.begin(), best_indices.begin() + items_in_beam, best_indices.end(),
        [&](const int a, const int b) { return candidates[a].logp > candidates[b].logp; });

    // copy best candidates into beam
    for (int idx = 0; idx < items_in_beam; ++idx) {
      assert(idx < BEAM_WIDTH);
      assert(idx < num_candidates);
      beam[idx] = candidates[best_indices[idx]];
    }

    assert(items_in_beam);
  }
}

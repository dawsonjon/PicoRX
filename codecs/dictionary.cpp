//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: dictionary.cpp
// description: Dictionary lookup functionality including autocorrect and autocomplete
// License: MIT
//

#include "cw_data.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <string>

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

bool binary_search_word(const char* words[], int num_words, const std::string& target)
{
  int left = 0;
  int right = num_words - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    assert(mid < num_words);
    int cmp = std::strcmp(words[mid], target.c_str());

    if (cmp == 0) {
      return true; // found
    } else if (cmp < 0) {
      left = mid + 1; // search right half
    } else {
      right = mid - 1; // search left half
    }
  }

  return false;
}

bool binary_search_prefix(const char* words[], int num_words, const std::string& target)
{
  int left = 0;
  int right = num_words - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    assert(mid < num_words);

    const char* word = words[mid];
    int cmp = std::strncmp(word, target.c_str(), target.size());

    if (cmp == 0) {
      // target is a prefix of words[mid]
      return true;
    } else if (std::strcmp(word, target.c_str()) < 0) {
      left = mid + 1; // search right half
    } else {
      right = mid - 1; // search left half
    }
  }

  return false;
}

int binary_search_prefix_index(const char* words[], int num_words, const std::string& target)
{
  int left = 0;
  int right = num_words - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    assert(mid < num_words);

    const char* word = words[mid];
    int cmp = std::strncmp(word, target.c_str(), target.size());

    if (cmp == 0) {
      // target is a prefix of words[mid]
      return mid;
    } else if (std::strcmp(word, target.c_str()) < 0) {
      left = mid + 1; // search right half
    } else {
      right = mid - 1; // search left half
    }
  }

  return false;
}

int binary_search_insertion_point(const char* words[], int num_words, const std::string& key)
{
  int left = 0, right = num_words;
  while (left < right) {
    int mid = left + (right - left) / 2;
    if (std::string(words[mid]) < key)
      left = mid + 1;
    else
      right = mid;
  }
  return left; // insertion index
}

int binary_search_ranking(const char* words[], int num_words, const std::string& target)
{
  int left = 0;
  int right = num_words - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    assert(mid < num_words);
    int cmp = std::strcmp(words[mid], target.c_str());

    if (cmp == 0) {
      return RANKINGS[mid]; // found
    } else if (cmp < 0) {
      left = mid + 1; // search right half
    } else {
      right = mid - 1; // search left half
    }
  }

  return -1;
}

int levenshtein_distance_1(const char* a, const char* b)
{
  int len_a = strlen(a);
  int len_b = strlen(b);
  int diff = abs(len_a - len_b);
  if (diff > 1)
    return 2; // can't be distance 1

  int i = 0, j = 0;
  bool found_diff = false;

  while (i < len_a && j < len_b) {
    if (a[i] == b[j]) {
      i++;
      j++;
      continue;
    }

    if (found_diff)
      return 2; // already had one mismatch
    found_diff = true;

    if (len_a > len_b)
      i++; // deletion in b
    else if (len_a < len_b)
      j++; // insertion in b
    else {
      i++;
      j++;
    } // substitution
  }

  // Account for trailing extra char
  if ((i < len_a) || (j < len_b)) {
    if (found_diff)
      return 2;
    found_diff = true;
  }

  return found_diff ? 1 : 0;
}

void autocorrect(std::string& word)
{

  // If the word already exists exactly, nothing to do
  if (binary_search_word(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, word))
    return;

  std::string best_word = word;
  int best_distance = INT_MAX;
  int best_ranking = INT_MAX;

  // 1️ Find the insertion point via binary search
  int idx = binary_search_insertion_point(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, word);

  // 2️ Define a small window around the index
  const int WINDOW = 50; // good balance for 10k words
  int start = std::max(0, idx - WINDOW);
  int end = std::min((int)NUM_AUTOCORRECT_WORDS, idx + WINDOW);

  // 3️ Only check candidates within the window
  for (int i = start; i < end; ++i) {
    const std::string& candidate = AUTOCORRECT_WORDS[i];
    int d = levenshtein_distance_1(word.c_str(), candidate.c_str());

    if (d <= 1 && ((d < best_distance) || (d == best_distance && RANKINGS[i] < best_ranking))) {
      best_distance = d;
      best_word = candidate;
      best_ranking = RANKINGS[i];

      if (best_distance == 0)
        break; // exact match within window
    }
  }

  // 4️ First letter substitutions won't be in the window so deal with those seperately
  static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (best_distance > 1) {
    std::string candidate;
    int ranking;
    for (int i = 0; i < 26; i++) {
      // first letter substitutions
      candidate = word;
      candidate[0] = letters[i];
      ranking = binary_search_ranking(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, candidate);
      if (ranking > 0 && ranking < best_ranking) {
        best_word = candidate;
        best_distance = 1;
        best_ranking = ranking;
      }
    }
  }

  if (best_distance == 1) {
    word = best_word;
  }
}

std::string suggestion(std::string& word, std::string& second, std::string& third)
{

  std::string best_word = "";
  int best_ranking = INT_MAX;
  std::string second_best_word = "";
  int second_best_ranking = INT_MAX;
  std::string third_best_word = "";
  int third_best_ranking = INT_MAX;
  second = third = "";
  if (word.size() < 1)
    return "";

  // 1️ Find the insertion point via binary search
  int idx = binary_search_prefix_index(AUTOCORRECT_WORDS, NUM_AUTOCORRECT_WORDS, word);

  // 2️ Define a small window around the index
  const int WINDOW = 50;
  int start = idx;
  int end = std::min((int)NUM_AUTOCORRECT_WORDS, idx + WINDOW);

  // 3️ Only check candidates within the window
  for (int i = start; i < end; ++i) {
    const std::string& candidate = AUTOCORRECT_WORDS[i];
    int cmp = std::strncmp(word.c_str(), candidate.c_str(), word.size());
    bool is_prefix = cmp == 0;

    if (is_prefix) {

      if (RANKINGS[i] < best_ranking) {
        third_best_word = second_best_word;
        third_best_ranking = second_best_ranking;
        second_best_word = best_word;
        second_best_ranking = best_ranking;
        best_word = candidate;
        best_ranking = RANKINGS[i];
      }

      else if (RANKINGS[i] < second_best_ranking) {
        third_best_word = second_best_word;
        third_best_ranking = second_best_ranking;
        second_best_word = candidate;
        second_best_ranking = RANKINGS[i];
      }

      else if (RANKINGS[i] < third_best_ranking) {
        third_best_word = candidate;
        third_best_ranking = RANKINGS[i];
      }
    }
  }

  second = second_best_word;
  third = third_best_word;
  return best_word;
}

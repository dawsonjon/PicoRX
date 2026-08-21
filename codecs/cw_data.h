//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_data.h
// description: cw related data including ~10000 word autocorrect dictionary
// License: MIT
//

#ifndef __CW_DATA_H__
#define __CW_DATA_H__
#include <cstdint>

struct s_morse
{
  char letter;
  const char* code;
};

extern const int NUM_MORSE_LETTERS;
extern const char MORSE[];

extern const int NUM_AUTOCORRECT_WORDS;
extern const char* AUTOCORRECT_WORDS[];
extern const uint16_t RANKINGS[];

extern const int NUM_PROSIGNS;
extern const char* PROSIGNS[];
#endif

#ifndef __SPEECH_PROCESSOR_H__
#define __SPEECH_PROCESSOR_H__
#include <cstdint>
int16_t treble_boost(int16_t x);
int16_t high_pass(int16_t x);
int16_t process_speech(int16_t x);
void compress(int16_t &x, float &envelope, uint16_t limit);
#endif

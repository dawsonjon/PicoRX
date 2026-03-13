#ifndef __SPEECH_PROCESSOR_H__
#define __SPEECH_PROCESSOR_H__
#include <cstdint>
int16_t treble(int16_t x, int8_t level);
int16_t bass(int16_t x, int8_t level);
int16_t process_speech(int16_t x, bool noise_gate, int8_t treble, int8_t bass);
int16_t noise_gate(int16_t x);
void compress(int16_t &x, float &envelope, uint16_t limit);
#endif

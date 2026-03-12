#include "../modulator.h"
#include "../cordic.h"
#include "../speech_processor.h"
#include "../../src/utils.h"
#include <cstdio>
#include <cmath>


int main()
{
  initialise_luts();
  modulator the_modulator;
  FILE *fp = fopen("test.pcm", "rb");
  //FILE *fp = fopen("tone.pcm", "rb");
  FILE *data_file = fopen("data_file.txt", "w");

  float env;
  float test_phase = 0.0f;
  s_debug debug;

  //tone burst
  for(int idx=0; idx<1024; idx++) {
    int16_t audio_processed;
    int16_t i, q, phase;
    uint16_t magnitude;

    int32_t audio_clipped = 4095.0f*sin(2.0f*M_PI*test_phase*0.2);// audio[idx];
    test_phase += 1.0f;
    audio_processed = process_speech(audio_clipped);
    int16_t audio_uncompressed = audio_processed;
    compress(audio_processed, env, 2048);
    the_modulator.process_sample(USB, audio_processed, i, q, magnitude, phase, 0, debug);
    fprintf(data_file, "%i %i %f %i %i %i %i %i %u %i %i %u %i\n", audio_uncompressed, audio_processed, env, debug.filtered_audio, debug.raw_i, debug.raw_q, debug.clipped_i, debug.clipped_q, debug.env, i, q, magnitude, phase);
  }

  while(1) {
    int16_t audio[1024];
    int samples = fread(audio, sizeof audio[0], 1024, fp);

    for(int idx=0; idx<samples; idx++) {
      int16_t audio_processed;
      int16_t i, q, phase;
      uint16_t magnitude;

      int32_t audio_clipped = audio[idx];
      test_phase += 1.0f;
      audio_processed = process_speech(audio_clipped);
      int16_t audio_uncompressed = audio_processed;
      compress(audio_processed, env, 1024);
      the_modulator.process_sample(USB, audio_processed, i, q, magnitude, phase, 0, debug);
      fprintf(data_file, "%i %i %f %i %i %i %i %i %u %i %i %u %i\n", audio_uncompressed, audio_processed, env, debug.filtered_audio, debug.raw_i, debug.raw_q, debug.clipped_i, debug.clipped_q, debug.env, i, q, magnitude, phase);
    }

    if(samples < 1024) break;
  }

}

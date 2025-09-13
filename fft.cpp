#include "fft.h"
#include <cmath>
#include <cstdint>
#include <cstdio>

#ifndef SIMULATION
#include "pico/stdlib.h"
#endif

static const uint16_t max_m = 8; // the largest size of FFT supported
static const uint16_t max_n_over_2 = 1 << (max_m - 1);
static int16_t fixed_cos_table[max_n_over_2];
static int16_t fixed_sin_table[max_n_over_2];

void fft_initialise() {
  for (int i = 0; i < max_n_over_2; ++i) {
    fixed_cos_table[i] = float2fixed(cosf((float)i * M_PI / max_n_over_2));
    fixed_sin_table[i] = float2fixed(sinf((float)i * M_PI / max_n_over_2));
  }
}

#ifndef SIMULATION
unsigned __not_in_flash_func(bit_reverse)(unsigned x, unsigned m) {
#else
unsigned bit_reverse(unsigned x, unsigned m) {
#endif
    static const unsigned char lookup[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
  };
  x = (lookup[x&0xff] << 8) | lookup[x>>8];
  return x >> (16-m);
}

#ifndef SIMULATION
void __not_in_flash_func(fixed_fft)(int16_t reals[], int16_t imaginaries[], unsigned m, bool scale) {
#else
void fixed_fft(int16_t reals[], int16_t imaginaries[], unsigned m, bool scale) {
#endif
  uint16_t stage, subdft_size, span, j, i, ip;
  int16_t temp_real, temp_imaginary;
  int16_t top_real, top_imaginary;
  int16_t bottom_real, bottom_imaginary;
  int16_t imaginary_twiddle, real_twiddle;
  const unsigned n = 1 << m;

  // bit reverse data
  for (i = 0u; i < n; i++) {
    ip = bit_reverse(i, m);
    if (i < ip) {
      temp_real = reals[i];
      temp_imaginary = imaginaries[i];
      reals[i] = reals[ip];
      imaginaries[i] = imaginaries[ip];
      reals[ip] = temp_real;
      imaginaries[ip] = temp_imaginary;
    }
  }

  // butterfly multiplies
  for (stage = 0; stage < m; ++stage) {
     subdft_size = 2 << stage;
     span = subdft_size >> 1;
     uint16_t shift = (max_m - stage - 1);
     uint16_t apply_scaling_this_stage = stage & 1;
     uint16_t quarter_turn = 1 << (stage-1);

     for (j = 0; j < span; ++j) {


       // Treat rotations by zero as a special case
       if(j == 0)
       {
 
         for (i = j; i < n; i += subdft_size) {
           ip = i + span;
 
           top_real = reals[i];
           top_imaginary = imaginaries[i];
 
           temp_real = reals[ip];
           temp_imaginary = imaginaries[ip];
 
           bottom_real = top_real - temp_real;
           bottom_imaginary = top_imaginary - temp_imaginary;
 
           top_real = top_real + temp_real;
           top_imaginary = top_imaginary + temp_imaginary;
 
           reals[ip] = bottom_real>>apply_scaling_this_stage;
           imaginaries[ip] = bottom_imaginary>>apply_scaling_this_stage;
           reals[i] = top_real>>apply_scaling_this_stage;
           imaginaries[i] = top_imaginary>>apply_scaling_this_stage;
         }
 
       }
 
       // Treat rotations by 1/4 as a special case
       else if(j == quarter_turn)
       {
 
         for (i = j; i < n; i += subdft_size) {
           ip = i + span;
 
           top_real         = reals[i];
           top_imaginary    = imaginaries[i];
           bottom_real      = reals[ip];
           bottom_imaginary = imaginaries[ip];
 
           temp_real        = bottom_imaginary;
           temp_imaginary   = -bottom_real;
 
           bottom_real      = top_real - temp_real;
           bottom_imaginary = top_imaginary - temp_imaginary;
  
           top_real         = top_real + temp_real;
           top_imaginary    = top_imaginary + temp_imaginary;
 
           // after every second stage lose 1 bit
           reals[ip]       = bottom_real>>apply_scaling_this_stage;
           imaginaries[ip] = bottom_imaginary>>apply_scaling_this_stage;
           reals[i]        = top_real>>apply_scaling_this_stage;
           imaginaries[i]  = top_imaginary>>apply_scaling_this_stage;
         }
 
       }
 
       // Use full complex multiplier for other cases
       else
       {
         real_twiddle = fixed_cos_table[j << shift];
         imaginary_twiddle = -fixed_sin_table[j << shift];
 
         for (i = j; i < n; i += subdft_size) {
           ip = i + span;
 
           top_real         = reals[i];
           top_imaginary    = imaginaries[i];
           bottom_real      = reals[ip];
           bottom_imaginary = imaginaries[ip];
 
           temp_real      = product(bottom_real, real_twiddle) -
                            product(bottom_imaginary, imaginary_twiddle);
           temp_imaginary = product(bottom_real, imaginary_twiddle) +
                            product(bottom_imaginary, real_twiddle);
 
           bottom_real      = top_real - temp_real;
           bottom_imaginary = top_imaginary - temp_imaginary;
 
           top_real         = top_real + temp_real;
           top_imaginary    = top_imaginary + temp_imaginary;
 
           // after every second stage lose 1 bit
           reals[ip]       = bottom_real>>apply_scaling_this_stage;
           imaginaries[ip] = bottom_imaginary>>apply_scaling_this_stage;
           reals[i]        = top_real>>apply_scaling_this_stage;
           imaginaries[i]  = top_imaginary>>apply_scaling_this_stage;
         }
       }
      
    }
  }
}
 
#ifndef SIMULATION
void __not_in_flash_func(fixed_ifft)(int16_t reals[], int16_t imaginaries[], unsigned m) {
#else
void fixed_ifft(int16_t reals[], int16_t imaginaries[], unsigned m) {
#endif
  fixed_fft(imaginaries, reals, m, true);
}

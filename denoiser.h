#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "arm_math.h"

#define DENOISER_FFT_SIZE (128)

    typedef struct
    {
        q15_t window[DENOISER_FFT_SIZE];
        q15_t prev_in_frame[DENOISER_FFT_SIZE/ 2];
        q15_t prev_out_frame[DENOISER_FFT_SIZE / 2];
        q15_t tmp_in_buf[2 * DENOISER_FFT_SIZE];
        q15_t tmp_out_buf[2 * DENOISER_FFT_SIZE];
        q31_t delta[DENOISER_FFT_SIZE / 4];
        arm_rfft_instance_q15 fft_i;
        arm_rfft_instance_q15 ifft_i;
    } denoiser_t;

    void denoiser_init(denoiser_t *self);
    void denoiser_process(denoiser_t *self, q15_t *const x, size_t nx);

#ifdef __cplusplus
}
#endif

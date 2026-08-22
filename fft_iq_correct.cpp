
#include "fft_iq_correct.h"

#include "pico/stdlib.h"
#include "rx_definitions.h"

#define FFT_SIZE (fft_size)
#define MU_SHIFT (12) // Equivalent to multiplying by mu = 1 / (2^12) = 0.000244

typedef struct
{
    int16_t re; // Real part (I)
    int16_t im; // Imaginary part (Q)
} cplx_q15_t;

// Persistent background calibration table (Size is FFT_SIZE / 2)
// This stores the complex leakage coefficient W(k) for each mirror pair.
static cplx_q15_t W_table[FFT_SIZE / 2] = {0};

inline cplx_q15_t __time_critical_func(cplx_mul_q15)(cplx_q15_t a, cplx_q15_t b)
{
    cplx_q15_t out;
    // Intermediate 32-bit math to prevent overflow before shifting
    int32_t re_32 = ((int32_t)a.re * b.re) - ((int32_t)a.im * b.im);
    int32_t im_32 = ((int32_t)a.re * b.im) + ((int32_t)a.im * b.re);

    // Scale back from Q30 to Q15
    out.re = (int16_t)(re_32 >> 15);
    out.im = (int16_t)(im_32 >> 15);
    return out;
}

void fft_iq_correct(int16_t *i, int16_t *q, uint16_t start_bin, uint16_t stop_bin)
{
    // We only loop through half the spectrum because bins k and (FFT_SIZE - k)
    // are processed together as mirror pairs.
    // Skip bin 0 (DC) and the Nyquist bin (FFT_SIZE / 2) as they have no unique mirrors.

    for (size_t k = start_bin; k <= stop_bin; k++)
    {
        const size_t k_mirror = FFT_SIZE - k;

        // 1. Fetch current raw bin values
        cplx_q15_t Y_k = {.re = i[k], .im = q[k]};
        cplx_q15_t Y_mirror = {.re = i[k_mirror], .im = q[k_mirror]};

        // 2. Form complex conjugates with narrow casting to fix compiler errors
        cplx_q15_t Y_mir_conj = {.re = Y_mirror.re, .im = (int16_t)(-Y_mirror.im)};
        cplx_q15_t Y_k_conj = {.re = Y_k.re, .im = (int16_t)(-Y_k.im)};

        // 3. Fetch the correction coefficient W(k)
        cplx_q15_t W = W_table[k];

        // 4. SYMMETRICAL CORRECTION PHASE
        // Correct Bin K:  Y_corr(k) = Y(k) - [ W(k) * Y*(-k) ]
        cplx_q15_t W_times_Y_mir_conj = cplx_mul_q15(W, Y_mir_conj);
        cplx_q15_t Y_k_corrected;
        Y_k_corrected.re = Y_k.re - W_times_Y_mir_conj.re;
        Y_k_corrected.im = Y_k.im - W_times_Y_mir_conj.im;

        // Correct Mirror Bin: Y_corr(-k) = Y(-k) - [ W* (k) * Y*(k) ]
        cplx_q15_t W_conj = {.re = W.re, .im = (int16_t)(-W.im)};
        cplx_q15_t W_conj_times_Y_k_conj = cplx_mul_q15(W_conj, Y_k_conj);
        cplx_q15_t Y_mirror_corrected;
        Y_mirror_corrected.re = Y_mirror.re - W_conj_times_Y_k_conj.re;
        Y_mirror_corrected.im = Y_mirror.im - W_conj_times_Y_k_conj.im;

        // Write corrected values back into the FFT spectrum array
        i[k] = Y_k_corrected.re;
        q[k] = Y_k_corrected.im;
        i[k_mirror] = Y_mirror_corrected.re;
        q[k_mirror] = Y_mirror_corrected.im;

        // 5. ESTIMATION PHASE (LMS Update Engine)
        // Adjust background matrix using the error vector of the primary bin
        cplx_q15_t error_term = cplx_mul_q15(Y_k_corrected, Y_mir_conj);

        int16_t delta_w_re = error_term.re >> MU_SHIFT;
        int16_t delta_w_im = error_term.im >> MU_SHIFT;

        // Update tracking table
        W_table[k].re += delta_w_re;
        W_table[k].im += delta_w_im;
    }
}

#include "denoiser.h"

#include <stdio.h>
#include "pico/stdlib.h"

#define NOISE_FRAMES_NUM (4)
#define NOISE_FRAMES_ROT_PERIOD (586) // 2.5s with current SR and frame length

#define KFRAME (28423)
#define ONE_MINUS_KFRAME (4344)

static q31_t noise_frames[NOISE_FRAMES_NUM][DENOISER_FFT_SIZE / 4];
static uint8_t noise_frame_idx = 0;
static uint8_t noise_frame_counter = 0;

// clang-format off

static const q31_t db_lut[600] = {
    -7891, -5918, -4764, -3946, -3310, -2792, -2353, -1973, -1638, -1338, -1066, -819, -591, -380, -184, 0,
    173, 335, 489, 635, 774, 906, 1033, 1154, 1270, 1382, 1489, 1593, 1693, 1789, 1882, 1973,
    2060, 2145, 2228, 2308, 2386, 2462, 2536, 2608, 2678, 2747, 2814, 2879, 2943, 3006, 3067, 3127,
    3185, 3243, 3299, 3355, 3409, 3462, 3514, 3565, 3616, 3665, 3714, 3762, 3809, 3855, 3901, 3946,
    3990, 4033, 4076, 4118, 4160, 4201, 4241, 4281, 4320, 4359, 4397, 4435, 4472, 4509, 4545, 4581,
    4616, 4651, 4685, 4719, 4753, 4786, 4819, 4852, 4884, 4916, 4947, 4978, 5009, 5040, 5070, 5100,
    5129, 5158, 5187, 5216, 5244, 5272, 5300, 5327, 5355, 5382, 5408, 5435, 5461, 5487, 5513, 5538,
    5564, 5589, 5613, 5638, 5663, 5687, 5711, 5735, 5758, 5782, 5805, 5828, 5851, 5873, 5896, 5918,
    5940, 5962, 5984, 6006, 6027, 6049, 6070, 6091, 6112, 6132, 6153, 6173, 6194, 6214, 6234, 6254,
    6273, 6293, 6312, 6332, 6351, 6370, 6389, 6407, 6426, 6445, 6463, 6481, 6500, 6518, 6536, 6553,
    6571, 6589, 6606, 6624, 6641, 6658, 6675, 6692, 6709, 6726, 6743, 6759, 6776, 6792, 6808, 6825,
    6841, 6857, 6873, 6889, 6904, 6920, 6936, 6951, 6967, 6982, 6997, 7012, 7027, 7043, 7057, 7072,
    7087, 7102, 7116, 7131, 7145, 7160, 7174, 7188, 7203, 7217, 7231, 7245, 7259, 7273, 7286, 7300,
    7314, 7327, 7341, 7354, 7368, 7381, 7394, 7408, 7421, 7434, 7447, 7460, 7473, 7486, 7498, 7511,
    7524, 7536, 7549, 7561, 7574, 7586, 7599, 7611, 7623, 7635, 7647, 7660, 7672, 7684, 7696, 7707,
    7719, 7731, 7743, 7754, 7766, 7778, 7789, 7801, 7812, 7824, 7835, 7846, 7858, 7869, 7880, 7891,
    7902, 7913, 7924, 7935, 7946, 7957, 7968, 7979, 7989, 8000, 8011, 8021, 8032, 8043, 8053, 8064,
    8074, 8084, 8095, 8105, 8115, 8126, 8136, 8146, 8156, 8166, 8176, 8186, 8197, 8206, 8216, 8226,
    8236, 8246, 8256, 8266, 8275, 8285, 8295, 8304, 8314, 8323, 8333, 8342, 8352, 8361, 8371, 8380,
    8390, 8399, 8408, 8417, 8427, 8436, 8445, 8454, 8463, 8472, 8481, 8490, 8499, 8508, 8517, 8526,
    8535, 8544, 8553, 8562, 8570, 8579, 8588, 8596, 8605, 8614, 8622, 8631, 8640, 8648, 8657, 8665,
    8673, 8682, 8690, 8699, 8707, 8715, 8724, 8732, 8740, 8749, 8757, 8765, 8773, 8781, 8789, 8797,
    8806, 8814, 8822, 8830, 8838, 8846, 8853, 8861, 8869, 8877, 8885, 8893, 8901, 8908, 8916, 8924,
    8932, 8939, 8947, 8955, 8962, 8970, 8978, 8985, 8993, 9000, 9008, 9015, 9023, 9030, 9038, 9045,
    9052, 9060, 9067, 9075, 9082, 9089, 9096, 9104, 9111, 9118, 9125, 9133, 9140, 9147, 9154, 9161,
    9168, 9175, 9183, 9190, 9197, 9204, 9211, 9218, 9225, 9232, 9238, 9245, 9252, 9259, 9266, 9273,
    9280, 9287, 9293, 9300, 9307, 9314, 9320, 9327, 9334, 9340, 9347, 9354, 9360, 9367, 9374, 9380,
    9387, 9393, 9400, 9407, 9413, 9420, 9426, 9433, 9439, 9445, 9452, 9458, 9465, 9471, 9477, 9484,
    9490, 9496, 9503, 9509, 9515, 9522, 9528, 9534, 9540, 9547, 9553, 9559, 9565, 9571, 9578, 9584,
    9590, 9596, 9602, 9608, 9614, 9620, 9626, 9632, 9638, 9644, 9650, 9656, 9662, 9668, 9674, 9680,
    9686, 9692, 9698, 9704, 9710, 9716, 9721, 9727, 9733, 9739, 9745, 9750, 9756, 9762, 9768, 9773,
    9779, 9785, 9791, 9796, 9802, 9808, 9813, 9819, 9825, 9830, 9836, 9842, 9847, 9853, 9858, 9864,
    9869, 9875, 9880, 9886, 9892, 9897, 9902, 9908, 9913, 9919, 9924, 9930, 9935, 9941, 9946, 9951,
    9957, 9962, 9968, 9973, 9978, 9984, 9989, 9994, 10000, 10005, 10010, 10015, 10021, 10026, 10031, 10036,
    10042, 10047, 10052, 10057, 10062, 10068, 10073, 10078, 10083, 10088, 10093, 10098, 10104, 10109, 10114, 10119,
    10124, 10129, 10134, 10139, 10144, 10149, 10154, 10159, 10164, 10169, 10174, 10179, 10184, 10189, 10194, 10199,
    10204, 10209, 10214, 10219, 10224, 10229, 10233, 10238, 10243, 10248, 10253, 10258, 10263, 10267, 10272, 10277,
    10282, 10287, 10291, 10296, 10301, 10306, 10311, 10315,
};

// clang-format on

static void subtract_noise(denoiser_t *self, q15_t *iq)
{
    q15_t mag_q15[DENOISER_FFT_SIZE / 4];
    static q31_t lpfMag[DENOISER_FFT_SIZE / 4];

    arm_cmplx_mag_fast_q15(iq, mag_q15, DENOISER_FFT_SIZE / 4);

    for (size_t i = 0; i < DENOISER_FFT_SIZE / 4; i++)
    {
        lpfMag[i] = ((ONE_MINUS_KFRAME * (q31_t)mag_q15[i]) >> 14) + ((KFRAME * (q31_t)lpfMag[i]) >> 14);
        if (lpfMag[i] < noise_frames[noise_frame_idx][i])
        {
            // make sure magnitudes are not zero to avoid division error
            if (lpfMag[i] < 1)
            {
                lpfMag[i] = 1;
            }
            noise_frames[noise_frame_idx][i] = lpfMag[i];
        }

        // find minimal amplitude for the bin
        const q15_t mmse = MIN(MIN(noise_frames[0][i], noise_frames[1][i]), MIN(noise_frames[2][i], noise_frames[3][i]));
        const q31_t snr = 16 * lpfMag[i] / mmse;

        const q31_t snr_dB = db_lut[snr < 600 ? snr : 599];
        q31_t alpha = (10 * 32767) - snr_dB;

        if (alpha > (15 * 32767))
        {
            alpha = (15 * 32767);
        }

        if (alpha < (1 * 32767))
        {
            alpha = (1 * 32767);
        }

        alpha = ((q63_t)alpha * self->delta[i]) >> 15;

        const q31_t a = 32767 - (16 * alpha / snr);
        const q31_t b = ((q31_t)(16 * 32767)) / (6 * snr);
        //                                       ^- spectral floor - decrease to reduce "musical noise"
        const q31_t gain = MAX(a, b);

        iq[(2 * i)] = (iq[(2 * i)] * gain) >> 15;
        iq[(2 * i) + 1] = (iq[(2 * i) + 1] * gain) >> 15;
        iq[2 * (DENOISER_FFT_SIZE - 1 - i)] = (iq[2 * (DENOISER_FFT_SIZE - 1 - i)] * gain) >> 15;
        iq[2 * (DENOISER_FFT_SIZE - 1 - i) + 1] = (iq[2 * (DENOISER_FFT_SIZE - 1 - i) + 1] * gain) >> 15;
    }

    // remove "dead" bins
    for (size_t i = DENOISER_FFT_SIZE / 4; i < DENOISER_FFT_SIZE - (DENOISER_FFT_SIZE / 4); i++)
    {
        iq[(2 * i)] = 0;
        iq[(2 * i) + 1] = 0;
    }

    // update the counters
    noise_frame_counter++;
    if (noise_frame_counter == NOISE_FRAMES_ROT_PERIOD)
    {
        noise_frame_idx++;
        if (noise_frame_idx == NOISE_FRAMES_NUM)
        {
            noise_frame_idx = 0;
        }
        for (size_t i = 0; i < DENOISER_FFT_SIZE / 2; i++)
        {
            noise_frames[noise_frame_idx][i] = Q31_MAX;
        }
        noise_frame_counter = 0;
    }
}

void denoiser_init(denoiser_t *self)
{
    float win[DENOISER_FFT_SIZE];

    arm_status status = arm_rfft_init_128_q15(&self->fft_i, 0, 1);
    hard_assert(status == ARM_MATH_SUCCESS);
    status = arm_rfft_init_128_q15(&self->ifft_i, 1, 1);
    hard_assert(status == ARM_MATH_SUCCESS);

    arm_hanning_f32(win, DENOISER_FFT_SIZE);
    arm_float_to_q15(win, self->window, DENOISER_FFT_SIZE);

    for (size_t i = 0; i < DENOISER_FFT_SIZE / 4; i++)
    {
        q31_t f = 15000 * i / (DENOISER_FFT_SIZE);
        if (f > 2000)
        {
            self->delta[i] = 1.5 * 37676;
        }
        else if (f > 1000)
        {
            self->delta[i] = 2.5 * 37676;
        }
        else
        {
            self->delta[i] = 1.0 * 37676;
        }

        for (size_t j = 0; j < NOISE_FRAMES_NUM; j++)
        {
            noise_frames[j][i] = Q31_MAX;
        }
    }
}

void denoiser_process(denoiser_t *self, q15_t *const x, size_t nx)
{
    hard_assert(nx == (DENOISER_FFT_SIZE / 2));
    arm_copy_q15(self->prev_in_frame, self->tmp_in_buf, DENOISER_FFT_SIZE / 2);
    arm_copy_q15(x, &self->tmp_in_buf[DENOISER_FFT_SIZE / 2], DENOISER_FFT_SIZE / 2);
    arm_copy_q15(x, self->prev_in_frame, nx);

    arm_mult_q15(self->tmp_in_buf, self->window, self->tmp_in_buf, DENOISER_FFT_SIZE);

    arm_rfft_q15(&self->fft_i, self->tmp_in_buf, self->tmp_out_buf);
    subtract_noise(self, self->tmp_out_buf);

    for (size_t i = 0; i < DENOISER_FFT_SIZE; i++)
    {
        self->tmp_out_buf[i] *= 8;
    }

    arm_rfft_q15(&self->ifft_i, self->tmp_out_buf, self->tmp_in_buf);

    arm_mult_q15(self->tmp_in_buf, self->window, self->tmp_in_buf, DENOISER_FFT_SIZE);
    arm_copy_q15(self->tmp_in_buf, x, DENOISER_FFT_SIZE / 2);
    arm_add_q15(self->prev_out_frame, x, x, DENOISER_FFT_SIZE / 2);

    for (size_t i = 0; i < nx; i++)
    {
        x[i] *= DENOISER_FFT_SIZE / 4;
    }

    arm_copy_q15(&self->tmp_in_buf[DENOISER_FFT_SIZE / 2], self->prev_out_frame, nx);
}

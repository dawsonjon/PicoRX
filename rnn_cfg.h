#pragma once

typedef struct
{
    const float *sigm_zeta;
    const float *sigm_nu;
    const float (*bias_update)[9];
    const float (*bias_gate)[9];
    const float (*w)[9][10];
    const float (*u)[9][9];
    const float (*fc_w)[10][9];
    const float (*fc_b)[10];
} rnn_cfg_t;

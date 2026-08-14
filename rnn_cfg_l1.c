#include "rnn_cfg.h"

#include "fastgrnn_rnn_params_l1.h"
#include "fastgrnn_fc_params_l1.h"

const rnn_cfg_t rnn_cfg_l1 = {
    .sigm_zeta = &GRNN0_SIGM_ZETA,
    .sigm_nu = &GRNN0_SIGM_NU,
    .bias_update = &GRNN0_BIAS_UPDATE,
    .bias_gate = &GRNN0_BIAS_GATE,
    .w = &GRNN0_W,
    .u = &GRNN0_U,
    .fc_w = &FC_W,
    .fc_b = &FC_B,
};

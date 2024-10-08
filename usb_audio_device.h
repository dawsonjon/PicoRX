/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _USB_AUDIO_DEVICE_H_
#define _USB_AUDIO_DEVICE_H_

#include "tusb.h"

#define USB_A_SAMPLE_RATE (15000)
#define SAMPLE_BUFFER_SIZE ((CFG_TUD_AUDIO_EP_SZ_IN / 2) - 1)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*usb_audio_device_tx_ready_handler_t)(void);
    typedef void (*usb_audio_device_mutevol_handler_t)(bool, int16_t);

    void usb_audio_device_init();
    void usb_audio_device_set_tx_ready_handler(usb_audio_device_tx_ready_handler_t handler);
    void usb_audio_device_set_mutevol_handler(usb_audio_device_mutevol_handler_t handler);
    void usb_audio_device_task();
    uint16_t usb_audio_device_write(const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // _USB_AUDIO_DEVICE_H_

/*
 * SPDX-FileCopyrightText: 2010-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

void app_main(void)
{
    RingbufHandle_t rb = xRingbufferCreate(1024, RINGBUF_TYPE_NOSPLIT);
    printf("Hello World\n");

    vRingbufferReset(rb);
}

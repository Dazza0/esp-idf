/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include "feat_a_helper.h"
#include "feat_b.h"

void feat_a_func(void)
{
    printf("feat_a\n");
    feat_a_helper();
    feat_b_func();
}

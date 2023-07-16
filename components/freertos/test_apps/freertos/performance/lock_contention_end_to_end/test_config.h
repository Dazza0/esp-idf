/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

/* This file must be included at the end of the FreeRTOSConfig.h. It contains
 * any FreeRTOS specific configurations that the test requires. */

#ifdef configNUMBER_OF_CORES
    #undef configNUMBER_OF_CORES
#endif

#ifdef configRUN_MULTIPLE_PRIORITIES
    #undef configRUN_MULTIPLE_PRIORITIES
#endif /* ifdef configRUN_MULTIPLE_PRIORITIES */

#define configNUMBER_OF_CORES            2
#define configRUN_MULTIPLE_PRIORITIES    1

#endif /* ifndef TEST_CONFIG_H */

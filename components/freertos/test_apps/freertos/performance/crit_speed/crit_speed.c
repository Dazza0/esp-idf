/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file crit_speed.c
 * @brief Test the speed of taskENTER_CRITICAL() and taskEXIT_CRITICAL()
 * functions without any lock contention.
 *
 * Procedure:
 *   - Measure elapsed time of taskENTER_CRITICAL()
 *   - Measure elapsed time of taskEXIT_CRITICAL()
 *   - Sample and average over portTEST_NUM_SAMPLES number of samples
 */

#if UPSTREAM_BUILD

    #ifndef TEST_CONFIG_H
        #error test_config.h must be included at the end of FreeRTOSConfig.h.
    #endif
/*-----------------------------------------------------------*/

#else /* UPSTREAM_BUILD */

/* ESP-IDF doesn't support upstream FreeRTOS test builds yet. We include
 * everything manually here. */

    #include "sdkconfig.h"
    #include  <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_cpu.h"
    #include "unity.h"

/*
 * Test macros to be defined by each port
 */
    #define portTEST_GET_TIME()    ( ( UBaseType_t ) esp_cpu_get_cycle_count() )
    #define portTEST_NUM_SAMPLES    128
/*-----------------------------------------------------------*/

#endif /* UPSTREAM_BUILD */

#if ( configNUMBER_OF_CORES < 2 )
    #error This test is for FreeRTOS SMP and therefore, requires at least 2 cores.
#endif /* if configNUMBER_OF_CORES != 2 */

#if ( configRUN_MULTIPLE_PRIORITIES != 1 )
    #error configRUN_MULTIPLE_PRIORITIES must be set to 1 for this test.
#endif /* if ( configRUN_MULTIPLE_PRIORITIES != 1 ) */

#ifndef portTEST_NUM_SAMPLES
    #error portTEST_NUM_SAMPLES must be defined indicating the number of samples
#endif

#ifndef portTEST_GET_TIME
    #error portTEST_GET_TIME must be defined in order to get current time
#endif
/*-----------------------------------------------------------*/

/**
 * @brief Test case "Critical Section Speed"
 */
static void Test_CriticalSectionSpeed( void );
/*-----------------------------------------------------------*/

static void Test_CriticalSectionSpeed( void )
{
    int i;

    UBaseType_t uxEntryElapsedCumulative = 0;
    UBaseType_t uxExitElapsedCumulative = 0;
    UBaseType_t uxTemp = 0;

    for( i = 0; i < portTEST_NUM_SAMPLES; i++ )
    {
        /* Test taskENTER_CRITICAL() elapsed time */
        uxTemp = portTEST_GET_TIME();
        taskENTER_CRITICAL();
        uxEntryElapsedCumulative += ( portTEST_GET_TIME() - uxTemp );

        /* Test taskEXIT_CRITICAL elapsed time */
        uxTemp = portTEST_GET_TIME();
        taskEXIT_CRITICAL();
        uxExitElapsedCumulative += ( portTEST_GET_TIME() - uxTemp );
    }

    printf( "taskENTER_CRITICAL() average elapsed time: %u\n", uxEntryElapsedCumulative / portTEST_NUM_SAMPLES );
    printf( "taskEXIT_CRITICAL() average elapsed time: %u\n", uxExitElapsedCumulative / portTEST_NUM_SAMPLES );
}
/*-----------------------------------------------------------*/

#if UPSTREAM_BUILD

/* Runs before every test, put init calls here. */
    void setUp( void )
    {
        /* Nothing to do */
    }
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
    void tearDown( void )
    {
        /* Nothing to do */
    }
/*-----------------------------------------------------------*/

    void vRunCriticalSectionSpeed( void )
    {
        UNITY_BEGIN();

        RUN_TEST( Test_CriticalSectionSpeed );

        UNITY_END();
    }
/*-----------------------------------------------------------*/

#else /* UPSTREAM_BUILD */

/* ESP-IDF doesn't support upstream FreeRTOS test builds yet. Use ESP-IDF
 * specific test registration macro. */

    TEST_CASE( "Test Performance: Critical Section Speed", "[freertos]" )
    {
        Test_CriticalSectionSpeed();
    }
/*-----------------------------------------------------------*/

#endif /* UPSTREAM_BUILD */

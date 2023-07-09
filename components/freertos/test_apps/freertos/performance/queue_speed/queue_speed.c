/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file queue_speed.c
 * @brief Test the speed of xQueueSend() and xQueueReceive() when not blocking
 *
 * Procedure:
 *   - Measure elapsed time of xQueueSend()
 *   - Measure elapsed time of xQueueReceive()
 *   - Sample and average over portTEST_NUM_SAMPLES number of samples
 */

#if UPSTREAM_BUILD

    #ifndef TEST_CONFIG_H
        #error test_config.h must be included at the end of FreeRTOSConfig.h.
    #endif
/*-----------------------------------------------------------*/

#else /* UPSTREAM_BUILD */

/* ESP-IDF doesn't support upstream FreeRTOS test builds yet. We include everything manually here. */

    #include "sdkconfig.h"
    #include  <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
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
 * @brief Test case "Queue Speed Non-Blocking"
 */
static void Test_QueueSpeedNonBlocking( void );
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the queue used in this test
 */
static QueueHandle_t xQueueHandle;
/*-----------------------------------------------------------*/

static void Test_QueueSpeedNonBlocking( void )
{
    int i;

    UBaseType_t uxSendElapsedCumulative = 0;
    UBaseType_t uxRecvElapsedCumulative = 0;
    UBaseType_t uxTemp = 0;

    for( i = 0; i < portTEST_NUM_SAMPLES; i++ )
    {
        /* Test xQueueSend() elapsed time */
        uxTemp = portTEST_GET_TIME();
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueSend( xQueueHandle, &i, 0 ), "xQueueSend() failed" );
        uxSendElapsedCumulative += ( portTEST_GET_TIME() - uxTemp );
    }

    for( i = 0; i < portTEST_NUM_SAMPLES; i++ )
    {
        int iValue;
        uxTemp = portTEST_GET_TIME();
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueReceive( xQueueHandle, &iValue, 0 ), "xQueueReceive() failed" );
        uxRecvElapsedCumulative += ( portTEST_GET_TIME() - uxTemp );
    }

    printf( "xQueueSend() average elapsed time: %u\n", uxSendElapsedCumulative / portTEST_NUM_SAMPLES );
    printf( "xQueueReceive() average elapsed time: %u\n", uxRecvElapsedCumulative / portTEST_NUM_SAMPLES );
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
#if UPSTREAM_BUILD
    void setUp( void )
#else
    static void setup_idf( void )
#endif /* UPSTREAM_BUILD */
{
    xQueueHandle = xQueueCreate( portTEST_NUM_SAMPLES, sizeof( int ) );
    TEST_ASSERT_NOT_NULL_MESSAGE( xQueueHandle, "Queue creation failed" );
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
#if UPSTREAM_BUILD
    void tearDown( void )
#else
    static void teardown_idf( void )
#endif /* UPSTREAM_BUILD */
{
    vQueueDelete( xQueueHandle );
    xQueueHandle = NULL;
}
/*-----------------------------------------------------------*/

#if UPSTREAM_BUILD

    void vRunCriticalSectionSpeed( void )
    {
        UNITY_BEGIN();

        RUN_TEST( Test_QueueSpeedNonBlocking );

        UNITY_END();
    }
/*-----------------------------------------------------------*/

#else /* UPSTREAM_BUILD */

/* ESP-IDF doesn't support upstream FreeRTOS test builds yet. Use ESP-IDF specific test registration macro. */

    TEST_CASE( "Test Performance: Queue Speed", "[freertos]" )
    {
        setup_idf();
        Test_QueueSpeedNonBlocking();
        teardown_idf();
    }
/*-----------------------------------------------------------*/

#endif /* UPSTREAM_BUILD */

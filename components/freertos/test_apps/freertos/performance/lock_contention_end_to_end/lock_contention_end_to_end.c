/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lock_contention_end_to_end.c
 * @brief End to end speed test of lock contention scenario
 *
 * This tests recreates a common lock contention scenario under the BKL (i.e.
 * global locking) scheme. Each core has a producer task, consumer task, and a
 * queue.This is to simulate two independent stream of data being independently
 * processed in parallel on each core.
 *
 * The lower throughput under BKL due to lock contention between the multiple
 * cores should be demonstrated by this test.
 *
 * Procedure:
 *   - For each core
 *      - Create a queue
 *      - Create a producer task that sends data to the queue
 *      - Create a consumer task that reads data from the queue
 *   - Repeat for portTEST_NUM_SAMPLES iterations
 *      - Start the producer and consumer tasks
 *      - Measure time elapsed for producer/consumer task to send/receive
 *        portTEST_NUM_ITEMS to/from the queue
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
    #include "freertos/semphr.h"
    #include "esp_cpu.h"
    #include "unity.h"

/*
 * Test macros to be defined by each port
 */
    #define portTEST_GET_TIME()    ( ( UBaseType_t ) esp_cpu_get_cycle_count() )
    #define portTEST_NUM_SAMPLES    128
    #define portTEST_NUM_ITEMS      64
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
 * @brief Test case "Lock Contention"
 */
static void Test_LockContentionEndToEnd( void );
/*-----------------------------------------------------------*/

/**
 * @brief Producer task run on each core
 */
static void prvProducerTask( void * pvParameters );
/*-----------------------------------------------------------*/

/**
 * @brief Consumer task run on each core
 */
static void prvConsumerTask( void * pvParameters );
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the queues accessed by each core
 */
static QueueHandle_t xQueueHandles[ configNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the producer tasks created for each core
 */
static TaskHandle_t xProducerTaskHandles[ configNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Handles of the consumer tasks created for each core
 */
static TaskHandle_t xConsumerTaskHandles[ configNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Start times of each iteration for each core
 */
static UBaseType_t uxStartTimes[ configNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Cumulative elapsed time of all iterations for each core
 */
static UBaseType_t uxElapsedCumulative[ configNUMBER_OF_CORES ];
/*-----------------------------------------------------------*/

/**
 * @brief Counting semaphore to indicate completion of an iteration
 */
static SemaphoreHandle_t xIterDoneSem;
/*-----------------------------------------------------------*/

static void prvProducerTask( void * pvParameters )
{
    int iSamples;
    int iItems;

    for( iSamples = 0; iSamples < portTEST_NUM_SAMPLES; iSamples++ )
    {
        /* Wait to be started by consumer task */
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        /* Record the start time for this sample */
        uxStartTimes[ portGET_CORE_ID() ] = portTEST_GET_TIME();

        for( iItems = 0; iItems < portTEST_NUM_ITEMS; iItems++ )
        {
            int Temp = iItems;
            TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueSend( xQueueHandles[ portGET_CORE_ID() ], &Temp, 0 ), "xQueueSend() failed" );
        }
    }

    /* Wait to be deleted */
    vTaskSuspend( NULL );
}
/*-----------------------------------------------------------*/

static void prvConsumerTask( void * pvParameters )
{
    int iSamples;
    int iItems;

    for( iSamples = 0; iSamples < portTEST_NUM_SAMPLES; iSamples++ )
    {
        /* Wait to be started by main test function */
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        /* Start the producer task */
        xTaskNotifyGive( xProducerTaskHandles[ portGET_CORE_ID() ] );

        /* Consume queue messages sent by producer tasks */
        for ( iItems = 0; iItems < portTEST_NUM_ITEMS; iItems++ )
        {
            int Temp;
            TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xQueueReceive( xQueueHandles[ portGET_CORE_ID() ], &Temp, portMAX_DELAY ), "xQueueReceive() failed" );
            TEST_ASSERT_EQUAL_MESSAGE( iItems, Temp, "Received incorrect value" );
        }

        /* Record end time for this iteration and add it to the cumulative count */
        uxElapsedCumulative[ portGET_CORE_ID() ] += portTEST_GET_TIME() - uxStartTimes[ portGET_CORE_ID() ];

        /* Notify main task that iteration is complete */
        xSemaphoreGive( ( SemaphoreHandle_t ) pvParameters );
    }

    /* Wait to be deleted */
    vTaskSuspend( NULL );
}
/*-----------------------------------------------------------*/

static void Test_LockContentionEndToEnd( void )
{
    int iCore;
    int iIter;

    /* Run test for portTEST_NUM_SAMPLES number of iterations */
    for( iIter = 0; iIter < portTEST_NUM_SAMPLES; iIter++ )
    {
        for( iCore = 0; iCore < configNUMBER_OF_CORES; iCore++ )
        {
            /* Start the consumer task, which in turn starts the producer task.
             * Start them on the other cores first so that we don't get
             * preempted immediately. */
            if( iCore != portGET_CORE_ID() )
            {
                xTaskNotifyGive( xConsumerTaskHandles[ iCore ] );
            }
        }
        /* now start the consumer task of this core */
        xTaskNotifyGive( xConsumerTaskHandles[ portGET_CORE_ID() ] );

        /* Wait until both cores have completed this iteration */
        for( iCore = 0; iCore < configNUMBER_OF_CORES; iCore++ )
        {
            xSemaphoreTake( xIterDoneSem, portMAX_DELAY );
        }
    }

    /* Print average results */
    printf("Time taken to send %d items, averaged over %d samples\n", portTEST_NUM_ITEMS, portTEST_NUM_SAMPLES);
    for( iCore = 0; iCore < configNUMBER_OF_CORES; iCore++ )
    {
        printf("Core %d: %d\n", iCore, ( uxElapsedCumulative[ iCore ] / portTEST_NUM_SAMPLES ) );
    }
}
/*-----------------------------------------------------------*/

/* Runs before every test, put init calls here. */
#if UPSTREAM_BUILD
    void setUp( void )
#else
    static void setup_idf( void )
#endif /* UPSTREAM_BUILD */
{
    int i;

    /* Create counting semaphore to indicate iteration completion */
    xIterDoneSem = xSemaphoreCreateCounting( configNUMBER_OF_CORES, 0 );
    TEST_ASSERT_NOT_NULL_MESSAGE( xIterDoneSem, "Failed to create counting semaphore");

    /* Create separate queues and tasks for each core */
    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        BaseType_t xRet;

        /* Separate queues for each core */
        xQueueHandles[ i ] = xQueueCreate( portTEST_NUM_ITEMS, sizeof(int) );
        TEST_ASSERT_NOT_NULL_MESSAGE( xQueueHandles[ i ], "Queue creation failed" );

        /* A producer task for each core to send to its queue */
        xRet = xTaskCreateAffinitySet( prvProducerTask,
                                       "prod",
                                       configMINIMAL_STACK_SIZE * 4,
                                       ( void * ) xIterDoneSem,
                                       uxTaskPriorityGet( NULL ) + 1,
                                       ( 1 << i ),
                                       &xProducerTaskHandles[ i ] );
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xRet, "Creating producer task failed" );

        /* A consumer task for each core to read items from its queue. The
         * consumer tasks has a higher priority than producer tasks so that sent
         * items are read immediately. */
        xRet = xTaskCreateAffinitySet( prvConsumerTask,
                                       "con",
                                       configMINIMAL_STACK_SIZE * 4,
                                       ( void * ) xIterDoneSem,
                                       uxTaskPriorityGet( NULL ) + 2,
                                       ( 1 << i ),
                                       &xConsumerTaskHandles[ i ] );
        TEST_ASSERT_EQUAL_MESSAGE( pdTRUE, xRet, "Creating consumer task failed" );
    }
}
/*-----------------------------------------------------------*/

/* Runs after every test, put clean-up calls here. */
#if UPSTREAM_BUILD
    void tearDown( void )
#else
    static void teardown_idf( void )
#endif /* UPSTREAM_BUILD */
{
    int i;

    vSemaphoreDelete( xIterDoneSem );

    /* Delete tasks and queues */
    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        vTaskDelete( xProducerTaskHandles[ i ] );
        vTaskDelete( xConsumerTaskHandles[ i ] );
        vQueueDelete( xQueueHandles[ i ] );
    }
}
/*-----------------------------------------------------------*/

#if UPSTREAM_BUILD

    void vRunCriticalSectionSpeed( void )
    {
        UNITY_BEGIN();

        RUN_TEST( Test_LockContentionEndToEnd );

        UNITY_END();
    }
/*-----------------------------------------------------------*/

#else /* UPSTREAM_BUILD */

/* ESP-IDF doesn't support upstream FreeRTOS test builds yet. Use ESP-IDF specific test registration macro. */

    TEST_CASE( "Test Performance: Lock Contention End To End", "[freertos]" )
    {
        setup_idf();
        Test_LockContentionEndToEnd();
        teardown_idf();
    }
/*-----------------------------------------------------------*/

#endif /* UPSTREAM_BUILD */

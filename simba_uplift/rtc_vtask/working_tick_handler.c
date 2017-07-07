/**
 *  * @section License
 *   *
 *    * The MIT License (MIT)
 *     *
 *      * Copyright (c) 2014-2017, Erik Moqvist
 *       *
 *        * Permission is hereby granted, free of charge, to any person
 *         * obtaining a copy of this software and associated documentation
 *          * files (the "Software"), to deal in the Software without
 *           * restriction, including without limitation the rights to use, copy,
 *            * modify, merge, publish, distribute, sublicense, and/or sell copies
 *             * of the Software, and to permit persons to whom the Software is
 *              * furnished to do so, subject to the following conditions:
 *               *
 *                * The above copyright notice and this permission notice shall be
 *                 * included in all copies or substantial portions of the Software.
 *                  *
 *                   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *                    * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *                     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *                      * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *                       * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *                        * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *                         * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *                          * SOFTWARE.
 *                           *
 *                            * This file is part of the Simba project.
 *                             */

#undef BIT

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_intr.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include "nvs_flash.h"

/* The main function is defined by the user in main.c. */
extern int main();

typedef struct {
    int type;                  /*!< event type */
    int group;                 /*!< timer group */
    int idx;                   /*!< timer number */
    uint64_t counter_val;      /*!< timer counter value */
    double time_sec;           /*!< calculated time from counter value */
} timer_event_t;


xQueueHandle timer_queue;
static void RAM_CODE sys_port_tick()
{
    /* Clear the interrupt flag and configure the timer to raise an
 *        alarm on the next timeout. */
    ESP32_TIMG0->INT.CLR = 1;
    ESP32_TIMG0->TIMER[0].CONFIG |= (ESP32_TIMG_TIMER_CONFIG_ALARM_EN);
    if(0) sys_tick_isr();
    
    timer_event_t evt;
    
    
    evt.type = 2;
    evt.group = 2;
    evt.idx = 2;
    evt.counter_val = 2;
    evt.time_sec = 2;
    xQueueSendFromISR(timer_queue, &evt, NULL);
}
static int sys_port_module_init(void)
{
    /* Setup interrupt handler. */
    
    timer_queue = xQueueCreate(10, sizeof(timer_event_t));
    ESP_INTR_DISABLE(ESP32_CPU_INTR_SYS_TICK_NUM);
    esp_xt_set_interrupt_handler(ESP32_CPU_INTR_SYS_TICK_NUM,
                                 (xt_handler)sys_port_tick,
                                 NULL);
    intr_matrix_set(xPortGetCoreID(),
                    ESP32_INTR_SOURCE_TG0_T0_LEVEL,
                    ESP32_CPU_INTR_SYS_TICK_NUM);

    /* Configure and start the system tick timer. */
    ESP32_TIMG0->TIMER[0].ALARMLO = (40000000 / CONFIG_SYSTEM_TICK_FREQUENCY);
    ESP32_TIMG0->TIMER[0].ALARMHI = 0;
    ESP32_TIMG0->TIMER[0].CONFIG = (ESP32_TIMG_TIMER_CONFIG_ALARM_EN
                                    | ESP32_TIMG_TIMER_CONFIG_LEVEL_INT_EN
                                    | ESP32_TIMG_TIMER_CONFIG_DIVIDER(1)
                                    | ESP32_TIMG_TIMER_CONFIG_AUTORELOAD
                                    | ESP32_TIMG_TIMER_CONFIG_INCREASE
                                    | ESP32_TIMG_TIMER_CONFIG_EN);
    ESP32_TIMG0->INT.ENA = 1;
    ESP_INTR_ENABLE(ESP32_CPU_INTR_SYS_TICK_NUM);
    
    
    while(1) {
        timer_event_t evt;
        printf("waiting for event to be received !\n" )  ;
        xQueueReceive(timer_queue, &evt, portMAX_DELAY);
        /*Show timer event from interrupt*/
        printf("-------INTR TIME EVT--------\n");
        printf("TG[%d] timer[%d] alarm evt\n", evt.group, evt.idx);
        printf("reg: ");
        printf("time: %.8f S\n", evt.time_sec);
        /*Read timer value from task*/
        printf("======TASK TIME======\n");
        uint64_t timer_val;
        esp_timer_get_counter_value(evt.group, evt.idx, &timer_val);
        double time;
        esp_timer_get_counter_time_sec(evt.group, evt.idx, &time);
        printf("TG[%d] timer[%d] alarm evt\n", evt.group, evt.idx);
        printf("reg: ");
        printf("time: %.8f S\n", time);
    }
    
    
    return (0);
}

__attribute__ ((noreturn))
static void sys_port_stop(int error)
{
    while (1);
}

static void sys_port_panic_putc(char c)
{
}

__attribute__ ((noreturn))
static void sys_port_reboot()
{
    esp_esp_restart();
}

static int sys_port_backtrace(void **buf_pp, size_t size)
{
    return (0);
}

static int sys_port_get_time_into_tick()
{
    return (0);
}

static void RAM_CODE sys_port_lock(void)
{
    portDISABLE_INTERRUPTS();
}

static void RAM_CODE sys_port_unlock(void)
{
    portENABLE_INTERRUPTS();
}

static void sys_port_lock_isr(void)
{
}

static void sys_port_unlock_isr(void)
{
}

static cpu_usage_t sys_port_interrupt_cpu_usage_get(void)
{
    return (0);
}

static void sys_port_interrupt_cpu_usage_reset(void)
{
}

/**
 *  * Simba runs in this FreeRTOS task.
 *   */
static void main_task(void *events)
{
    char dummy = 0;
    struct thrd_t *thrd_p;

    /* Setup the stack pointers. Needed bacuase the stack is allocated
 *        on the heap. */
    thrd_p = (struct thrd_t *)(&dummy - CONFIG_SYS_SIMBA_MAIN_STACK_MAX + 128);
    thrd_port_set_main_thrd(thrd_p);
    thrd_port_set_main_thrd_stack_top(&dummy);
    
    /* Call the Simba application main function. */
    main();

    thrd_suspend(NULL);
}

int app_main()
{
    esp_nvs_flash_init();

    esp_xTaskCreatePinnedToCore(&main_task,
                                "simba",
                                CONFIG_SYS_SIMBA_MAIN_STACK_MAX / sizeof(StackType_t),
                                NULL,
                                5,
                                NULL,
                                0);

    return (0);
}

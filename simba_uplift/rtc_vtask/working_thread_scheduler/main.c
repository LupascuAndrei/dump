
n License
 *
 *  * The MIT License (MIT)
 *   *
 *    * Copyright (c) 2014-2017, Erik Moqvist
 *     *
 *      * Permission is hereby granted, free of charge, to any person
 *       * obtaining a copy of this software and associated documentation
 *        * files (the "Software"), to deal in the Software without
 *         * restriction, including without limitation the rights to use, copy,
 *          * modify, merge, publish, distribute, sublicense, and/or sell copies
 *           * of the Software, and to permit persons to whom the Software is
 *            * furnished to do so, subject to the following conditions:
 *             *
 *              * The above copyright notice and this permission notice shall be
 *               * included in all copies or substantial portions of the Software.
 *                *
 *                 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *                  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *                   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *                    * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *                     * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *                      * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *                       * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *                        * SOFTWARE.
 *                         *
 *                          * This file is part of the Simba project.
 *                           */
#include "uplift.h"
#include "simba.h"
#include <limits.h>
#define SECONDS_PER_MSB (INT_MAX / CONFIG_SYSTEM_TICK_FREQUENCY)
#define TICKS_PER_MSB   (SECONDS_PER_MSB * CONFIG_SYSTEM_TICK_FREQUENCY)
#define CONFIG_START_SHELL 0     


                     
int main()
{
    printf("Hello from simba !\n");
    /* Start the system. */
    sys_start();
    printf("TICKS_PER_MSB = %d\n", TICKS_PER_MSB ) ; 
    printf("Hello 2! sleeping : \n");
    thrd_sleep_ms(13000);
    printf("Woke up !\n");
    
    struct pin_driver_t pin1 ;
    struct pin_driver_t pin2 ;
    struct pin_driver_t pin3 ;
    
    pin_init(&pin1, &pin_gpio12_dev, PIN_OUTPUT);
    pin_init(&pin2, &pin_gpio15_dev, PIN_OUTPUT);
    pin_init(&pin3, &pin_gpio05_dev, PIN_OUTPUT);
    
    while(1)
    {
        pin_toggle(&pin1);
        time_busy_wait_us(100000 * 3);
        pin_toggle(&pin2);
        time_busy_wait_us(100000 * 3);
        pin_toggle(&pin3);
        time_busy_wait_us(100000 * 3);
    }
    
    return (0);
}


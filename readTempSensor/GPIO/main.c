/**
 * @file        main.c
 * @brief       GPIO Example
 * @details
 */

/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/***** Includes *****/
#include <stdio.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "pb.h"
#include "board.h"
#include "gpio.h"

/***** Definitions *****/

#ifdef BOARD_FTHR_REVA
#define MXC_GPIO_PORT_IN MXC_GPIO1
#define MXC_GPIO_PIN_IN MXC_GPIO_PIN_7

#define MXC_GPIO_PORT_OUT MXC_GPIO2
#define MXC_GPIO_PIN_OUT MXC_GPIO_PIN_0

#define MXC_GPIO_PORT_INTERRUPT_IN MXC_GPIO0
#define MXC_GPIO_PIN_INTERRUPT_IN MXC_GPIO_PIN_2

#define MXC_GPIO_PORT_INTERRUPT_STATUS MXC_GPIO0
#define MXC_GPIO_PIN_INTERRUPT_STATUS MXC_GPIO_PIN_9
#endif

/***** Globals *****/

/***** Functions *****/
void gpio_isr(void *cbdata)
{
    mxc_gpio_cfg_t *cfg = cbdata;
    MXC_GPIO_OutToggle(cfg->port, cfg->mask); // toggle the output for port_mask, mask is the pin
}

int main(void)
{
    // mxc_gpio_cfg_t is structure for GPIO port
    mxc_gpio_cfg_t gpio_in;
    mxc_gpio_cfg_t gpio_out;
    mxc_gpio_cfg_t gpio_interrupt;
    mxc_gpio_cfg_t gpio_interrupt_status;

    printf("\n\n***** GPIO Example *****\n\n");
    printf("1. This example reads P1.7 (SW2 input) and outputs the same state onto\n");
    printf("   P2.0 (LED1).\n");
    printf("2. An interrupt is set to occur when SW1 (P0.2) is pressed. P0.9 (SDIO3 pin\n");
    printf("   on header J4) toggles when that interrupt occurs.\n\n");

    /* Setup interrupt status pin as an output so we can toggle it on each interrupt. */
    gpio_interrupt_status.port = MXC_GPIO_PORT_INTERRUPT_STATUS; // pointer to GPIO register, port 0
    gpio_interrupt_status.mask = MXC_GPIO_PIN_INTERRUPT_STATUS; // pin mask, pin 9
    gpio_interrupt_status.pad = MXC_GPIO_PAD_NONE; // pad type (no pull-up or pull-down)
    gpio_interrupt_status.func = MXC_GPIO_FUNC_OUT; // GPIO is output
    gpio_interrupt_status.vssel = MXC_GPIO_VSSEL_VDDIOH; // output voltage is VDDIOH, ~3V
    gpio_interrupt_status.drvstr = MXC_GPIO_DRVSTR_0; // drive strength
    MXC_GPIO_Config(&gpio_interrupt_status); // configure the GPIO

    /*
     *   Set up interrupt pin.
     *   Switch on EV kit is open when non-pressed, and grounded when pressed.  Use an internal pull-up so pin
     *     reads high when button is not pressed.
     */
    gpio_interrupt.port = MXC_GPIO_PORT_INTERRUPT_IN;// Port 0
    gpio_interrupt.mask = MXC_GPIO_PIN_INTERRUPT_IN; // pin 2 (SW1)
    gpio_interrupt.pad = MXC_GPIO_PAD_PULL_UP; // pull-up, since it is pull-up, interrupt is probably active low
    gpio_interrupt.func = MXC_GPIO_FUNC_IN; // GPIO is input
    gpio_interrupt.vssel = MXC_GPIO_VSSEL_VDDIOH; // ~3V
    gpio_interrupt.drvstr = MXC_GPIO_DRVSTR_0; // drive strength
    MXC_GPIO_Config(&gpio_interrupt); // configure GPIO
    MXC_GPIO_RegisterCallback(&gpio_interrupt, gpio_isr, &gpio_interrupt_status); // the isr toggles the output pin
    MXC_GPIO_IntConfig(&gpio_interrupt, MXC_GPIO_INT_FALLING); // interrupt trigger on falling edge
    MXC_GPIO_EnableInt(gpio_interrupt.port, gpio_interrupt.mask); // enable interrupt
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO_PORT_INTERRUPT_IN))); // enable external interrupt

    /*
     *   Setup input pin.
     *   Switch on EV kit is open when non-pressed, and grounded when pressed.  Use an internal pull-up so pin
     *     reads high when button is not pressed.
     */
    gpio_in.port = MXC_GPIO_PORT_IN;
    gpio_in.mask = MXC_GPIO_PIN_IN; // P1.7, SW2
    gpio_in.pad = MXC_GPIO_PAD_PULL_UP;
    gpio_in.func = MXC_GPIO_FUNC_IN;
    gpio_in.vssel = MXC_GPIO_VSSEL_VDDIO;
    gpio_in.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_in);

    /* Setup output pin. */
    gpio_out.port = MXC_GPIO_PORT_OUT;
    gpio_out.mask = MXC_GPIO_PIN_OUT; // P2.0, LED1
    gpio_out.pad = MXC_GPIO_PAD_NONE;
    gpio_out.func = MXC_GPIO_FUNC_OUT;
    gpio_out.vssel = MXC_GPIO_VSSEL_VDDIO;
    gpio_out.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_out);

    while (1) {
        /* Read state of the input pin. */
        if (MXC_GPIO_InGet(gpio_in.port, gpio_in.mask)) {
            /* Input pin was high, set the output pin. */
            MXC_GPIO_OutSet(gpio_out.port, gpio_out.mask);
        } else {
            /* Input pin was low, clear the output pin. */
            MXC_GPIO_OutClr(gpio_out.port, gpio_out.mask);
        }
    }

    return 0;
}

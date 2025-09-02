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

/**
 * @file    main.c
 * @brief   Timer example
 * @details PWM Timer        - Outputs a PWM signal (2Hz, 30% duty cycle) on TMR2
 *          Continuous Timer - Outputs a continuous 1s timer on LED0 (GPIO toggles every 500s)
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_sys.h"
#include "nvic_table.h"
#include "gpio.h"
#include "board.h"
#include "tmr.h"
#include "led.h"
#include "pb.h"
#include "lp.h"
#include "lpgcr_regs.h"
#include "gcr_regs.h"
#include "pwrseq_regs.h"

#include "mxc_delay.h"

/***** Definitions *****/
#define SLEEP_MODE // Select between SLEEP_MODE and LP_MODE for LPTIMER

// Parameters for PWM output
#define OST_CLOCK_SOURCE MXC_TMR_IBRO_CLK // \ref mxc_tmr_clock_t
#define PWM_CLOCK_SOURCE MXC_TMR_ISO_CLK // \ref mxc_tmr_clock_t
#define CONT_CLOCK_SOURCE MXC_TMR_APB_CLK // \ref mxc_tmr_clock_t

// Parameters for Continuous timer
#define OST_FREQ 1 // (Hz)
#define OST_TIMER MXC_TMR4 // Can be MXC_TMR0 through MXC_TMR5

#define FREQ 1000 // (Hz)
#define DUTY_CYCLE 20 // (%)
#define PWM_TIMER MXC_TMR1 // must change PWM_PORT and PWM_PIN if changed

// Parameters for Continuous timer
#define CONT_FREQ 2 // (Hz)
#define CONT_TIMER0 MXC_TMR0 // Can be MXC_TMR0 through MXC_TMR5
#define CONT_TIMER1 MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

// Parameters for GPIO pins
#define MXC_GPIO_PIN_TMR0 MXC_GPIO_PIN_7
#define MXC_GPIO_PORT_TMR MXC_GPIO2
#define MXC_GPIO_PIN_TMR1 MXC_GPIO_PIN_8
    
mxc_gpio_cfg_t gpio_out_tmr0;
mxc_gpio_cfg_t gpio_out_tmr1;


// Check Frequency bounds
#if (FREQ == 0)
#error "Frequency cannot be 0."
#elif (FREQ > 100000)
#error "Frequency cannot be over 100000."
#endif

// Check duty cycle bounds
#if (DUTY_CYCLE < 0) || (DUTY_CYCLE > 100)
#error "Duty Cycle must be between 0 and 100."
#endif

// Toggles GPIO when continuous timer repeats
void ContinuousTimerHandler0(void)
{
    // Clear interrupt
    MXC_TMR_ClearFlags(CONT_TIMER0);
    MXC_GPIO_OutToggle(MXC_GPIO_PORT_TMR, MXC_GPIO_PIN_TMR0);
    LED_Toggle(0);
}

void ContinuousTimer0(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(CONT_TIMER0, MXC_TMR_IBRO_CLK, MXC_TMR_PRES_4096, 2);
    printf("TMR0: %d\n", periodTicks);

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */ 

    MXC_TMR_Shutdown(CONT_TIMER0);

    tmr.pres = MXC_TMR_PRES_4096;
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_IBRO_CLK;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(CONT_TIMER0, &tmr, 0) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }


    MXC_TMR_EnableInt(CONT_TIMER0);

    MXC_NVIC_SetVector(TMR0_IRQn, ContinuousTimerHandler0);
    NVIC_EnableIRQ(TMR0_IRQn);

    MXC_TMR_Start(CONT_TIMER0);

    printf("Continuous timer0 started.\n\n");
}


// Toggles GPIO when continuous timer repeats
void ContinuousTimerHandler1(void)
{
    // Clear interrupt
    MXC_TMR_ClearFlags(CONT_TIMER1);
    MXC_GPIO_OutToggle(MXC_GPIO_PORT_TMR, MXC_GPIO_PIN_TMR1);
    LED_Toggle(1);

}


void ContinuousTimer1(void)
{

    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(CONT_TIMER1, MXC_TMR_IBRO_CLK, TMR_PRES_128, 2);
    printf("TMR1: %d\n", periodTicks);

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(CONT_TIMER1);

    tmr.pres = TMR_PRES_128;
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_IBRO_CLK;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(CONT_TIMER1, &tmr, 0) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }

    MXC_TMR_EnableInt(CONT_TIMER1);

    MXC_NVIC_SetVector(TMR1_IRQn, ContinuousTimerHandler1);
    NVIC_EnableIRQ(TMR1_IRQn);

    MXC_TMR_Start(CONT_TIMER1);

    printf("Continuous timer1 started.\n\n");
}


// *****************************************************************************
int main(void)
{
    //Exact timer operations can be found in tmr_utils.c

    printf("\n**************************Timer Example **************************\n\n");


    /* Setup output pin. */
    gpio_out_tmr0.port = MXC_GPIO_PORT_TMR;
    gpio_out_tmr0.mask = MXC_GPIO_PIN_TMR0;
    gpio_out_tmr0.pad = MXC_GPIO_PAD_NONE;
    gpio_out_tmr0.func = MXC_GPIO_FUNC_OUT;
    gpio_out_tmr0.vssel = MXC_GPIO_VSSEL_VDDIO;
    gpio_out_tmr0.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_out_tmr0);

    gpio_out_tmr1.port = MXC_GPIO_PORT_TMR;
    gpio_out_tmr1.mask = MXC_GPIO_PIN_TMR1;
    gpio_out_tmr1.pad = MXC_GPIO_PAD_NONE;
    gpio_out_tmr1.func = MXC_GPIO_FUNC_OUT;
    gpio_out_tmr1.vssel = MXC_GPIO_VSSEL_VDDIO;
    gpio_out_tmr1.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_out_tmr1);

    while (!PB_Get(0)) {}
    // Start continuous timer
    ContinuousTimer0();
    ContinuousTimer1();
    MXC_Delay(MXC_DELAY_SEC(1));

    while (1) {


    }

    return 0;
}

/**
 * @file    main.c
 * @brief   SPI Temperature Sensor Demo
 * @details Shows the interaction between MAX78000FTHR and MAX31723PMB1.
 *          Topics: SPI
 */


/***** Includes *****/
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "dma.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "mxc_sys.h"

#include "nvic_table.h"
#include "spi.h"
#include "uart.h"
#include "pb.h"
#include "gpio.h"
#include "tmr.h"
#include "led.h"
#include "lp.h"
#include "lpgcr_regs.h"
#include "gcr_regs.h"
#include "pwrseq_regs.h"




/***** Preprocessors *****/
#define MASTERSYNC 0
#define MASTERASYNC 1
//#define MASTERDMA 0

/***** Definitions *****/
#define DATA_LEN 2 
// SLEEP mode ensures all oscillators are enabled
#define SLEEP_MODE 
// use APB clock source for continuous timer
#define CONT_CLOCK_SOURCE MXC_TMR_APB_CLK
// timer interrupt frequency (Hz)
#define CONT_FREQ 1
// timer 0
#define CONT_TIMER MXC_TMR0

// frequency = 100 kHz
#define SPI_SPEED 100000
#define SPI MXC_SPI4
// for async transasction
#define SPI_IRQ SPI4_IRQn
#define SS_IDX 0

#define FTHR_Defined 0

/***** Temperature Sensor *****/
// max conversion time is 200ms
// resolution for the temperature reading (9 - 12 bits)
#define TEMP_RES 12 

// (P1.27, SW2)
#define IN_INTERRUPT_PORT MXC_GPIO1
#define IN_INTERRUPT_PIN MXC_GPIO_PIN_27
// (P2.1, LED1)
#define OUT_INTERRUPT_PORT MXC_GPIO2
#define OUT_INTERRUPT_PIN MXC_GPIO_PIN_1

// define SPI GPIO pins for setting the VDDIO
#define MXC_GPIO_PORT MXC_GPIO1
#define MXC_GPIO_PIN_SS MXC_GPIO_PIN_0
#define MXC_GPIO_PIN_MOSI MXC_GPIO_PIN_1
#define MXC_GPIO_PIN_MISO MXC_GPIO_PIN_2
#define MXC_GPIO_PIN_SCLK MXC_GPIO_PIN_3

// define SPI IO pins
mxc_gpio_cfg_t gpio_spi_pins;

/***** Globals *****/
uint8_t rx_data[DATA_LEN];
uint8_t tx_data[DATA_LEN];
volatile int SPI_FLAG;
volatile int SW_SPI_FLAG = 0; // SPI transaction thru SW2 GPIO interrupt
volatile int TMR_SPI_FLAG = 0; // SPI transaction thru timer interrupt
volatile uint8_t DMA_FLAG = 0;
mxc_spi_req_t req; // SPI transaction request

uint8_t temp_MSB; // most significant byte of temperature reading
uint8_t temp_LSB; // least significant byte of temperature reading

// GPIO pins for interrupt
mxc_gpio_cfg_t gpio_interrupt;
mxc_gpio_cfg_t gpio_interrupt_status;

int interruptInterval = 5; // 5 seconds
int interruptCounter = 0;

/***** Functions *****/
void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);
}

void SPI_Callback(mxc_spi_req_t *req, int error)
{
    SPI_FLAG = error;
}

// NOTE (BH): I would avoid firing off another transaction inside ISR
//            In general, we want to be OUT of an ISR AS SOON AS WE CAN!
void gpio_callback(void *cbdata)
{
    // EXAMPLE (BH): Just set a flag here and respond in main
    // gpio_isr_flag = 1;
    // ... (in main)
    // while(1) {if (gpio_isr_flag == 1)) { // do stuff, then set flag low}}

    // disable push button interrupt
    NVIC_DisableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT)));

    mxc_gpio_cfg_t *cfg = cbdata;
    MXC_GPIO_OutToggle(cfg->port, cfg->mask);

    SW_SPI_FLAG = 1;
}

// NOTE (BH)
// If NVIC is not copied to RAM: GPIO1_IRQHandler
// If NVIC is copied to RAM:     arbitrary name "gpio_isr"
// 
// To copy NVIC to RAM: NVIC_SetRAM() in "nvic_table.h"
void gpio_isr(void)
{
    MXC_Delay(MXC_DELAY_MSEC(100)); // Debounce
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT));
}


// Toggles GPIO when continuous timer repeats
void ContinuousTimerHandler(void)
{
    // Clear interrupt
    // always clear the timer flag in the handler
    MXC_TMR_ClearFlags(CONT_TIMER);

    // update the interrupt counter
    if (interruptCounter < interruptInterval) {
        interruptCounter++;
    } else {
        // disable push button interrupt
        NVIC_DisableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT)));
        LED_Toggle(0); // toggle LED index 0
        TMR_SPI_FLAG = 1;
        interruptCounter = 0;
    }
    
}

void ContinuousTimer(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(CONT_TIMER, CONT_CLOCK_SOURCE, 32, CONT_FREQ);

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(CONT_TIMER);

    tmr.pres = TMR_PRES_32; // prescaler
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_32; // 32-bit mode
    tmr.clock = CONT_CLOCK_SOURCE; // APB
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(CONT_TIMER, &tmr, 0) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }

    MXC_TMR_EnableInt(CONT_TIMER);

    MXC_NVIC_SetVector(TMR0_IRQn, ContinuousTimerHandler);
    NVIC_EnableIRQ(TMR0_IRQn);

    MXC_TMR_Start(CONT_TIMER);

    printf("\n\n****   CONTINUOUS TIMER START   ****\n\n");
}


int main(void)
{
    int retVal;
    mxc_spi_pins_t spi_pins;

    printf("\n\n\n*********************** SPI TEMPERATURE READ TEST ********************\n");
    printf("This example configures SPI to get a single temperture reading from\n");
    printf("MAX31723 to AD-APARD32690-SL when an interrupt is triggered by pressing SW2\n");
    printf("or by a 5-second timer. The initial press of SW2 enables the timer.\n");
    printf("Both the timer interrupts and GPIO (SW2) interrupts toggle LED1.\n");

    spi_pins.clock = TRUE;
    spi_pins.miso = TRUE;
    spi_pins.mosi = TRUE;
    spi_pins.sdio2 = FALSE;
    spi_pins.sdio3 = FALSE;
    spi_pins.ss0 = TRUE; // slave select pin 0
    spi_pins.ss1 = FALSE; 
    spi_pins.ss2 = FALSE;
    spi_pins.vddioh = MXC_GPIO_VSSEL_VDDIOH;


#ifdef BOARD_EVKIT_V1
    printf("\nBoard: BOARD_EVKIT_V1\n");
#else
    printf("\nBoard: AD-APARD32690-SL\n");
#endif


#if MASTERSYNC
    printf("Performing blocking (synchronous) transactions...\n");
#elif MASTERASYNC
    printf("Performing non-blocking (asynchronous) transactions...\n");
#endif

    // set up interrupt
    MXC_NVIC_SetVector(SPI_IRQ, SPI_IRQHandler);

    // initialize the tx buffer with 0x0
    // initialize the rx buffer with 0x0
    memset(tx_data, 0x00, DATA_LEN * sizeof(uint8_t));
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 

    // Setup tx_data to read configuration register
    tx_data[0] = 0x00; // configuration register address
    tx_data[1] = 0x00; // dummy byte

    /* Setup interrupt status pin as an output so we can toggle it on each interrupt. */
    gpio_interrupt_status.port = OUT_INTERRUPT_PORT;
    gpio_interrupt_status.mask = OUT_INTERRUPT_PIN;
    gpio_interrupt_status.pad = MXC_GPIO_PAD_NONE;
    gpio_interrupt_status.func = MXC_GPIO_FUNC_OUT;
    gpio_interrupt_status.vssel = MXC_GPIO_VSSEL_VDDIO;
    gpio_interrupt_status.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_interrupt_status);

    /*
     *   Set up interrupt pin.
     *   Switch on EV kit is open when non-pressed, and grounded when pressed.  Use an internal pull-up so pin
     *     reads high when button is not pressed.
     */
    gpio_interrupt.port = IN_INTERRUPT_PORT;
    gpio_interrupt.mask = IN_INTERRUPT_PIN;
    gpio_interrupt.pad = MXC_GPIO_PAD_NONE;
    gpio_interrupt.func = MXC_GPIO_FUNC_IN;
    gpio_interrupt.vssel = MXC_GPIO_VSSEL_VDDIOH;
    gpio_interrupt.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_interrupt);
    MXC_GPIO_RegisterCallback(&gpio_interrupt, gpio_callback, &gpio_interrupt_status);
    MXC_GPIO_IntConfig(&gpio_interrupt, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(gpio_interrupt.port, gpio_interrupt.mask);
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT)));
    MXC_NVIC_SetVector(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT)), gpio_isr);

    // Configure the peripheral
    // initialize the SPI port
    // master mode
    // not quad mode
    // 1 slave
    // CS of ss0 is active high, 1
    retVal = MXC_SPI_Init(SPI, MXC_SPI_TYPE_MASTER,
        MXC_SPI_INTERFACE_STANDARD, 
        1, 
        1, 
        SPI_SPEED,
        spi_pins);
    if (retVal != E_NO_ERROR) {
        printf("SPI Initialization ERROR\n");
        return retVal;
    }

    gpio_spi_pins.port = MXC_GPIO_PORT;
    gpio_spi_pins.func = MXC_GPIO_FUNC_ALT1;
    gpio_spi_pins.mask = MXC_GPIO_PIN_SS |
        MXC_GPIO_PIN_MISO |
        MXC_GPIO_PIN_MOSI |
        MXC_GPIO_PIN_SCLK;
    gpio_spi_pins.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_spi_pins);

    printf("SPI Initialization SUCCESS\n");

    // configure SPI mode
    // for SPI_MODE_3, CPHA = CPOL = 1
    retVal = MXC_SPI_SetMode(SPI, SPI_MODE_3);	
    if (retVal != E_NO_ERROR) {
        printf("SPI MODE ERROR\n");
        return retVal;
    }

    printf("SPI Mode Initialization SUCCESS\n");

    //SPI Request
    req.spi = SPI;
    req.txData = (uint8_t *)tx_data;
    req.rxData = (uint8_t *)rx_data;
    req.txLen = DATA_LEN;
    req.rxLen = DATA_LEN;
    req.ssIdx = 0;
    req.ssDeassert = 1;
    req.txCnt = 0;
    req.rxCnt = 0;
    req.completeCB = (spi_complete_cb_t)SPI_Callback;
    SPI_FLAG = 1;

    retVal = MXC_SPI_SetDataSize(SPI, 8);

    if (retVal != E_NO_ERROR) {
        printf("\nSPI SET DATASIZE ERROR: %d\n", retVal);
        return retVal;
    }

    retVal = MXC_SPI_SetWidth(SPI, SPI_WIDTH_STANDARD);

    if (retVal != E_NO_ERROR) {
        printf("\nSPI SET WIDTH ERROR: %d\n", retVal);
        return retVal;
    }

    // Read the configuration register
    printf("\nReading Configuration Register...\n");

    // async transaction
    MXC_SPI_MasterTransactionAsync(&req);
    int tempCount = 0; // allow the following while loop to only toggle the LED once
    while (SPI_FLAG == 1) {
        if (tempCount == 0) {
            MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
            tempCount++;
        }
    }
    
    printf("Configuration Register: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (rx_data[1] >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }

    // write to configuration register
    tx_data[0] = 0x80;
    // config write to EEPROM, EEPROM write in progress, disable 1SHOT, interrupt mode, 12-bit precision, continuous conversion 
    tx_data[1] = 0b01000110;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 
    printf("\n\nWriting Configuration Register...\n");

    // async transaction
    MXC_SPI_MasterTransactionAsync(&req);
    tempCount = 0; // allow the following while loop to only toggle the LED once
    while (SPI_FLAG == 1) {
        if (tempCount == 0) {
            MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
            tempCount++;
        }   
    }
    
    printf("Configuration Register Value Sent: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (tx_data[1] >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }


    // read configuration register
    tx_data[0] = 0x00;
    tx_data[1] = 0x00;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 
    printf("\n\nReading Configuration Register...\n");
    // async transaction
    MXC_SPI_MasterTransactionAsync(&req);
    tempCount = 0; // allow the following while loop to only toggle the LED once
    SPI_FLAG = 1;
    while (SPI_FLAG == 1) {
        if (tempCount == 0) {
            MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
            tempCount++;
        }
    }

    printf("Configuration Register: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (rx_data[1] >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }


    // read temp MSB register
    tx_data[0] = 0x02;
    tx_data[1] = 0x00;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t));
    printf("\n\nReading Temperature MSB...\n");

    // async transaction
    MXC_SPI_MasterTransactionAsync(&req);
    tempCount = 0; // allow the following while loop to only toggle the LED once
    SPI_FLAG = 1;
    while (SPI_FLAG == 1) {
        if (tempCount == 0) {
            MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
            tempCount++;
        }
    }

    printf("Temperature MSB: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (rx_data[1] >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }
    printf("\nTemperature MSB: %d ", rx_data[1]);
    temp_MSB = rx_data[1];
    

    // read temp LSB register
    tx_data[0] = 0x01;
    tx_data[1] = 0x00;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 
    printf("\n\nReading Temperature LSB...\n");

    // async transaction
    MXC_SPI_MasterTransactionAsync(&req);
    tempCount = 0; // allow the following while loop to only toggle the LED once
    SPI_FLAG = 1;
    while (SPI_FLAG == 1) {
        if (tempCount == 0) {
            MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
            tempCount++;
        }
    }
    
    printf("Temperature LSB: ");
    for (int i = 7; i >= 0; i--) {
        printf("%d", (rx_data[1] >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }
    printf("\nTemperature LSB: %d ", rx_data[1]);
    temp_LSB = rx_data[1];
    printf("\nTemp_Fraction: %.4f\n", (double)(temp_LSB/256.0f));
    

    double temp_final = temp_MSB + temp_LSB/((float)256.0);
    printf("\nFinal Temperature: %.4f\n", temp_final);

    // Example (BH): Polling push button
    // while(PB_Get(0) != TRUE) {}

    // Example (BH): Respond to GPIO ISR Flag in main

    // Wait until button press to start PWM and continuous timers
    while (!PB_Get(0)) {}

    // enable push button interrupt
    NVIC_EnableIRQ(SPI_IRQ);
    // Start continuous timer
    ContinuousTimer();
    MXC_Delay(MXC_DELAY_SEC(1)); // pause for a second

    while(1){ // listen to interrupts
        if ((TMR_SPI_FLAG || SW_SPI_FLAG) == 1) {
            // read temp MSB register
            tx_data[0] = 0x02;
            tx_data[1] = 0x00;
            memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t));
            // printf("\n\nReading Temperature MSB...\n");

            // async transaction
            MXC_SPI_MasterTransactionAsync(&req);
            tempCount = 0; // allow the following while loop to only toggle the LED once
            SPI_FLAG = 1;
            while (SPI_FLAG == 1) {
                if (tempCount == 0) {
                    MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
                    tempCount++;
                }
            }

            // printf("Temperature MSB: ");
            // for (int i = 7; i >= 0; i--) {
            //     printf("%d", (rx_data[1] >> i) & 1);
            //     if (i == 4) {
            //         printf(" ");
            //     }
            // }
            // printf("\nTemperature MSB: %d ", rx_data[1]);
            temp_MSB = rx_data[1];
            

            // read temp LSB register
            tx_data[0] = 0x01;
            tx_data[1] = 0x00;
            memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 
            // printf("\n\nReading Temperature LSB...\n");

            // async transaction
            MXC_SPI_MasterTransactionAsync(&req);
            tempCount = 0; // allow the following while loop to only toggle the LED once
            SPI_FLAG = 1;
            while (SPI_FLAG == 1) {
                if (tempCount == 0) {
                    MXC_GPIO_OutToggle(gpio_interrupt_status.port, gpio_interrupt_status.mask);
                    tempCount++;
                }
            }
            
            // printf("Temperature LSB: ");
            // for (int i = 7; i >= 0; i--) {
            //     printf("%d", (rx_data[1] >> i) & 1);
            //     if (i == 4) {
            //         printf(" ");
            //     }
            // }
            // printf("\nTemperature LSB: %d ", rx_data[1]);
            temp_LSB = rx_data[1];
            // printf("\nTemp_Fraction: %.4f\n", (double)(temp_LSB/256.0f));
            

            double temp_final = temp_MSB + temp_LSB/((float)256.0);

            if (TMR_SPI_FLAG == 1) {
                printf("\nTIMER0: \n");
            } else {
                printf("\nSW2: \n");
            }
            printf("Final Temperature: %.4f\n", temp_final);
            // clear both timer and switch interrupt flags
            TMR_SPI_FLAG = 0;
            SW_SPI_FLAG = 0;
            // enable push button interrupt
            NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT))); 
        }
    }

    return 0;

}


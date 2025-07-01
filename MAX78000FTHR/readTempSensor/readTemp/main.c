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
#include "nvic_table.h"
#include "spi.h"
#include "uart.h"
#include "pb.h"
#include "gpio.h"


/***** Preprocessors *****/
#define MASTERSYNC 1

/***** Definitions *****/
#define DATA_LEN 2 
// frequency = 100 kHz
#define SPI_SPEED 100000

#define SPI_INSTANCE_NUM 1
#define SPI MXC_SPI0

// MUST modify spi_pins.ss1 so the board uses chip select 1
#define SS_IDX 1 

/***** Temperature Sensor *****/
// max conversion time is 200ms
// resolution for the temperature reading (9 - 12 bits)
#define TEMP_RES 12 

// (P1.7, SW2)
#define IN_INTERRUPT_PORT MXC_GPIO1 
#define IN_INTERRUPT_PIN MXC_GPIO_PIN_7
// (P2.0, LED1)
#define OUT_INTERRUPT_PORT MXC_GPIO2
#define OUT_INTERRUPT_PIN MXC_GPIO_PIN_0


/***** Globals *****/
uint8_t rx_data[DATA_LEN];
uint8_t tx_data[DATA_LEN];
volatile int SPI_FLAG;
volatile uint8_t DMA_FLAG = 0;
mxc_spi_req_t req;

uint8_t temp_MSB; // most significant byte of temperature reading
uint8_t temp_LSB; // least significant byte of temperature reading

/***** Functions *****/
void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);
}

void DMA0_IRQHandler(void)
{
    MXC_DMA_Handler();
}

void DMA1_IRQHandler(void)
{
    MXC_DMA_Handler();
    DMA_FLAG = 1;
}

void SPI_Callback(mxc_spi_req_t *req, int error)
{
    SPI_FLAG = error;
}

void gpio_isr(void *cbdata)
{
    mxc_gpio_cfg_t *cfg = cbdata;
    MXC_GPIO_OutToggle(cfg->port, cfg->mask);

    // read temp MSB register
    tx_data[0] = 0x02;
    tx_data[1] = 0x00;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t));
    MXC_SPI_MasterTransaction(&req);
    temp_MSB = rx_data[1];
    

    // read temp LSB register
    tx_data[0] = 0x01;
    tx_data[1] = 0x00;
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 
    MXC_SPI_MasterTransaction(&req);
    temp_LSB = rx_data[1];

    double temp_final = temp_MSB + temp_LSB/((float)256.0);
    printf("\nFinal Temperature: %.4f\n", temp_final);
}

int main(void)
{
    int retVal;
    mxc_spi_pins_t spi_pins;

    mxc_gpio_cfg_t gpio_interrupt;
    mxc_gpio_cfg_t gpio_interrupt_status;

    printf("\n\n\n*********************** SPI TEMPERATURE READ TEST ********************\n");
    printf("This example configures SPI to get a single temperture reading from\n");
    printf("MAX31723 to MAX78000 when an interrupt is triggered by pressing SW2.\n");
    printf("The interrupt also turns on LED1.\n");

    spi_pins.clock = TRUE;
    spi_pins.miso = TRUE;
    spi_pins.mosi = TRUE;
    spi_pins.sdio2 = FALSE;
    spi_pins.sdio3 = FALSE;
    spi_pins.ss0 = FALSE; // slave select pin 0
    spi_pins.ss1 = TRUE; // P0.11
    spi_pins.ss2 = FALSE;
    spi_pins.vddioh = MXC_GPIO_VSSEL_VDDIOH;

#ifdef BOARD_EVKIT_V1
    printf("\nBoard: BOARD_EVKIT_V1\n");
#else
    printf("\nBoard: MAX78000FTHR\n");
#endif

#if MASTERSYNC
    printf("Performing blocking (synchronous) transactions...\n");
#endif

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
    gpio_interrupt.pad = MXC_GPIO_PAD_PULL_UP;
    gpio_interrupt.func = MXC_GPIO_FUNC_IN;
    gpio_interrupt.vssel = MXC_GPIO_VSSEL_VDDIOH;
    gpio_interrupt.drvstr = MXC_GPIO_DRVSTR_0;
    MXC_GPIO_Config(&gpio_interrupt);
    MXC_GPIO_RegisterCallback(&gpio_interrupt, gpio_isr, &gpio_interrupt_status);
    MXC_GPIO_IntConfig(&gpio_interrupt, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(gpio_interrupt.port, gpio_interrupt.mask);
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(IN_INTERRUPT_PORT)));


    // Configure the peripheral
    // initialize the SPI port
    // master mode
    // not quad mode
    // 1 slave
    // CS of ss1 is active high, 2
    retVal = MXC_SPI_Init(SPI, 1, 0, 1, 2, SPI_SPEED, spi_pins);
    if (retVal != E_NO_ERROR) {
        printf("SPI Initialization ERROR\n");
        return retVal;
    }

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
    req.ssIdx = 1;
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
    // clear the input buffer
    MXC_SPI_ClearRXFIFO	(SPI);


    // Read the configuration register
    printf("\nReading Configuration Register...\n");
    MXC_SPI_MasterTransaction(&req);
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
    MXC_SPI_MasterTransaction(&req);
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
    MXC_SPI_MasterTransaction(&req);
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
    MXC_SPI_MasterTransaction(&req);
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
    MXC_SPI_MasterTransaction(&req);
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

    while(1){ // listen to interrupts}

    return 0;
}

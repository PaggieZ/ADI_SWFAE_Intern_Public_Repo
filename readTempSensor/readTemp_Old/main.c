/**
 * @file    main.c
 * @brief   SPI Master Demo
 * @details Shows Master loopback demo for SPI
 *          Read the printf() for instructions
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "spi.h"
#include "uart.h"

/***** Preprocessors *****/
// master synchronous, CPU waits until operation is done 
#define MASTERSYNC 1 

/***** Definitions *****/
#define DATA_LEN 2
// address for reading the configuration register
#define DATA_VALUE 0x00 
#define DUMMY_BYTE 0xFF
// Bit Rate = 100 kHz, max is 5 MHz
#define SPI_SPEED 100000 

#define SPI_INSTANCE_NUM 1
// use the SPI0 peripheral
#define SPI MXC_SPI0 

// modify spi_pins.ss1 so the board uses chip select 1
#define SS_IDX 1 

/***** Temperature Sensor *****/
// max conversion time is 200ms
// resolution for the temperature reading (9 - 12 bits)
#define TEMP_RES 9 

/***** Globals *****/
uint8_t rx_data[DATA_LEN];
uint8_t tx_data[DATA_LEN];
// variable can change unexpectedly
// must read the memory without optimization
volatile int SPI_FLAG; 
volatile uint8_t DMA_FLAG = 0;

/***** Functions *****/
void SPI_Callback(mxc_spi_req_t *req, int error)
{
    SPI_FLAG = error;
}

int main(void)
{
    int retVal;
    mxc_spi_req_t req;
    mxc_spi_pins_t spi_pins;

    printf("\n**************************** SPI MASTER TEST *************************\n");
    printf("This example configures SPI to read temperature from MAX31723 to MAX78000.\n");

    spi_pins.clock = TRUE;
    spi_pins.miso = TRUE;
    spi_pins.mosi = TRUE;
    spi_pins.sdio2 = FALSE;
    spi_pins.sdio3 = FALSE;
    spi_pins.ss0 = FALSE; // slave select pin 0
    spi_pins.ss1 = TRUE; // P0.11
    spi_pins.ss2 = FALSE;

#ifdef BOARD_EVKIT_V1
    printf("\nBoard: BOARD_EVKIT_V1\n");
#else
    printf("\nBoard: FTHR\n");
#endif

#if MASTERSYNC
    printf("Performing blocking (synchronous) transactions...\n");
#endif

    // initialize the tx buffer with 0x0
    // initialize the rx buffer with 0x0
    memset(tx_data, 0x00, DATA_LEN * sizeof(uint8_t));
    memset(rx_data, 0x00, DATA_LEN * sizeof(uint8_t)); 

    // Setup tx_data to read configuration register
    // TODO: change tx_data based on tasks
    tx_data[0] = 0x00;
    tx_data[1] = DUMMY_BYTE;

    // Configure the peripheral
    
    // initialize the SPI port
    // return the actual clock frequency if success
    // ssPolarity = 0 implies active low
    // the init function enables SPI port
    retVal = MXC_SPI_Init(SPI, 1, 0, 1, 0, SPI_SPEED, spi_pins);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI INITIALIZATION ERROR\n");
        return retVal;
    }

    printf("\nINITIALIZATION SUCCESS\n");


    //SPI Request
    req.spi = SPI;
    // the program is little endian
    // casting a 16-bit data to 8-bit
    // ex: if tx_data = [0x0102]
    //     then, (unit8_t *)tx_data = [0x02, 0x01]
    req.txData = (uint8_t *)tx_data;
    req.rxData = (uint8_t *)rx_data;
    req.txLen = DATA_LEN;
    req.rxLen = DATA_LEN;
    // chip select index
    req.ssIdx = SS_IDX; 
    // Deassert SS at end of transaction
    req.ssDeassert = 1; 
    // number of bytes transmitted from txData (updates during transaction in real time)
    req.txCnt = 0;
    // numof of bytes stored in rxData (updates during transaction in real time)
    req.rxCnt = 0; 
    // pointer to function called when transaction is complete
    req.completeCB = (spi_complete_cb_t)SPI_Callback; 

    // data size is 1 byte
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

    printf("\nRequest READY\n");
#if MASTERSYNC
        // execute the request
        MXC_SPI_MasterTransaction(&req);
#endif

    printf("\nRequest COMPLETE\n");

    printf("Rx_data array:\n");
    for (int i = 0; i < DATA_LEN; i++) {
        printf("rx_data[%d] = %u\n", i, rx_data[i]);
    }

    retVal = MXC_SPI_Shutdown(SPI);

    if (retVal != E_NO_ERROR) {
        printf("\n-->SPI SHUTDOWN ERROR: %d\n", retVal);
        return retVal;
    }

    printf("\nExample Complete.\n");
    return E_NO_ERROR;
}

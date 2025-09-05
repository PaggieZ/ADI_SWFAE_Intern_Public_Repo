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
 * @file        main.c
 * @brief     Example code for scanning the available addresses on an I2C bus
 * @details     This example uses the I2C Master to found addresses of the I2C Slave devices 
 *              connected to the bus. If using EvKit, you must connect the pull-up jumpers
 *              (JP21 and JP22) to the proper I/O voltage.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "i2c.h"

/***** Definitions *****/
#define I2C_MASTER MXC_I2C0 // SDA P0.30, SCL P0.31
#define I2C_FREQ 100000 // 100kHZ

// *****************************************************************************
int main(void)
{
    uint8_t counter = 0;

    printf("\n******** I2C SLAVE ADDRESS SCANNER *********\n");
    printf("\nThis example finds the addresses of any I2C Slave devices connected to the");
    printf("\nsame bus as I2C0 (SDA - P0.30, SCL - P0.31).");
#if defined(EvKit_V1)
    printf("\nSelect the proper voltage for the I2C0 pullup resistors using jumper JP2 ");
    printf("\nand enable them by installing jumpers JP3 and JP4.");
#endif

    int error;

    //Setup the I2CM
    error = MXC_I2C_Init(I2C_MASTER, 1, 0);
    if (error != E_NO_ERROR) {
        printf("-->I2C Master Initialization failed, error:%d\n", error);
        return -1;
    } else {
        printf("\n-->I2C Master Initialization Complete\n");
    }

    printf("-->Scanning started\n");
    MXC_I2C_SetFrequency(I2C_MASTER, I2C_FREQ);
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = I2C_MASTER; // pointer to I2C register
    reqMaster.addr = 0; // slave address (7 or 10 bit)
    reqMaster.tx_buf = NULL; // buffer containing bytes to write
    reqMaster.tx_len = 0;  // number of bytes transmitted
    reqMaster.rx_buf = NULL; // buffer to read the data into
    reqMaster.rx_len = 0; // number of bytes read
    reqMaster.restart = 0; // transaction is terminated with a stop
    reqMaster.callback = NULL;

    for (uint8_t address = 8; address < 120; address++) {
        printf("%03d \n", address);
        fflush(0);

        reqMaster.addr = address;
        // perform a blocking transaction
        if ((MXC_I2C_MasterTransaction(&reqMaster)) == 0) {
            printf("\nFound slave ID %03d; 0x%02X\n", address, address);
            counter++;
        }
        MXC_Delay(MXC_DELAY_MSEC(200));
    }

    printf("\n-->Scan finished. %d devices found\n", counter);

    return 0;
}

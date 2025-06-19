## Description

This code demonstrates how to get temperature readings from a sensor through SPI. The microcontroller is MAX78000FTHR. The temperature sensor is MAX31723.
The program performs blocking SPI transactions. 

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

* This project is based on the SPI sample code form the MSDK, pre-configured for the MAX78000EVKIT.  See [Board Support Packages](https://analogdevicesinc.github.io/msdk/USERGUIDE/#board-support-packages) in the UG for instructions on changing the target board.

## Setup
MAX78000FTHR (FTHR_RevA):
-   Connect a USB cable between the PC and the CN1 (USB/PWR) connector.
-   Open a terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.
-	Connect pins based on the *Connection* section.

## Expected Output
TBA

## Connection
MAX31723    MAX78000FTHR
1 (SS)      P0_11
2 (MOSI)    P0_5
3 (MISO)    P0_6
4 (SCK)     P0_7
5 (GND)     GND
6 (VCC)     3V3

## Useful Links and Development Log
* max78000 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/max78000.pdf
  - pinout
  - found SPI0 slave select pins
    - select 0: P0.4 (can't find p0.4 on board)
    - select 1: P0.11 [x]
    - select 2: P0.10
  - modified spi_pins.ss1 to TRUE

* MAX31723PMB1 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31723PMB1.pdf

* MAX31723 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/max31722-max31723.pdf
  - need to first write to configuration register
  - then read 01 for LSB and 02 for MSB of the temperature

* MSDK User Guide: https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system
  - changed BSP (board support package) to FTHR in .vscode/settings.json
  - looked into MSDK libraries, can't find MAX31723 library

* MAX31723 driver code: https://os.mbed.com/teams/MaximIntegrated/code/MAX31723_Digital_Temperature_Sensor/docs/tip/

## Next Step (6/18/2025)
- find MXC_SPI_MasterTransaction() definition (line 188 in main.c) 
- understand the receiving process (after line 213 in main.c)

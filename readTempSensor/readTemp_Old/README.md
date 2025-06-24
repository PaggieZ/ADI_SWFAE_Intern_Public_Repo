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
| MAX31723  |  MAX78000FTHR |
-----------------------------
| 1 (SS)    |  P0_11 |
| 2 (MOSI)  |  P0_5 |
| 3 (MISO)  |  P0_6 |
| 4 (SCK)   |  P0_7 |
| 5 (GND)   |  GND |
| 6 (VCC)   |  3V3 |

## Useful Links and Development Log
* MAX78000 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/max78000.pdf
  - pinout
  - found SPI0 slave select pins
    - select 0: P0.4 (can't find p0.4 on board)
    - select 1: P0.11 [x]
    - select 2: P0.10
  - modified spi_pins.ss1 in main.c to TRUE

* MAX31723PMB1 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31723PMB1.pdf

* MAX31723 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/max31722-max31723.pdf
  - SPI
    - max SCLK frequency is 5 MHz
    - data is read from or written to the MSB first (big-endian)
    
  - Temperature Reading:
    - use continuous temperature conversion mode (LSB: 01h, MSB: 02h)
    - default temp resolution is 9-bit
    - CE (chip enable) must be inactive for the temp register to be updated
    - 2's complement

  - Configuration Register
    - Read: 00h, write: 80h
    - look at *Table 3* for bit definition
    - can start by reading the configuration register
      - expected power-up state is 0000 0001


* MSDK User Guide: https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system
  - changed BSP (board support package) to FTHR in .vscode/settings.json
  - looked into MSDK libraries, can't find MAX31723 library, can continue without a library

* MAX31723 driver code: https://os.mbed.com/teams/MaximIntegrated/code/MAX31723_Digital_Temperature_Sensor/docs/tip/

* MAX78000 Peripheral Driver API: https://analogdevicesinc.github.io/msdk/Libraries/PeriphDrivers/Documentation/MAX78000/
- SPI_WIDTH_STANDARD

* MAX78000 MSDK SPI Sample Code:
- Enable is active low
- Data is sent from the least significant bit
- Output data is little endian

## Variables
- MAX_SPI_Init() sets the **SPI mode** to 0.

## Questions
- why retVal = 0 after MXC_SPI_Init()? Thought it is supposed to be the actual clock frequency used.
  - 0 = E_NO_ERROR, implies initialization success


## Next Step (6/18/2025)
- find MXC_SPI_MasterTransaction() definition (line 188 in main.c) 
- understand the receiving process (after line 213 in main.c)

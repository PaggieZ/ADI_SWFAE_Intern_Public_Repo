## Description

This code demonstrates how to get temperature readings from a sensor through SPI. The microcontroller is MAX78000FTHR. The temperature sensor is MAX31723. This README file serves as the development log of the read temperature sensor project.

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


## Connection
|Pin    | MAX31723PMB1  |  MAX78000FTHR |
|:-----:|:-------------:|:-------------:|
| SS    | Pin 1         | P0_11 |
| MOSI  | Pin 2         | P0_5 |
| MISO  | Pin 3         | P0_6 |
| SCK   | Pin 4         | P0_7 |  
| GND   | Pin 5         | GND |
| VCC   | Pin 6         | 3V3 |

## Useful Links
* **MAX78000 datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/max78000.pdf
  - pinout
  - found SPI0 slave select pins
    - select 0: P0.4 (can't find p0.4 on board)
    - select 1: P0.11 [x]
    - select 2: P0.10
  - modify spi_pins.ss1 in main.c to TRUE

* **MAX31723PMB1 datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31723PMB1.pdf

* **MAX31723 datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/max31722-max31723.pdf
  - ***SPI***
    - max SCLK frequency is 5 MHz
    - data is read from or written to the MSB first (big-endian)
    - Chip select must be active high
    - inactive clock polarity is not improtant
    - clock phase must be 1, clock polarity could be any
    - data transfer is 1 byte at a time
    
  - ***Temperature Reading***
    - use continuous temperature conversion mode (LSB: 01h, MSB: 02h)
    - default temp resolution is 9-bit
    - CE (chip enable) must be inactive for the temp register to be updated
    - 2's complement

  - ***Configuration Register***
    - Read: 00h, write: 80h
    - look at *Table 3* for bit definition
    - can start by reading the configuration register
      - expected power-up state is 0000 0001


* **MSDK User Guide:** https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system
  - changed BSP (board support package) to FTHR in .vscode/settings.json
  - looked into MSDK libraries, can't find MAX31723 library, can continue without a library

* **MAX31723 driver code:** https://os.mbed.com/teams/MaximIntegrated/code/MAX31723_Digital_Temperature_Sensor/docs/tip/


* **MAX78000 Peripheral Driver API:** https://analogdevicesinc.github.io/msdk/Libraries/PeriphDrivers/Documentation/MAX78000/
  - MAX_SPI_Init() sets the **SPI mode** to 0.

* **MAX78000 MSDK SPI Example Code:** [SPI Example Code from MSDK](../SPI)
  - To set chip select of ss1 to active high, must change the value at the 2nd bit (sspolarity = 2)
  - Data is sent from the least significant bit
  - Output data is little endian

## Milestones
### **Temperature Readings Triggered by Interrupts** (6/27/2025)
  - fixed the unstable reading issue
    - The VDDIO of the MISO pin was 1.8V while all other pins are at 3.3V
  - SW2 triggers a temperature reading through ISR. SW2 also toggles LED1
  - expected output (The last few "Final Temperature" lines are triggered by SW2):
    ```
    *********************** SPI TEMPERATURE READ TEST ********************
    This example configures SPI to get a single temperture reading from
    MAX31723 to MAX78000 when an interrupt is triggered by pressing SW2.
    The interrupt also turns on LED1.

    Board: MAX78000FTHR
    Performing blocking (synchronous) transactions...
    SPI Initialization SUCCESS
    SPI Mode Initialization SUCCESS

    Reading Configuration Register...
    Configuration Register: 0100 0110

    Writing Configuration Register...
    Configuration Register Value Sent: 0100 0110

    Reading Configuration Register...
    Configuration Register: 0110 0110

    Reading Temperature MSB...
    Temperature MSB: 0001 1000
    Temperature MSB: 24 

    Reading Temperature LSB...
    Temperature LSB: 1001 0000
    Temperature LSB: 144 
    Temp_Fraction: 0.562500

    Final Temperature: 24.562500

    Final Temperature: 24.812500

    Final Temperature: 25.312500

    Final Temperature: 25.500000

    Final Temperature: 25.625000

    Final Temperature: 25.687500

    Final Temperature: 25.812500
    ```
### **Single Temperature Reading** (6/24/2025)
  - read from and write to any registers on the temperature sensor
  - expected output:
      ```
      *********************** SPI TEMPERATURE READ TEST ********************
      This example configures SPI to get a single temperture reading from
      MAX31723 to MAX78000.

      Board: MAX78000FTHR
      Performing blocking (synchronous) transactions...
      SPI Initialization SUCCESS
      SPI Mode Initialization SUCCESS

      Reading Configuration Register...
      Configuration Register: 0100 0110

      Writing Configuration Register...
      Configuration Register Value Sent: 0100 0110

      Reading Configuration Register...
      Configuration Register: 0110 0110

      Reading Temperature MSB...
      Temperature MSB: 0001 1000
      Temperature MSB: 24 

      Reading Temperature LSB...
      Temperature LSB: 1011 0000
      Temperature LSB: 176 
      Temp_Fraction: 0.687500

      Final Temperature: 24.687500

      Example Complete.
      ```
    
## Questions
- _Why retVal = 0 after MXC_SPI_Init()? Thought it is supposed to be the actual clock frequency used._
  - 0 = E_NO_ERROR, implies initialization success

## Interesting Topics
### SPI
- Half Duplex and Full Duplex
- Adding more sublines to send multiple bits at a time
- Clock phase and clock polarity
- Synchronous vs. Asynchronous

## Takeaways
* Always make sure the VDDIO on all GPIOs are matched when driving a component

## Next Step
- **6/27/2025**
  - Explore the Asynchronous peripheral API (USE_ASYNC)
  - Modify the code to perform non-blocking transaction
- **6/24/2025**
  - :white_check_mark:learn how to use the push buttons by reading MAX78000FTHR datasheet and the GPIO example code
  - :white_check_mark:implement the interrupt such that when a button is pushed, a temperature reading is displayed on the serial monitor
  - :arrow_down:perform continuous reading until keyboard interrupt
  - :arrow_down:resample when the return value is invalid (0xFF)
- **6/18/2025**
  - :white_check_mark: find MXC_SPI_MasterTransaction() definition (line 188 in main.c)
  - :white_check_mark: understand the receiving process (after line 213 in main.c)

## Description

This code demonstrates how to get temperature readings from a sensor through SPI. The microcontroller is AD-APARD32690-SL. The temperature sensor is MAX31723. This README file serves as the development log of the read temperature sensor project. This project is derived from the previous temperature reading project using a MAX78000FTHR.

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.


## Setup
AD-APARD32690-SL (APARD):
- Connect a USB cable between the PC and the USB/PWR connector.
-	Connect pins based on the *Connection* section. Essentially, connect the temperature sensor to P8 (top row) on the MCU.


## Connection
| Pin   | MAX31723PMB1  | AD-APARD32690-SL |
|:-----:|:-------------:|:----------------:|
| SS    | Pin 1         | SPI4A_SS0 (P1.0) |
| MOSI  | Pin 2         | SPI4A_MOSI (P1.1) |
| MISO  | Pin 3         | SPI4A_MISO (P1.2) |
| SCK   | Pin 4         | SPI4A_SCLK (P1.3) |  
| GND   | Pin 5         | GND |
| VCC   | Pin 6         | VDD_3P3_D (3V3) |

## Useful Links
* **MAX32690 datasheet:** https://www.analog.com/media/en/technical-documentation/data-sheets/max32690.pdf
  - alternate function for setting up SPI4 pins

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
  

* **MAX31723 driver code:** https://os.mbed.com/teams/MaximIntegrated/code/MAX31723_Digital_Temperature_Sensor/docs/tip/



## Milestones
### **RTC Alarm-Interrupted SPI Transaction** (7/10/2025)
  - implemented a 5-second RTC time-of-day alarm ISR to start an asynchronous temperature reading.
    - using similar techniques as in **Timer-Interrupted SPI Transaction** milestone, I added a RTC_SPI_FLAG to trigger the transaction in the RTC request handler
  - can still use SW2 to trigger an temperature reading
  - this marks the completion of the temperture reading project with MSDK :smiley:
  - will move on to CircuitPython :grin:
  - expected output: 
    ```
    *********************** SPI TEMPERATURE READ TEST ********************

    This example configures SPI to get a single temperture reading from
    MAX31723 to AD-APARD32690-SL when an interrupt is triggered by pressing SW2
    or by a 5-second RTC time-of-day alarm. The SW2 interrupt toggles LED1 (blue).
    The RTC interrupt toggles LED2 (green).


    Board: AD-APARD32690-SL
    Performing non-blocking (asynchronous) transactions...
    SPI Initialization SUCCESS
    SPI Mode Initialization SUCCESS

    Reading Configuration Register...
    Configuration Register: 0100 0110

    Writing Configuration Register...
    Configuration Register Value Sent: 0100 0110

    Reading Configuration Register...
    Configuration Register: 0110 0110

    Reading Temperature MSB...
    Temperature MSB: 0001 0111
    Temperature MSB: 23

    Reading Temperature LSB...
    Temperature LSB: 1001 0000
    Temperature LSB: 144
    Temp_Fraction: 0.5625

    Final Temperature: 23.5625

    RTC started
    Current Time (dd:hh:mm:ss): 00:00:00:00.00


    SW2:
    Current Time (dd:hh:mm:ss): 00:00:00:03.77
    Final Temperature: 25.1875


    RTC:
    Current Time (dd:hh:mm:ss): 00:00:00:05.00
    Final Temperature: 25.3125


    SW2:
    Current Time (dd:hh:mm:ss): 00:00:00:05.72
    Final Temperature: 24.8125


    SW2:
    Current Time (dd:hh:mm:ss): 00:00:00:06.40
    Final Temperature: 24.5625

    RTC:
    Current Time (dd:hh:mm:ss): 00:00:00:10.00
    Final Temperature: 26.9375
    ```

### **Timer-Interrupted SPI Transaction** (7/8/2025)
  - implemented a 5-second continuous timer ISR using TMR0 to start an asynchronous temperature reading.
  - referenced the continuous timer implementaion in `TMR` Example Code
    - added a timer initailization function and a timer callback function
  - The continuous timer is enabled by pressing SW2. After the initial press, SW2 is used for GPIO interrupt.
  - I took some notes while reading the MAX32690 datasheet to understand timers and clocks in AD-APARD32690-SL for future project.
    - `AD-APARD32690-SL.pdf` in the folder
    - `timer.pdf` in the folder
  - expected output:
    ```
    *********************** SPI TEMPERATURE READ TEST ********************
    This example configures SPI to get a single temperture reading from
    MAX31723 to AD-APARD32690-SL when an interrupt is triggered by pressing SW2
    or by a 5-second timer. The initial press of SW2 enables the timer.
    Both the timer interrupts and GPIO (SW2) interrupts toggle LED1.

    Board: AD-APARD32690-SL
    Performing non-blocking (asynchronous) transactions...
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
    Temp_Fraction: 0.6875

    Final Temperature: 24.6875


    ****   CONTINUOUS TIMER START   ****


    SW2:
    Final Temperature: 24.5625

    TIMER0:
    Final Temperature: 26.9375

    SW2:
    Final Temperature: 28.0625

    SW2:
    Final Temperature: 28.9375

    TIMER0:
    Final Temperature: 28.3125

    SW2:
    Final Temperature: 28.0625

    TIMER0:
    Final Temperature: 26.1250

    TIMER0:
    Final Temperature: 25.6875

    TIMER0:
    Final Temperature: 25.4375
    ```

### **Asynchronous SPI Transaction** (7/7/2025)
  - implemented asynchronous SPI transaction pattern in the temperature reading project
    - SPI asynchronous transaction note could be found in **SPI.pdf**.
  - expected output (The last few "Final Temperature" sections are triggered by SW2):
    ```
    *********************** SPI TEMPERATURE READ TEST ********************
    This example configures SPI to get a single temperture reading from
    MAX31723 to AD-APARD32690-SL when an interrupt is triggered by pressing SW2.
    The interrupt also toggles on LED1.

    Board: AD-APARD32690-SL
    Performing non-blocking (asynchronous) transactions...
    SPI Initialization SUCCESS
    SPI Mode Initialization SUCCESS

    Reading Configuration Register...
    Configuration Register: 0100 0110

    Writing Configuration Register...
    Configuration Register Value Sent: 0100 0110

    Reading Configuration Register...
    Configuration Register: 0110 0110

    Reading Temperature MSB...
    Temperature MSB: 0001 0111
    Temperature MSB: 23

    Reading Temperature LSB...
    Temperature LSB: 0011 0000
    Temperature LSB: 48
    Temp_Fraction: 0.1875

    Final Temperature: 23.1875


    Reading Temperature MSB...
    Temperature MSB: 0001 0111
    Temperature MSB: 23

    Reading Temperature LSB...
    Temperature LSB: 0011 0000
    Temperature LSB: 48
    Temp_Fraction: 0.1875

    Final Temperature: 23.1875


    Reading Temperature MSB...
    Temperature MSB: 0001 1001
    Temperature MSB: 25

    Reading Temperature LSB...
    Temperature LSB: 0001 0000
    Temperature LSB: 16
    Temp_Fraction: 0.0625

    Final Temperature: 25.0625
    ```


### **Transition from MAX78000FTHR to AD-APARD32690-SL** (7/2/2025)
  - transitioned from MAX78000FTHR to AD-APARD32690-SL and modified the temperature sensor project code to perform single temperature reading and temperature reading by SW2 interrupt on AD-APARD32690-SL
  - major differences and discoveries:
    - _always remember to set the alternate function on the GPIO pins_
      - this fixes the GPIO VDDIO mismatch issue
      - can find the alternate function settings in the MAX32690 datasheet
    - the AD-APARD32690-SL uses [SPI4 (SS0), P2.0 (LED1), P1.7 (SW2) while the MAX78000FTHR uses [SPI0 (SS1), P2.1 (LED1), P2.27 (SW2)]
    - AD-APARD32690-SL requires both `gpio_isr()` and `gpio_callback()`
    - `gpio_interrupt.pad` is `PULL-UP` for MAX78000FTHR but `NONE` for AD-APARD32690-SL
  - expected output (The last few "Final Temperature" lines are triggered by SW2):
    ```
    *********************** SPI TEMPERATURE READ TEST *******************
    This example configures SPI to get a single temperture reading from
    MAX31723 to AD-APARD32690-SL when an interrupt is triggered by pressing SW2.
    The interrupt also toggles on LED1.

    Board: AD-APARD32690-SL
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
    Temperature MSB: 0001 1010
    Temperature MSB: 26

    Reading Temperature LSB...
    Temperature LSB: 0010 0000
    Temperature LSB: 32
    Temp_Fraction: 0.1250

    Final Temperature: 26.1250

    Final Temperature: 26.0625

    Final Temperature: 28.2500

    Final Temperature: 28.9375

    Final Temperature: 29.4375

    Final Temperature: 30.1875

    Final Temperature: 28.9375

    Final Temperature: 28.1250

    Final Temperature: 27.6875

    Final Temperature: 27.4375

    Final Temperature: 27.1250
    ```


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



## Interesting Topics
### SPI
- Half Duplex and Full Duplex
- Adding more sublines to send multiple bits at a time
- Clock phase and clock polarity
- Synchronous vs. Asynchronous

## Takeaways
* Always make sure the VDDIO on all GPIOs are matched when driving a component
* Always make sure to set the alternate function for GPIO pins
* Always set the SPI flag before the while loop for asynchronous transaction
* Perform minimum tasks in ISR
  - use ISR to set a flag to trigger SPI transaction in main()
* Always clear the timer flag in the timer callback function fro exiting the while loop in asynchronous implementation
* Use the corresponsing shell to open vscode (Windows vs. Linux)

## Notes
* Timers and clocks on MAX32690: AD-APARD32690-SL.pdf
* Timers in general: Timer.pdf
* Sync and Async SPI: SPI.pdf

## Next Step
- **7/8/2025**
  - :white_check_mark: use RTC to implement the 5-second timer
- **7/7/2025**
  - :white_check_mark: use timer interrupt to trigger temperature reading
- **6/27/2025**
  - :white_check_mark: Explore the Asynchronous peripheral API (USE_ASYNC)
  - :white_check_mark: Modify the code to perform non-blocking transaction
  - :white_check_mark: replacing MAX78000FTHR with AD-APARD32690-SL
    - :white_check_mark: single temperture reading
    - :white_check_mark: read temperature through on-board switch interrupt
- **6/24/2025**
  - :white_check_mark: learn how to use the push buttons by reading MAX78000FTHR datasheet and the GPIO example code
  - :white_check_mark: implement the interrupt such that when a button is pushed, a temperature reading is displayed on the serial monitor
  - :arrow_down: perform continuous reading until keyboard interrupt
  - :arrow_down: resample when the return value is invalid (0xFF)
- **6/18/2025**
  - :white_check_mark: find MXC_SPI_MasterTransaction() definition (line 188 in main.c)
  - :white_check_mark: understand the receiving process (after line 213 in main.c)

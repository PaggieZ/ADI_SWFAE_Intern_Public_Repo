# This is an example code for reading the status register
# and the temperture registers of a temperature sensor (MAX31723)
# using AD_APARD32690_SL

import board
from busio import SPI

# initialize SPI pins (MOSI, MISO, SCLK)
spi = SPI(board.P1_1, board.P1_2, board.P1_3)

# SW lock for SPI bus
while (spi.try_lock() != True):
    pass

# Configure baud & clock mode
spi.configure(baudrate=1000000, polarity=0, phase=1)

tx = bytes([0x00, 0xFF])
rx = bytearray(2)

# Read/Print MAX31723 status register
spi.write_readinto(tx, rx)
print("MAX31723 STATUS: "+ hex(rx[1]).upper())


tx = bytes([0x02, 0xFF])
rx = bytearray(2)

# Read/Print Temperature MSB
spi.write_readinto(tx, rx)
print("Temperature MSB: "+ hex(rx[1]).upper())


tx = bytes([0x01, 0xFF])
rx = bytearray(2)

# Read/Print Temperature LSB
spi.write_readinto(tx, rx)
print("Temperature LSB: "+ hex(rx[1]).upper())

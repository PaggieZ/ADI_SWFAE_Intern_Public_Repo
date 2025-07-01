# connect the GDB client to the openOCD server
target extended-remote localhost:3333

# serial monitor setup
# direct serial data to COM5
set inferior-tty //./COM5

# check serial port connection
show inferior-tty

# reset the MCU
monitor reset halt

# set breakpoint at main and while
b main
b 48

# show breakpoint information
info breakpoints

# go to main
continue



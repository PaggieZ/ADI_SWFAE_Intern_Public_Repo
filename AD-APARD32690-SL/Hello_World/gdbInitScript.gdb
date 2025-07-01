# connect the GDB client to the openOCD server
target extended-remote localhost:3333

# serial monitor setup
# direct serial data to COM Port
set inferior-tty //./COM9

# check serial port connection
show inferior-tty

# reset the MCU
monitor reset halt

# set breakpoint at main and while
b main
b 49

# show breakpoint information
info breakpoints

# go to main
continue



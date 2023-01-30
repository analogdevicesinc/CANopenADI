# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://github.com/Analog-Devices-MSDK/VSCode-Maxim/tree/develop#build-configuration

# **********************************************************

# Add your config here!

# If you have secure version of MCU, set SBT=1 to generate signed binary
# For more information on how sing process works, see
# https://www.analog.com/en/education/education-library/videos/6313214207112.html
SBT=0

# Add CANopenNode source files into build
VPATH += ../../CANopenNode
VPATH += ../../CANopenNode/301
VPATH += ../../CANopenNode/303
VPATH += ../../CANopenNode/304
VPATH += ../../CANopenNode/305
VPATH += ../../CANopenNode/309
VPATH += ../../CANopenNode/extra
VPATH += ../../CANopenNode/storage
# Add MAX32xxx driver
VPATH += ../../MAX32xxx 

# Add CANopenNode to include path
IPATH += ../../CANopenNode
IPATH += ../../MAX32xxx

# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://github.com/Analog-Devices-MSDK/VSCode-Maxim/tree/develop#build-configuration

# **********************************************************

# Add your config here!

# Add CANopenNode source files into build
VPATH += CANopenNode/301
VPATH += CANopenNode/303
VPATH += CANopenNode/304
VPATH += CANopenNode/305
VPATH += CANopenNode/309
VPATH += CANopenNode/extra
VPATH += CANopenNode/storage
VPATH += CANopenNode
# Add CANopenNode to include path
IPATH += CANopenNode
# Add CANopenNode_MAX32XXX source and includes
VPATH += CANopenNode_MAX32XXX 
IPATH += CANopenNode_MAX32XXX

# Build default example
VPATH += examples/default
IPATH += examples/default
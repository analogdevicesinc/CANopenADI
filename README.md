# CANopenNode Analog Devices Inc.

CANopenADI is a CANopen stack running on Analog Devices Inc. MAX32662 and MAX32690 microcontroller based on [CANOpenNode](https://github.com/CANopenNode/CANopenNode) stack.

## How to run demos

In order to run the demos, you will need:

- Analog Devices MSDK: If you have not installed it already, see this [guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/#installation) for detailed instructions.

- A DAPLink debug adapter for flashing and debugging: [MAX32625PICO](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/max32625pico.html)

#### Cloning the project

Clone the project from git repository and get submodules:

```
git clone --recursive https://github.com/CANopenNode/CANopenADI
cd CANopenADI
```

Update an existing project including submodules:

```
cd CANopenADI
git pull
git submodule update --init --recursive
```

#### Importing projects

You can import example projects into Eclipse MaximSDK.

1. Click **File -> Import...**
2. Select **General -> Existing Projects into Workspace**
3. Enter the CANopenADI repository path or click browse and navigate to CANopenADI then hit **Select Folder**.
4. Make sure **Search nested projects** is enabled otherwise the example projects will not be listed.
5. Example projects should now be visible inside the **Projects** box. Check the ones you wish to import then click **Finish**.
6. The projects should now be added to your workspace and visible in the **Project Explorer** tab.

See also this [guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/#importing-examples) for more information about setting up projects in Eclipse MaximSDK.

#### Building and running

1. Select the example project in the **Launch Configuration** list.
2. Click the hammer icon on the left to build the project.
3. Select **Launch Mode**, which is either **Debug** or **Run**.
4. Use the **Debug**/**Run** button to upload the binary and launch the application.

See also this [guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/#building-and-running-examples) for more information about building and running.

Example projects mostly differ in their **Object Dictionary** configurations. Use [CANopenEditor](https://github.com/CANopenNode/CANopenEditor) to configure the object dictionary profiles according to the needs of your project.

## Repository directories

- `.\CANopenNode` : Includes the stack implemenation. In most scenarios, you do not need to edit these files as they are platform independent (i.e. ADI, Linux, PIC, STM32 and etc.).
- `.\MAX32xxx` : Includes the implementation of low-level driver for MAX32662 and MAX32690 microcontrollers.
- `.\examples_MAX32662` : Contains examples targeting MAX32662 boards.
- `.\examples_MAX32690` : Contains examples targeting MAX32690 boards.

## Supported boards and MCUs
 

### [MAX32690-EvKit](https://www.analog.com/).

Evaluation kit for the MAX32690 microcontroller. A CAN transceiver is already available on the board. The data lines are exposed at JH8, labelled CAN0. You can connect CANH and CANL lines to an existing network or another board's CAN port.


### [MAX32662-EvKit](https://www.analog.com/)

Evaluation kit for the MAX32662 microcontroller. A CAN transceiver is already available on the board. The data lines are exposed at JH3, labelled CANB. You can connect CANH and CANL lines to an existing network or another board's CAN port.


### Known limitations : 

There are only two hardware acceptance filters on MAX32662 and MAX32690 which is less than what CANopen usually requires. 
As a consequence every message on the bus has to be evaluated by the CPU before being discarded or passed to the upper 
layers for further processing.

The decision to use software filters is made in `CO_CANmodule_init`.

```c
    CANmodule->useCANrxFilters = false;
```
`CO_CANrxBufferInit` is invoked for each CANopen receive buffer to set the identifier and mask for acceptance filters.

```c
    /* CAN identifier and CAN mask, bit aligned with CAN module. Different on different microcontrollers. */
	buffer->ident = MXC_CAN_STANDARD_ID(ident);
	if(rtr){
		buffer->ident |= MXC_CAN_BUF_CFG_RTR(1);
	}
	buffer->mask = MXC_CAN_STANDARD_ID(mask) | MXC_CAN_BUF_CFG_RTR(1);
```

The received messages are then checked against previously set filters, in function `CO_CANRXinterrupt`.

```c
    else{
        /* CAN module filters are not used, message with any standard 11-bit identifier */
        /* has been received. Search rxArray from CANmodule for the same CAN-ID. */
        buffer = &CANmodule->rxArray[0];
        for(index = CANmodule->rxSize; index > 0U; index--){
            if(((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U){
                msgMatched = true;
                break;
            }
            buffer++;
        }
    }
```

## License

This file is part of CANopenNode, an opensource CANopen Stack. Project home page is https://github.com/CANopenNode/CANopenNode. For more information on CANopen see http://www.can-cia.org/.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

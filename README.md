# CANopenNode Analog Devices Inc.

CANopenADI is a CANopen stack running on Analog Devices Inc. MAX32662 and MAX32690 microcontroller based on [CANOpenNode](https://github.com/CANopenNode/CANopenNode) stack.

## How to run demos

TODO:

## Repository directories

TODO:
- `.\CANopenNode` : Includes the stack implemenation, for most of usecases you don't need to touch these files as they are constant between all the variations and ports (i.e. ADI, Linux, PIC, STM32 and etc.)
- `.\CANopenNodeADI` : Includes the implementation of low-level driver for MAX32662 and MAX32690 microcontrollers.

## Supported boards and MCUs
 

### [MAX32690-EvKit](https://www.analog.com/).

TODO



### [MAX32662-EvKit](https://www.analog.com/)

TODO


### Known limitations : 

TODO

### Clone or update

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

## License

This file is part of CANopenNode, an opensource CANopen Stack. Project home page is https://github.com/CANopenNode/CANopenNode. For more information on CANopen see http://www.can-cia.org/.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

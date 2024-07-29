# Build

## Dependencies

To build this project, you'll need:

- IDF tool : [ESP-IDF tool](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation)
- The Python 3 modules `esptool`, `intelhex`, (see [requirement.txt](../firmware_bin/requirement.txt))
  - To keep the system clean, you can install python modules into a python virtual environment (`venv`)
    ```sh 
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install wheel
    python -m pip install -r requirements.txt
    ```

## Build steps

### Clone the repo
---
You can clone the repo in any folder you want, open the cmd/bash/shell and go the required directory and run below commands 
```sh
git clone https://github.com/StimVeda-Neurosciences/Ease_BLECode 
git submodule update --init 
```



### Project generation using IDF Tool

IDF  configures the project using Kconfig and build using CMake and Ninja :

 commnd  | Description | IDF command |
----------|-------------|--------|
**menu configuration**|Configure the current project  |`idf.py menuconfig `|
**build the application**|start building the aaplication |`idfpy build `|
***clean***.|clean the current Build  | `idf.py clean `
**set the target chip**| you can use show target to show the chips supported | `idf.py set-target esp32`
**flash**| Flash the binaries into the esp32 (flash the bootloader,partition_table, application and serial_number ) |`idf.py falsh -p COMPORT`
**monitor the output**|you can see the output on a serial terminal in idf tool. it will be shown as ansi colored serial output |`idf.py monitor``
---

<br>


List of files generated:
Binary files are generated into the folder  Ease_firmware/build:

- **EASE.bin, .hex and .out** : standalone firmware in bin, hex and out formats.
- **EASE.map** : map file
- **partition_table.bin, .hex and .out** : firmware with partition_file data  in bin, hex and out formats.
- **bootloader.bin, .hex and .out** : firmware with esp32 bootloader support  



### How to upload Firmware
--- 
For flashing firmware in the device you can refer to  [`flashing`](flashing.md).  

---
``` 
 Serial_number.bin
This file is genrated by the Python program that reads the input from the user input and genrate this file to be flashed at this address `0xd000` the main program will find the device information like the serial number, Hardware version and Firmware version here.
```
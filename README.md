# HAL layer for STM32F407 dicovery board

HAL layer can be used as reference when creating other stm32F4 HAL layers. Based on STM's HAL layer.

Written in "System workbench for STM32".

## Command interface 

| Command     | interface     |
|-------------|---------------|
|  USART3 TX  |      PA2      |
|  USART3 RX  |      PA3      |
| Baud rate   | 115200 bits/s |
| Word length | 8             |
| Stop bit    | 1             |
| Parity      | None          |

## Debug interface 

Debug interface is enabled with semihosting through ST-Link integrated on the board. 
If user wants to enable debug output, he/she should build and flash the program with "CBL Debug.cfg". 
On the other hand, if user doesn't need the debug output, build and flash the program with "CBL Release.cfg".

**NOTE:** If program is flashed using "CBL Debug.cfg" and ST-Link losses connection with the "System workbench for STM32" STM32 application will not work. Because of that use "CBL Debug.cfg" only when debugging. 

## Boot record

Used to store metadata about active application (application currently running) and new application (application to be updated to).
Defined in custom_bootloader/Inc/etc/cbl_boot_record. Boot record is located in SEC3, always on location 0x0800C000, it is not initalized on system startup.

**NOTE:** When using CRC32 input data length must be divisible by 4! If your input is not divisible by 4 then append needed number of 0xFF on the end, before the checksum.

|       CRC32       |       settings       |
|:-----------------:|:--------------------:|
| Polynomial length |          32          |
|     Polynomial    | 0x4C11DB7 (Ethernet) |
|     Init value    |      0xFFFFFFFF      |
|       XOROut      |         true         |
|       RefOut      |         true         |
|       RefIn       |         true         |

# Custom bootloader with shell interface

Project is writen using "System workbench for STM32" based on Eclipse.

Branches:
<pre>
master:             Custom bootloader implementation without HAL.
hal_ti_tms570lc43x: HAL layer for Texas instruments TMS570LC43x. 
user_app_stm32f407: Needed user application modifications to work with the bootloader.
hal_stm32_f4disc1:  HAL layer for stm3232f407 discovery board.
</pre>
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

## STM32F4xx user application template (branch user_app)

This template contains files that need to be configured in user application when using the bootloader. 

Files were generated with STM32Cube-MX. Changes made:
- system_stm32f4xx.c

   - added #if 0 guard around vector table location, let bootloader deal with that
   
- STM32F407VGTx_FLASH.ld

   - added memory section SEC3, representing sector 3
   - SEC3 shall be used as persistent memory location 
   - for application boot record
   - and for communication between user application and bootloader

## Boot record

Used to store metadata about active application (application currently running) and new application (application to be updated to).
Defined in custom_bootloader/Inc/etc/cbl_boot_record. Boot record is located in SEC3, always on location 0x0800C000, it is not initalized on system startup.

## Command reference

**NOTE:**

 - Every execute of a command must end with **"\r\n"**

 - Commands are case insensitive
 
 - On error bootloader returns "ERROR:\<Explanation of error\>"
 
 - Optional parameters are surrounded with [] e.g. [example]

### List
* [version](#cmd_version) : Gets a version of the bootloader
* [help](#cmd_help) : Makes life easier
* [reset](#cmd_reset) : Resets the microcontroller
* [cid](#cmd_cid) : Gets chip identification number
* [get-rdp-level](#cmd_get-rdp-level) : Gets read protection Ref. man. p. 93
* [jump-to](#cmd_jump-to) : Jumps to a requested address
* [flash-erase](#cmd_flash-erase) : Erases flash memory
* [flash-write](#cmd_flash-write) : Writes to flash
* [mem-read](#cmd_mem-read) : Read bytes from memory
* [update-act](#cmd_update-act) : Updates active application from new application memory area
* [update-new](#cmd_update-new) : Updates new application
* [en-write-prot](#cmd_en-write-prot) : Enables write protection per sector
* [dis-write-prot](#cmd_dis-write-prot) : Disables write protection per sector
* [get-write-prot](#cmd_get-write-prot) : Returns bit array of sector write protection
* [exit](#cmd_exit) : Exits the bootloader and starts the user application

### More about
<a name="cmd_version"></a>
#### [version](#cmd_version)—Gets a version of the bootloader

Parameters:

 - None

Execute command: 

    > version  
Response: 

    v1.0  

<a name="cmd_help"></a>
####  [help](#cmd_help)—Makes life easier
Parameters:

 -  None

Execute command: 

    > help  
Response: 

    <List of all commands and examples>
    
<a name="cmd_reset"></a>
#### [reset](#cmd_reset)—Resets the microcontroller
Parameters:

 - None

Execute command: 

    > reset
    
Response: 

    OK

<a name="cmd_cid"></a>
####  [cid](#cmd_cid)—Gets chip identification number
Parameters:

 - None

Execute command: 

    > cid  
    
Response: 

    0x413

<a name="cmd_get-rdp-level"></a>
####  [get-rdp-level](#cmd_get-rdp-level)—Gets read protection, used to protect the software code stored in Flash memory. Ref. man. p. 93
Parameters:

  - None

Execute command: 

    > get-rdp-level  
Response: 

    level 0

<a name="cmd_jump-to"></a>
####  [jump-to](#cmd_jump-to)—Jumps to a requested address
Parameters:

- addr - Address to jump to in hex format (e.g. 0x12345678), 0x can be omitted

Execute command: 

    > jump-to addr=0x87654321  
Response: 

    OK
     
<a name="cmd_flash-erase"></a>
####  [flash-erase](#cmd_flash-erase)—Erases flash memory
Parameters:

- type - Defines type of flash erase. "mass" erases all sectors, "sector" erases only selected sectors
    
- sector - First sector to erase. Bootloader is on sectors 0, 1 and 2. Not needed with mass erase
    
- count - Number of sectors to erase. Not needed with mass erase

Execute command: 

    > flash-erase sector=3 type=sector count=4  
Response: 

    OK

   
<a name="cmd_flash-write"> </a>
####  [flash-write](#cmd_flash-write)—Writes to flash byte by byte. Splits data into chunks

Parameters:

 - start - Starting address in hex format (e.g. 0x12345678), 0x can be omitted
     
 - count - Number of bytes to write, without checksum. Chunk size: 5120
 
 - [cksum] - Defines the checksum to use. If not present no checksum is assumed. WARNING: Even if checksum is wrong data will be written into flash memory!
 
      - "sha256" - Gives best protection (32 bytes), slowest, uses software implementation
           
      - "crc32" - Medium protection (4 bytes), fast, uses hardware implementation. Settings in [Apendix A](#apend_a)

      - "no" - No protection, fastest

Note:

  When using crc-32 checksum sent data has to be divisible by 4

Execute command: 

    > flash-write start=0x87654321 count=64 cksum=crc32  
    
Response: 

    chunks:1

    chunk:0|length:64|address:0x87654321

    ready
    
Send bytes:

    <64 bytes>
    
Response:

    chunk OK

    checksum|length:4

    ready
 
Send checksum:
     
     <4 bytes>
     
Response:
 
    OK
    
<a name="cmd_dis-write-prot"></a>
####  [dis-write-prot](#cmd_dis-write-prot)—Disables write protection per sector, as selected with "mask"
Parameters:

- mask - Mask in hex form for sectors where LSB corresponds to sector 0

Execute command: 

    > dis-write-prot mask=0xFF0
Response: 

    OK


<a name="cmd_mem-read"></a>
####  [mem-read](#cmd_mem-read)—Read bytes from memory
Parameters:

- start - Starting address in hex format (e.g. 0x12345678), 0x can be omitted
     
- count - Number of bytes to read

Execute command: 

    > mem-read start=0x87654321 count=3  
Response: 

    <3 bytes starting from the address 0x87654321>
    
Note:
- Entering invalid read address crashes the program and reboot is required. 
    
<a name="cmd_update-act"></a>
#### [update-act](#cmd_update-act)—Updates active application from new application memory area
Parameters:
- [force] - Forces update even if not needed

   - "true" - Force the update
                
   - "false" - Don't force the update


Execute command: 

    > update-act force=true
Response: 

    No update needed for user application
    Updating user application
    OK
    
<a name="cmd_update-new"></a>
#### [update-new](#cmd_update-new)—Updates new application
Parameters:

 - count - Number of bytes to write, without checksum. Chunk size: 5120
 
 - type - Type of application coding
       
      - "bin" - Binary format (.bin)
                
      - "hex" - Intel hex format (.hex)
      
      - "srec" - Motorola S-record format (.srec)
 
 - [cksum] - Defines the checksum to use. If not present no checksum is assumed. WARNING: Even if checksum is wrong data will be written into flash memory!
 
      - "sha256" - Gives best protection (32 bytes), slowest, uses software implementation
           
      - "crc32" - Medium protection (4 bytes), fast, uses hardware implementation. Settings in [Apendix A](#apend_a)

      - "no" - No protection, fastest


Execute command: 

    > update-new count=4 type=bin cksum=sha256
Response: 

    chunks:1

    chunk:0|length:4|address:0x08080000

    ready
    
Send bytes:

    <4 bytes>
    
Response:

    chunk OK

    checksum|length:32

    ready
 
Send checksum:
     
     <32 bytes>
     
Response:
 
    OK
    
<a name="cmd_dis-write-prot"></a>
####  [dis-write-prot](#cmd_dis-write-prot)—Disables write protection per sector, as selected with "mask"
Parameters:

- mask - Mask in hex form for sectors where LSB corresponds to sector 0

Execute command: 

    > dis-write-prot mask=0xFF0
Response: 

    OK

    
<a name="cmd_en-write-prot"></a>
####  [en-write-prot](#cmd_en-write-prot)—Enables write protection per sector, as selected with "mask"
Parameters:

- mask - Mask in hex form for sectors where LSB corresponds to sector 0

Execute command: 

    > en-write-prot mask=0xFF0
    
Response: 

    OK

<a name="cmd_get-write-prot"></a>
####  [get-write-prot](#cmd_get-write-prot)—Returns bit array of sector write protection. LSB corresponds to sector 0
Parameters:

- None

Execute command: 

    > get-write-prot
Response: 

    0b100000000010
  
<a name="cmd_exit"></a>
####  [exit](#cmd_exit)—Exits the bootloader and starts the user application
Parameters:

- None

Execute command: 

    > exit  
Response: 

    Exiting

<a name="apend_a"></a>
## [Apendix A](#apend_a)

**NOTE:** When using CRC32 input data length must be divisible by 4! If your input is not divisible by 4 then append needed number of 0xFF on the end, before the checksum.

|       CRC32       |       settings       |
|:-----------------:|:--------------------:|
| Polynomial length |          32          |
|     Polynomial    | 0x4C11DB7 (Ethernet) |
|     Init value    |      0xFFFFFFFF      |
|       XOROut      |         true         |
|       RefOut      |         true         |
|       RefIn       |         true         |

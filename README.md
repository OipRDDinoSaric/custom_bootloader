# STM32F4 Discovery board custom bootloader with shell interface

This project is a bootloader for STM32F4 Disc1 board with STM32F407VG. It is also easily transferable to other STM32F4 platforms. 

Project is writen using "System workbench for STM32" based on Eclipse.

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

## Command reference

**NOTE:**

 - Every execute of a command must end with **"\r\n"**

 - Commands are case insensitive
 
 - On error bootloader returns "ERROR:\<Explanation of error\>"
 
 - Optional parameters are surrounded with [] e.g. [example]

### List
* [version](#cmd_version) : Gets a version of the bootloader
* [help](#cmd_help) : Makes life easier
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

    > flash-write start=0x87654321 count=68 cksum=crc32  
Response: 

    ready

Send bytes:

    <64 data bytes + 4 bytes CRC calculation>
    
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
    
<a name="cmd_update-act"></a>
### [update-act](#cmd_update-act) : Updates active application from new application memory area
Parameters:
- [force] - Forces update even if not needed
                "true" - Force the updateslowest
                "true" - Don't force the update


Execute command: 

    > update-act force=true
Response: 

    No update needed for user application
    Updating user application
    OK
    
<a name="cmd_update-new"></a>
### [update-new](#cmd_update-new) : Updates new application
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

    > update-act force=true
Response: 

    No update needed for user application
    Updating user application
    OK
    
<a name="cmd_en-write-prot"></a>
####  [en-write-prot](#cmd_en-write-prot)—Enables write protection per sector, as selected with "mask"
Parameters:

- mask - Mask in hex form for sectors where LSB corresponds to sector 0

Execute command: 

    > en-write-prot mask=0xFF0
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

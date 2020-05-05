# STM32F4xx user application template

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

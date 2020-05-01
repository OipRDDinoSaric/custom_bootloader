/** @file cbl_cmds_memory.h
 *
 * @brief Contains functions for memory access from the bootloader
 */
#ifndef CBL_CMDS_MEMORY_H
#define CBL_CMDS_MEMORY_H

#include "cbl_common.h"
#include "cbl_checksum.h"

#define TXT_FLASH_WRITE_SZ "5120" /*!< Size of a buffer used to write to flash
                                  as char array */
#define FLASH_WRITE_SZ 5120 /*!< Size of a buffer used to write to flash */
/* NOTE: Flash write size shall be divisible by 4 if CRC32 checksum is used*/

#define TXT_CMD_JUMP_TO "jump-to"
#define TXT_CMD_FLASH_ERASE "flash-erase"
#define TXT_CMD_FLASH_WRITE "flash-write"
#define TXT_CMD_MEM_READ "mem-read"

#define TXT_PAR_JUMP_TO_ADDR "addr"

#define TXT_PAR_FLASH_WRITE_START "start"
#define TXT_PAR_FLASH_WRITE_COUNT "count"


#define TXT_PAR_FLASH_ERASE_TYPE "type"
#define TXT_PAR_FLASH_ERASE_SECT "sector"
#define TXT_PAR_FLASH_ERASE_COUNT "count"
#define TXT_PAR_FLASH_ERASE_TYPE_MASS "mass"
#define TXT_PAR_FLASH_ERASE_TYPE_SECT "sector"

cbl_err_code_t cmd_jump_to (parser_t * phPrsr);
cbl_err_code_t cmd_flash_erase (parser_t * phPrsr);
cbl_err_code_t cmd_flash_write (parser_t * phPrsr);
cbl_err_code_t cmd_mem_read (parser_t * phPrsr);
cbl_err_code_t flash_write (uint32_t start, uint32_t len, cksum_t cksum);
#endif /* CBL_CMDS_MEMORY_H */
/*** end of file ***/

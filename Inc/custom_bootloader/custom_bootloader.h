/**
 * @file custom_bootloader.h
 *
 * @brief   Custom bootloader for STM32F4 Disc1 development board with
 *          STM32F407. Uses UART for communcation
 *
 * @note    Written according to BARR-C:2018 coding standard
 *
 */
#ifndef __CBL_H
#define __CBL_H

#include <stdlib.h>

#define CBL_VERSION "v1.1"

#define pUARTCmd &huart2 /*!< UART used for shell communication */

#define CBL_ADDR_USERAPP 0x08010000UL /*!< Address to MSP of user application */

typedef enum
{
    CBL_ERR_OK = 0, /*!< No error, we gucci */
    CBL_ERR_READ_OF, /*!< Buffer overflowed while reading */
    CBL_ERR_WRITE, /*!< Error while writing */
    CBL_ERR_STATE, /*!< Unexpected state requested */
    CBL_ERR_HAL_TX, /*!< Error happened in HAL library while transmitting */
    CBL_ERR_HAL_RX, /*!< Error happened in HAL library while receiving */
    CBL_ERR_RX_ABORT, /*!< Error happened while aborting receive */
    CBL_ERR_CMD_SHORT, /*!< Received command is of length 0 */
    CBL_ERR_CMD_UNDEF, /*!< Received command is invalid */
    CBL_ERR_CMDCD, /*!< Invalid command code enumerator */
    CBL_ERR_NEED_PARAM, /*!< Called command is missing a parameter */
    CBL_ERR_JUMP_INV_ADDR, /*!< Given address is not jumpable */
    CBL_ERR_HAL_ERASE, /*!< HAL error happened while erasing */
    CBL_ERR_SECTOR, /*!< Error happened while erasing sector */
    CBL_ERR_INV_SECT, /*!< Wrong sector number given */
    CBL_ERR_INV_SECT_COUNT, /*!< Wrong sector count given */
    CBL_ERR_WRITE_INV_ADDR, /*!< Given address can't be written to */
    CBL_ERR_INV_SZ, /*!< Entered size 0 or too long */
    CBL_ERR_HAL_WRITE, /*!< Error on HAL level while writing to flash */
    CBL_ERR_ERASE_INV_TYPE, /*!< Erase command has wrong erase type param */
    CBL_ERR_RWP_INV_TYPE, /*!< Invalid type in enable rw protec. */
    CBL_ERR_HAL_UNLOCK, /*!< Unlocking with HAL failed */
    CBL_ERR_INV_PARAM, /*!< Invalid function parameter */
    CBL_ERR_NOT_DIG, /*!< String contains non digit characters  */
    CBL_ERR_UNSUP_BASE, /*!< Unsupported number base */
    CBL_ERR_1ST_NOT_ZERO, /*!< First char must be '0' */
    CBL_ERR_CKSUM_WRONG, /*!< Checksum for received bytes is wrong */
    CBL_ERR_TEMP_NOT_VAL1, /*!< Explanation of error. DON'T FORGET HANDLER */
    CBL_ERR_UNSUP_CKSUM, /*!< Checksum not supported for received data */
    CBL_ERR_CRC_LEN, /*!< Invalid length for CRC check */
    CBL_ERR_SHA256_LEN /*!< Invalid length for sha256 */
} cbl_err_code_t;

void CBL_run_system (void);
cbl_err_code_t CBL_process_cmd (char * cmd, size_t len);

#endif /* __CBL_H */
/****END OF FILE****/

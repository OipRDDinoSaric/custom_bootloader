/** @file cbl_boot_record.c
 *
 * @brief Boot record hold useful data about current version of user application
 *        and of a new one, if it is available
 *
 * @note This file is part of custom bootloader, but is also included in the
 *       user application
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cbl_cmds_memory.h"
#include "cbl_boot_record.h"

static volatile cbl_record_t cbl_record __attribute__((section(".appbr")));

/**
 * @brief Gets a reference of boot record
 *
 * @return Pointer to boot record
 */
volatile cbl_record_t * cbl_boot_record_get (void)
{
    return &cbl_record;
}

/**
 * @brief Sets the boot record value
 *
 * @param new_bl_record Value to be written
 */
cbl_err_code_t cbl_boot_record_set (cbl_record_t * p_new_cbl_record)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t len = (uint32_t)sizeof(cbl_record_t);
    uint8_t *p_new_byte = (uint8_t *)p_new_cbl_record;

    eCode = flash_erase_sector(BOOT_RECORD_SECTOR, BOOT_RECORD_MAX_SECTORS);
    ERR_CHECK(eCode);

    eCode = write_program_bytes(BOOT_RECORD_START, p_new_byte, len);
    return eCode;
}
/*** end of file ***/

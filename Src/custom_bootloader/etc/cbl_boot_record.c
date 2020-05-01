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

#define GOOD_KEY 0x12345678

static volatile boot_record_t boot_record __attribute__((section(".appbr")));
static boot_record_t boot_record_editable;

static void boot_record_init (boot_record_t * p_boot_record);
/**
 * @brief Gets a editable copy of boot record
 *
 * @return Pointer to editable boot record if it has been initialized, else NULL
 */
boot_record_t * boot_record_get (void)
{
    if (boot_record.key == GOOD_KEY)
    {
        memcpy( &boot_record_editable, (void *) &boot_record,
                sizeof(boot_record));
    }
    else
    {
        boot_record_init( &boot_record_editable);
        boot_record_set( &boot_record_editable);
    }

    return &boot_record_editable;
}

/**
 * @brief Sets the boot record value
 *
 * @param new_bl_record Value to be written
 */
cbl_err_code_t boot_record_set (boot_record_t * p_new_boot_record)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t len = (uint32_t)sizeof(boot_record_t);
    uint8_t *p_new_byte = (uint8_t *)p_new_boot_record;

    p_new_boot_record->key = 0x12345678;

    eCode = hal_flash_erase_sector(BOOT_RECORD_SECTOR, BOOT_RECORD_MAX_SECTORS);
    ERR_CHECK(eCode);

    eCode = hal_write_program_bytes(BOOT_RECORD_START, p_new_byte, len);
    return eCode;
}

/**
 * @brief Writes correct application type to p_app_type
 *
 * @param char_app_type[in] String of application type
 * @param len[in]           Length of 'char_app_type'
 * @param p_app_type[out]   Enumerator of application type
 */
cbl_err_code_t enum_app_type (char *char_app_type, uint32_t len,
        app_type_t * p_app_type)
{
    if (char_app_type == NULL)
    {
        *p_app_type = TYPE_UNDEF;
        return CBL_ERR_NULL_PAR;
    }

    if (strlen(TXT_PAR_APP_TYPE_BIN) == len
            && strncmp(char_app_type, TXT_PAR_APP_TYPE_BIN, len) == 0)
    {
        *p_app_type = TYPE_BIN;
    }
    else if (strlen(TXT_PAR_APP_TYPE_HEX) == len
            && strncmp(char_app_type, TXT_PAR_APP_TYPE_HEX, len) == 0)
    {
        *p_app_type = TYPE_HEX;
    }
    else if (strlen(TXT_PAR_APP_TYPE_SREC) == len
            && strncmp(char_app_type, TXT_PAR_APP_TYPE_SREC, len) == 0)
    {
        *p_app_type = TYPE_SREC;
    }
    else
    {
        *p_app_type = TYPE_UNDEF;
        return CBL_ERR_APP_TYPE;
    }

    return CBL_ERR_OK;
}

/**
 * @brief Resets the boot_record reference
 * @param p_boot_record Boot record to initialize
 */
static void boot_record_init (boot_record_t * p_boot_record)
{
    p_boot_record->act_app.app_type = TYPE_UNDEF;
    p_boot_record->act_app.cksum_used = CKSUM_UNDEF;
    p_boot_record->act_app.len = 0;

    p_boot_record->new_app.app_type = TYPE_UNDEF;
    p_boot_record->new_app.cksum_used = CKSUM_UNDEF;
    p_boot_record->new_app.len = 0;

    p_boot_record->key = GOOD_KEY;
    p_boot_record->is_new_app_ready = false;
}

/*** end of file ***/

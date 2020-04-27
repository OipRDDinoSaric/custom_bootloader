/** @file cbl_cmds_update_act.c
 *
 * @brief Adds command for updating bytes for active application, writes to boot
 *        record
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cbl_boot_record.h"
#include "cbl_cmds_memory.h"
#include "cbl_cmds_update_act.h"

static cbl_err_code_t update_act (app_type_t app_type, uint32_t new_len);
static cbl_err_code_t update_act_bin (uint32_t new_len);
static cbl_err_code_t update_act_hex (uint32_t new_len);
static cbl_err_code_t update_act_srec (uint32_t new_len);
/**
 * @brief Checks 'boot record' if update to user application is available.
 *        If it is available updates the user application.
 *
 * @param phPrsr Not used
 */
cbl_err_code_t cmd_update_act (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    boot_record_t * p_boot_record;
    uint32_t new_len;

    p_boot_record = boot_record_get();
    new_len = p_boot_record->new_app.len;

    if (p_boot_record->is_new_app_ready == false)
    {
        /* Notify that no update is required */
        const char *msg = "No update needed for user application\r\n";
        INFO("%s", msg);
        send_to_host(msg, strlen(msg));
        return CBL_ERR_OK;
    }

    /* Notify that no update is required */
    const char *msg = "Update for user application available\r\n"
            "Updating user application\r\n";
    INFO("%s", msg);
    eCode = send_to_host(msg, strlen(msg));
    ERR_CHECK(eCode);

    /* Remove the flag signalizing update */
    p_boot_record->is_new_app_ready = false;

    if (new_len > BOOT_ACT_APP_MAX_LEN)
    {
        /* New application is too long to fit into flash */
        return CBL_ERR_NEW_APP_LEN;
    }

    /* Write bytes to active application location */
    eCode = update_act(p_boot_record->new_app.app_type, new_len);
    ERR_CHECK(eCode);

    /* Update active application meta data */
    p_boot_record->act_app.app_type = p_boot_record->new_app.app_type;
    p_boot_record->act_app.cksum_used = p_boot_record->new_app.cksum_used;
    p_boot_record->act_app.len = p_boot_record->new_app.len;

    /* Update new application meta data */
    p_boot_record->act_app.app_type = TYPE_UNDEF;
    p_boot_record->act_app.cksum_used = CKSUM_UNDEF;
    p_boot_record->act_app.len = 0;

    eCode = boot_record_set(p_boot_record);
    ERR_CHECK(eCode);

    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));

    return eCode;
}

static cbl_err_code_t update_act (app_type_t app_type, uint32_t new_len)
{
    cbl_err_code_t eCode;
    switch (app_type)
    {
        case TYPE_BIN:
        {
            eCode = update_act_bin(new_len);
        }
        break;

        case TYPE_HEX:
        {
            eCode = update_act_hex(new_len);
        }
        break;

        case TYPE_SREC:
        {
            eCode = update_act_srec(new_len);
        }
        break;

        case TYPE_UNDEF:
        default:
        {
            return CBL_ERR_APP_TYPE;
        }
        break;
    }

    return eCode;
}

static cbl_err_code_t update_act_bin (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    eCode = write_program_bytes(BOOT_ACT_APP_START,
            (uint8_t *)BOOT_NEW_APP_START, new_len);

    return eCode;
}

static cbl_err_code_t update_act_hex (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    return CBL_ERR_NOT_IMPL;

    return eCode;
}

static cbl_err_code_t update_act_srec (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    return CBL_ERR_NOT_IMPL;

    return eCode;
}
/*** end of file ***/

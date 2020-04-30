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

typedef struct
{
    bool is_EOF; /*!< Signal of end of file, set by function 01 */
    uint16_t upper_address; /*!< Set by function 04 */
    uint32_t * p_main; /*!< Set by function 05, BIG ENDIAN */
} h_ihex_t;

static cbl_err_code_t update_act (app_type_t app_type, uint32_t new_len);
static cbl_err_code_t update_act_bin (uint32_t new_len);
static cbl_err_code_t update_act_hex (uint32_t new_len);
static cbl_err_code_t update_act_srec (uint32_t new_len);
static cbl_err_code_t enum_param_force (char * char_force, uint32_t len,
bool * p_force);
static cbl_err_code_t hex_handle_fcn (h_ihex_t * ph_ihex, uint8_t * p_fcn_start,
        uint32_t len, uint32_t * p_fcn_len);
static cbl_err_code_t srec_handle_fcn (uint8_t * p_fcn_start, uint32_t len,
        uint32_t * p_fcn_len);

/**
 * @brief Checks 'boot record' if update to user application is available.
 *        If it is available updates the user application.
 *        Parameters from phPrsr:
 *          force - force update even if flag for update is not set
 *                  valid values TXT_PAR_UP_ACT_TRUE and TXT_PAR_UP_ACT_FALSE
 *
 * @param phPrsr Pointer to handle of parser
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
        char *char_force = NULL;
        bool force = false;

        /* Notify that no update is required */
        const char *msg = "No update needed for user application\r\n";
        INFO("%s", msg);
        eCode = send_to_host(msg, strlen(msg));
        ERR_CHECK(eCode);

        /* Check if force parameter is given */
        char_force = parser_get_val(phPrsr, TXT_PAR_UP_ACT_FORCE,
                strlen(TXT_PAR_UP_ACT_FORCE));
        if (char_force != NULL)
        {
            /* Fill boolean force */
            eCode = enum_param_force(char_force, strlen(char_force), &force);
            ERR_CHECK(eCode);
        }

        if (force == false)
        {
            eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
            return eCode;
        }
    }
    else
    {
        /* Notify that update is available */
        const char *msg = "Update for user application available\r\n";
        INFO("%s", msg);
        eCode = send_to_host(msg, strlen(msg));
        ERR_CHECK(eCode);
    }

    const char *msg = "Updating user application\r\n";
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

    /* Erase user application sectors */
    eCode = flash_erase_sector(BOOT_ACT_APP_START_SECTOR,
    BOOT_ACT_APP_MAX_SECTORS);
    ERR_CHECK(eCode);

    /* Write bytes to active application location */
    eCode = update_act(p_boot_record->new_app.app_type, new_len);
    ERR_CHECK(eCode);

    /* Update active application meta data */
    p_boot_record->act_app.app_type = p_boot_record->new_app.app_type;
    p_boot_record->act_app.cksum_used = p_boot_record->new_app.cksum_used;
    p_boot_record->act_app.len = p_boot_record->new_app.len;

    eCode = boot_record_set(p_boot_record);
    ERR_CHECK(eCode);

    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));

    return eCode;
}

/**
 * @brief Converts text of force parameter to boolean
 *
 * @param char_force Text of force parameter value
 * @param len        Length of char_force
 * @param p_force    Pointer to force boolean
 */
static cbl_err_code_t enum_param_force (char * char_force, uint32_t len,
bool * p_force)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    if (char_force == NULL)
    {
        eCode = CBL_ERR_PAR_FORCE;
    }
    else if (strlen(TXT_PAR_UP_ACT_TRUE) == len
            && strncmp(char_force, TXT_PAR_UP_ACT_TRUE, len) == 0)
    {
        ( *p_force) = true;
    }
    else if (strlen(TXT_PAR_UP_ACT_FALSE) == len
            && strncmp(char_force, TXT_PAR_UP_ACT_FALSE, len) == 0)
    {
        ( *p_force) = false;
    }
    else
    {
        eCode = CBL_ERR_PAR_FORCE;
    }

    return eCode;
}

/**
 * @brief Updates the flash bytes according to app_type
 *
 * @param app_type Application type used in new application
 * @param new_len  Length of new application
 */
static cbl_err_code_t update_act (app_type_t app_type, uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
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

/**
 * @brief Updates bytes of current application from binary new application
 *
 * @param new_len Length of new application
 */
static cbl_err_code_t update_act_bin (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    eCode = write_program_bytes(BOOT_ACT_APP_START,
            (uint8_t *)BOOT_NEW_APP_START, new_len);

    return eCode;
}

/**
 * @brief Updates bytes of current application from Intel hex new application
 *
 * @param new_len Length of new application
 */
static cbl_err_code_t update_act_hex (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_INV_IHEX;
    h_ihex_t h_ihex;
    uint8_t * p_app = (uint8_t *)BOOT_NEW_APP_START;
    uint8_t * p_fcn_start = memchr(p_app, ':', new_len);

    h_ihex.is_EOF = false;
    h_ihex.p_main = 0; /* Unused! */
    h_ihex.upper_address = 0;

    while (p_fcn_start != NULL)
    {
        uint32_t fcn_len;
        uint32_t fcn_offset;

        fcn_offset = p_fcn_start - p_app;

        eCode = hex_handle_fcn( &h_ihex, p_fcn_start, new_len - fcn_offset,
                &fcn_len);
        ERR_CHECK(eCode);

        /* Check if EOF record received */
        if(true == h_ihex.is_EOF)
        {
            eCode = CBL_ERR_OK;
            break;
        }

        /* Get next function start */
        p_fcn_start = memchr(p_fcn_start + fcn_len, ':', new_len);
    }
    return eCode;
}

/**
 * @brief Updates bytes of current application from Motorola S-Record S37-style
 *        new application. Writes to flash
 *
 * @param new_len Length of new application
 */
static cbl_err_code_t update_act_srec (uint32_t new_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint8_t * p_app = (uint8_t *)BOOT_NEW_APP_START;
    uint8_t * p_fcn_start = memchr(p_app, 'S', new_len);

    while (p_fcn_start != NULL)
    {
        uint32_t fcn_len;
        uint32_t fcn_offset;

        fcn_offset = p_fcn_start - p_app;

        eCode = srec_handle_fcn(p_fcn_start, new_len - fcn_offset, &fcn_len);
        ERR_CHECK(eCode);

        /* Get next function start */
        p_fcn_start = memchr(p_fcn_start + fcn_len, 'S', new_len);
    }

    return eCode;
}

static cbl_err_code_t hex_handle_fcn (h_ihex_t * ph_ihex, uint8_t * p_fcn_start,
        uint32_t len, uint32_t * p_fcn_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint8_t fcn_code;
    uint32_t offset = 1;
    uint8_t byte_count; /* Number of byte PAIRS, not number of characters! */
    uint64_t calc_checksum = 0;
    uint8_t expected_checksum;
    uint16_t fcn_address;

    /* 11 is the minimal theoretical function length */
    if (len < 11)
    {
        return CBL_ERR_INV_SREC;
    }

    /* Byte count is on index 1 and 2 */
    eCode = two_hex_chars2ui8(p_fcn_start[offset], p_fcn_start[offset + 1],
            &byte_count);
    ERR_CHECK(eCode);

    /* Accumulate checksum */
    calc_checksum += byte_count;

    /* 1 - ':', 2 - byte count, 4 - address, 2 - fcn code, 2*n -  data,
     * 2 - checksum */
    *p_fcn_len = 1 + 2 + 4 + 2 + byte_count * 2 + 2;

    if ( *p_fcn_len > len)
    {
        return CBL_ERR_INV_IHEX;
    }

    /* Set offset for address */
    offset = 3;

    eCode = four_hex_chars2ui16( &p_fcn_start[offset], 4, &fcn_address);
    ERR_CHECK(eCode);

    /* Calculate checksum for address */
    for (uint32_t iii = 0; iii < 4; iii += 2)
    {
        uint8_t one_byte;

        eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                p_fcn_start[offset + iii + 1], &one_byte);
        ERR_CHECK(eCode);

        calc_checksum += one_byte;
    }

    /* Set to checksum */
    offset = 9 + byte_count * 2;

    /* Fill expected checksum */
    eCode = two_hex_chars2ui8(p_fcn_start[offset], p_fcn_start[offset + 1],
            &expected_checksum);
    ERR_CHECK(eCode);

    /* Set offset for function code */
    offset = 7;

    /* Fill function code */
    eCode = two_hex_chars2ui8(p_fcn_start[offset], p_fcn_start[offset + 1],
            &fcn_code);
    ERR_CHECK(eCode);

    /* Accumulate checksum for function code */
    calc_checksum += fcn_code;

    switch (fcn_code)
    {
        case 00:
        {
            /* Data function handler */

            uint16_t lower_address;
            uint32_t address;
            uint8_t p_data[255]; /* 255 is maximum value for a byte */

            lower_address = fcn_address;

            address = (ph_ihex->upper_address << 16) | lower_address;

            if (IS_ACT_APP_ADDRESS(address) == false)
            {
                return CBL_ERR_SEGMEN;
            }

            /* Move to the data start */
            offset = 9;

            /* Fill p_data */
            for (uint32_t iii = 0; iii < (byte_count * 2); iii += 2)
            {
                /* Fill p_data */
                eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                        p_fcn_start[offset + iii + 1], &p_data[iii / 2]);
                ERR_CHECK(eCode);

                /* Calculate checksum */
                calc_checksum += p_data[iii / 2];
            }

            /* 2's complement */
            calc_checksum = (((calc_checksum ^ 0xFF) & 0xFF) + 1) & 0xFF;

            if (calc_checksum != expected_checksum)
            {
                return CBL_ERR_CKSUM_WRONG;
            }

            eCode = write_program_bytes(address, p_data, byte_count);
            ERR_CHECK(eCode);
        }
        break;

        case 01:
        {
            /* EOF function handler */

            /* 2's complement */
            calc_checksum = ((calc_checksum ^ 0xFF) & 0xFF) + 1;

            if (expected_checksum != calc_checksum)
            {
                return CBL_ERR_CKSUM_WRONG;
            }

            ph_ihex->is_EOF = true;
        }
        break;

        case 04:
        {
            /* Extended linear address handler */

            uint16_t temp_upper_address;

            if (byte_count != 2)
            {
                return CBL_ERR_INV_IHEX;
            }

            /* Put to data field*/
            offset = 9;

            eCode = four_hex_chars2ui16( &p_fcn_start[offset], 4,
                    &temp_upper_address);
            ERR_CHECK(eCode);

            /* Calculate checksum for data field */
            for (uint32_t iii = 0; iii < 4; iii += 2)
            {
                uint8_t one_byte;

                eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                        p_fcn_start[offset + iii + 1], &one_byte);
                ERR_CHECK(eCode);

                calc_checksum += one_byte;
            }

            /* 2's complement */
            calc_checksum = ((calc_checksum ^ 0xFF) & 0xFF) + 1;

            if (expected_checksum != calc_checksum)
            {
                return CBL_ERR_CKSUM_WRONG;
            }

            ph_ihex->upper_address = temp_upper_address;
        }
        break;

        case 05:
        {
            /* Start linear address handler */

            uint32_t main_fcn;

            if (byte_count != 4)
            {
                return CBL_ERR_INV_IHEX;
            }

            /* Put to data field */
            offset = 9;

            /* BIG ENDIAN */
            eCode = eight_hex_chars2ui32( &p_fcn_start[offset], 8, &main_fcn);
            ERR_CHECK(eCode);

            /* Calculate checksum for data field */
            for (uint32_t iii = 0; iii < 8; iii += 2)
            {
                uint8_t one_byte;

                eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                        p_fcn_start[offset + iii + 1], &one_byte);
                ERR_CHECK(eCode);

                calc_checksum += one_byte;
            }

            /* 2's complement */
            calc_checksum = ((calc_checksum ^ 0xFF) & 0xFF) + 1;

            if (expected_checksum != calc_checksum)
            {
                return CBL_ERR_CKSUM_WRONG;
            }

            /* BIG ENDIAN */
            ph_ihex->p_main = (uint32_t *)main_fcn;
        }
        break;

        default:
        {
            return CBL_ERR_IHEX_FCN;
        }
        break;

    }

    return eCode;
}

/**
 * @brief Handles given S-record function and write to active application flash
 *        if needed
 *
 * @note Flash sectors containing active application shall be erased before
 *
 * @param p_fcn_start[in] Pointer to function
 * @param len[in]         Length of buffer containing function
 * @param p_fcn_len[out]  Length of the actual function
 *
 * @return Error status
 */
static cbl_err_code_t srec_handle_fcn (uint8_t * p_fcn_start, uint32_t len,
        uint32_t * p_fcn_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint8_t fcn_num;
    uint32_t offset = 2;
    uint8_t byte_count; /* Number of byte PAIRS, not number of characters! */

    /* 8 is the minimal theoretical function length */
    if (len < 8)
    {
        return CBL_ERR_INV_SREC;
    }

    /* Byte count is on index 2 and 3, big endian format */
    eCode = two_hex_chars2ui8(p_fcn_start[offset], p_fcn_start[offset + 1],
            &byte_count);
    ERR_CHECK(eCode);

    *p_fcn_len = byte_count * 2 + 4; /* 4 = 'S', function type and byte count */

    if (len < *p_fcn_len || byte_count < 3)
    {
        return CBL_ERR_INV_SREC;
    }

    fcn_num = p_fcn_start[1];

    /* Move to starting index of Address */
    offset += 2;

    switch (fcn_num)
    {
        case '0':
        {
            /* Header: contains description of following bytes
             * SW4STM32 usually writes file name */
            /* Handle not needed */
        }
        break;

        case '3':
        {
            uint32_t address;
            uint8_t data_len = byte_count * 2 - 8 - 2; /* - 8 for address,
             - 2 for checksum */
            uint8_t p_data[255 - 4 - 1] = { 0 }; /* 255 is max value of
             one byte */
            uint8_t expected_checksum = 0;
            uint64_t calc_checksum = 0;

            eCode = eight_hex_chars2ui32( &p_fcn_start[offset], 8, &address);
            ERR_CHECK(eCode);

            /* Calculate checksum */
            calc_checksum = byte_count;

            /* Calculate checksum for address */
            for (uint32_t iii = 0; iii < 8; iii += 2)
            {
                uint8_t one_byte;

                eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                        p_fcn_start[offset + iii + 1], &one_byte);
                ERR_CHECK(eCode);

                calc_checksum += one_byte;
            }

            /* Move offset to data */
            offset += 8;

            if (IS_ACT_APP_ADDRESS(address) == false)
            {
                return CBL_ERR_SEGMEN;
            }

            for (uint32_t iii = 0; iii < data_len; iii += 2)
            {
                /* Fill p_data */
                eCode = two_hex_chars2ui8(p_fcn_start[offset + iii],
                        p_fcn_start[offset + iii + 1], &p_data[iii / 2]);
                ERR_CHECK(eCode);

                /* Calculate checksum */
                calc_checksum += p_data[iii / 2];
            }

            /* Move offset to checksum */
            offset += data_len;

            /* Fill checksum */
            eCode = two_hex_chars2ui8(p_fcn_start[offset],
                    p_fcn_start[offset + 1], &expected_checksum);
            ERR_CHECK(eCode);

            if (((calc_checksum ^ 0xFF) & 0xFF) != expected_checksum)
            {
                return CBL_ERR_CKSUM_WRONG;
            }

            eCode = write_program_bytes(address, p_data, data_len / 2);
            ERR_CHECK(eCode);
        }
        break;

        case '5':
        {
            /* Optional */
            /* Contains number of 'S3' functions in a file */
            /* Handle not needed */
        }
        break;

        case '6':
        {
            /* Optional */
            /* Contains number of 'S3' functions in a file */
            /* Handle not needed */
        }
        break;

        case '7':
        {
            /* File terminator */
            /* Contains starting execution location */
            /* Handle not needed for memory devices */
        }
        break;

        default:
        {
            return CBL_ERR_SREC_FCN;
        }
        break;

    }

    return eCode;
}

/*** end of file ***/

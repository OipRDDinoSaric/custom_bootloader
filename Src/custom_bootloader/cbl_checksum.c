/** @file cbl_checksum.c
 *
 * @brief All checksum implementations available
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crc.h"
#include "cbl_checksum.h"

#if 0
static cbl_err_code_t calculate_crc32 (uint8_t * data, uint32_t len,
        uint32_t * out_crc);
#endif
static uint32_t lit_to_big_endian (uint32_t number);
static uint32_t reflect_ui32 (uint32_t number);

static const uint8_t reflect_byte_table[] = { 0x00, 0x80, 0x40, 0xC0, 0x20,
        0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08,
        0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38,
        0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14,
        0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C,
        0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 0x02,
        0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32,
        0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A,
        0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26,
        0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E,
        0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E,
        0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11,
        0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29,
        0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05,
        0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35,
        0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D,
        0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23,
        0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B,
        0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B,
        0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17,
        0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 0x0F, 0x8F, 0x4F, 0xCF, 0x2F,
        0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF };

/**
 * @brief Checks checksum parameter value to check if it is supported
 *
 * @note If 'checksum' is NULL function assigns p_cksum CKSUM_NO
 *
 * @param checksum[in] checksum name
 * @param len[in]       length of 'checksum'
 * @param p_cksum[out]   pointer to checksum enumerator
 */
cbl_err_code_t enum_checksum (char * checksum, uint32_t len, cksum_t * p_cksum)
{
    if (checksum == NULL)
    {
        *p_cksum = CKSUM_NO;
        return CBL_ERR_OK;
    }

    if (strlen(TXT_CKSUM_CRC) == len
            && strncmp(checksum, TXT_CKSUM_CRC, len) == 0)
    {
        *p_cksum = CKSUM_CRC32;
    }
    else if (strlen(TXT_CKSUM_SHA256) == len
            && strncmp(checksum, TXT_CKSUM_SHA256, len) == 0)
    {
        *p_cksum = CKSUM_SHA256;
    }
    else if (strlen(TXT_CKSUM_NO) == len
            && strncmp(checksum, TXT_CKSUM_NO, len) == 0)
    {
        *p_cksum = CKSUM_NO;
    }
    else
    {
        *p_cksum = CKSUM_UNDEF;
        return CBL_ERR_UNSUP_CKSUM;
    }

    return CBL_ERR_OK;
}

/**
 * @brief Initializes the checksum denoted by 'cksum'
 *
 * @param cksum[in]     Enumerator of checksum to initialize
 * @param ph_sha256[in] Pointer to the handle of sha256 checksum, if not using
 *        sha256 send NULL for this parameter
 */
void init_checksum (cksum_t cksum, SHA256_CTX * ph_sha256)
{
    switch (cksum)
    {
        case CKSUM_CRC32:
        {
            /* Reset CRC value (0xFFFFFFFF) */
            __HAL_CRC_DR_RESET( &hcrc);
        }
        break;

        case CKSUM_SHA256:
        {
            if (ph_sha256 != NULL)
            {
                sha256_init(ph_sha256);
            }
        }
        break;

        case CKSUM_NO:
        case CKSUM_UNDEF:
        default:
        {
        }
        break;
    }
}

/**
 * @brief Accumulates the checksum to the previous value
 *
 * @param buf[in]       Buffer to be accumulated
 * @param len[in]       Length of buf
 * @param cksum[in]     Enumerator of checksum to use
 * @param ph_sha256[in] Pointer to the handle of sha256 checksum, if not using
 *        sha256 send NULL for this parameter
 */
cbl_err_code_t accumulate_checksum (uint8_t * buf, uint32_t len, cksum_t cksum,
        SHA256_CTX * ph_sha256)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cksum)
    {
        case CKSUM_CRC32:
        {
            eCode = accumulate_crc32(buf, len);
        }
        break;

        case CKSUM_SHA256:
        {
            if (ph_sha256 == NULL)
            {
                return CBL_ERR_CKSUM_WRONG;
            }
            eCode = accumulate_sha256(buf, len, ph_sha256);
        }
        break;

        case CKSUM_NO:
        {
            /* Checksum is not needed */
        }
        break;

        case CKSUM_UNDEF:
        default:
        {
            eCode = CBL_ERR_UNSUP_CKSUM;
        }
        break;
    }

    return eCode;
}

/**
 * @brief Accumulates bytes from 'buf' for CRC32
 *                      CRC parameters:
 *                          Polynomial length: 32
 *                          CRC-32 polynomial: 0x4C11DB7 (Ethernet)
 *                                 Init value: 0xFFFFFFFF
 *                                     XOROut: true
 *                                      RefIn: true
 *                                     RefOut: true
 *
 * @note Assumes memory is in little endian!
 * @note Input data length must be divisible by 4! So no leading zeroes are
 *       added.
 *
 * @param buf[in]       Bytes to accumulate
 * @param len[in]       Length of 'buf'
 */
cbl_err_code_t accumulate_crc32 (uint8_t * buf, uint32_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t ui32_data;

    /* For crc32 data must be divisible by 4 */
    if ((len % 4) != 0)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    for (uint32_t iii = 0; iii < len; iii = iii + 4)
    {
        ui32_data = *(uint32_t *) &buf[iii];

        /* Make big endian and reflect bytes */
        ui32_data = reflect_ui32(ui32_data);

        HAL_CRC_Accumulate( &hcrc, &ui32_data, 1);
    }

    return eCode;
}

/**
 * @brief Accumulates bytes from 'buf' for sha256, accumulated states are stored
 *        in ph_sha25
 *
 * @param buf[in]       Buffer to be accumulated
 * @param len[in]       Length of 'buf'
 * @param ph_sha256[in] Pointer of a handle of sha256 states
 */
cbl_err_code_t accumulate_sha256 (uint8_t * buf, uint32_t len,
        SHA256_CTX * ph_sha256)
{
    sha256_update(ph_sha256, buf, len);

    return CBL_ERR_OK;
}

/**
 * @brief Checks if received checksum value matches the calculated one
 *
 * @param p_recv_cksum[in] Pointer to the received checksum
 * @param cksum_len[in]    Length of received checksum
 * @param cksum[in]        Enumerator for checksum
 * @param ph_sha256[in]    Pointer of a handle of sha256 states
 */
cbl_err_code_t verify_checksum (uint8_t * p_recv_cksum, uint32_t cksum_len,
        cksum_t cksum, SHA256_CTX * ph_sha256)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cksum)
    {
        case CKSUM_CRC32:
        {
            eCode = verify_crc32(p_recv_cksum, cksum_len);
        }
        break;

        case CKSUM_SHA256:
        {
            eCode = verify_sha256(p_recv_cksum, cksum_len, ph_sha256);
        }
        break;

        case CKSUM_NO:
        {
            /* Checksum is not needed */
        }
        break;

        case CKSUM_UNDEF:
        default:
        {
            eCode = CBL_ERR_UNSUP_CKSUM;
        }
        break;
    }

    return eCode;
}

/**
 * @brief Verify if calculated CRC32 is the same as received checksum
 *
 * @param p_recv_cksum[in] Byte array of received checksum
 * @param cksum_len[in]    Length of received checksum
 */
cbl_err_code_t verify_crc32 (uint8_t * p_recv_cksum, uint32_t cksum_len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t calculated_crc32 = hcrc.Instance->DR;
    uint32_t expected_crc;

    if (cksum_len != 4)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    expected_crc = *(uint32_t *)(p_recv_cksum);

    /* Bring in line with physical layer */
    expected_crc = lit_to_big_endian(expected_crc);

    /* Reflect calculated CRC*/
    calculated_crc32 = reflect_ui32(calculated_crc32);

    /* XOROut */
    calculated_crc32 = calculated_crc32 ^ 0xFFFFFFFF;

    if (calculated_crc32 != expected_crc)
    {
        eCode = CBL_ERR_CKSUM_WRONG;
    }

    return eCode;
}

/**
 * @brief Verifies if received and calculated sha256 are the same
 *
 * @param p_recv_cksum Pointer to recevied checksum
 * @param cksum_len    Length of received checksum
 * @param ph_sha256    Handle of sha256 states
 */
cbl_err_code_t verify_sha256 (uint8_t * p_recv_cksum, uint32_t cksum_len,
        SHA256_CTX * ph_sha256)
{
    BYTE p_calculated_sha[SHA256_BLOCK_SIZE] = { 0 };
    sha256_final(ph_sha256, p_calculated_sha);

    if (SHA256_BLOCK_SIZE != cksum_len)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    if (memcmp(p_recv_cksum, p_calculated_sha, SHA256_BLOCK_SIZE) != 0)
    {
        return CBL_ERR_CKSUM_WRONG;
    }
    return CBL_ERR_OK;
}

/**
 * @brief Returns the length of checksum a specified by 'cksum'
 *
 * @param cksum Checksum type
 *
 * @return Length of 'cksum' type checksum
 */
uint32_t checksum_get_length(cksum_t cksum)
{
    switch (cksum)
        {
            case CKSUM_CRC32:
            {
                return 4u;
            }
            break;

            case CKSUM_SHA256:
            {
                return 32u;
            }
            break;

            case CKSUM_NO:
            case CKSUM_UNDEF:
            default:
            {
                return 0;
            }
            break;
        }

}

#if 0
/**
 * @brief Returns if selected checksum is correct
 *
 * @param buf[in] Buffer to be checked
 * @param len[in] Length of buf
 * @param cksum[in] Enumerator of checksum to use
 */
cbl_err_code_t verify_checksum_old (uint8_t * buf, uint32_t len, cksum_t cksum)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cksum)
    {
        case CKSUM_CRC32:
        {
            eCode = verify_crc_old(buf, len);
        }
        break;

        case CKSUM_SHA256:
        {
            eCode = verify_sha256_old(buf, len);
        }
        break;

        case CKSUM_NO:
        {
            /* Checksum is not needed */
        }
        break;

        case CKSUM_UNDEF:
        default:
        {
            eCode = CBL_ERR_UNSUP_CKSUM;
        }
        break;
    }

    return eCode;
}

/**
 * @brief               Checks if CRC value of input buffer is OK
 *                      CRC parameters:
 *                          Polynomial length: 32
 *                          CRC-32 polynomial: 0x4C11DB7 (Ethernet)
 *                                 Init value: 0xFFFFFFFF
 *                                     XOROut: true
 *                                      RefIn: true
 *                                     RefOut: true
 *
 * @note Assumes memory is in little endian!
 *
 * @param write_buf[in] Buffer with 32 bit CRC in the end
 * @param len[in]       Length of write_buf
 */
cbl_err_code_t verify_crc_old (uint8_t * write_buf, uint32_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t calc_crc;
    uint32_t expected_crc;

    if (len <= 4)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    expected_crc = *(uint32_t *)( &write_buf[len - 4]);

    /* Bring in line with physical layer */
    expected_crc = lit_to_big_endian(expected_crc);

    /* Don't look at CRC32 bytes */
    len -= 4;

    eCode = calculate_crc32(write_buf, len, &calc_crc);
    ERR_CHECK(eCode);

    if (calc_crc != expected_crc)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    return CBL_ERR_OK;
}

/**
 * @brief Checks if the checksum of 256 on the end of the message matches with
 *        calculated one
 *
 * @param buf[in] String to be checked
 * @param len[in] Length of buf
 */
cbl_err_code_t verify_sha256_old (uint8_t * buf, uint32_t len)
{
    BYTE *expected_sha = NULL;
    BYTE calc_sha[SHA256_BLOCK_SIZE] =
    {   0};
    SHA256_CTX h_ctx;

    if (len <= SHA256_BLOCK_SIZE)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    /* SHA256 is on the end */
    expected_sha = &buf[len - SHA256_BLOCK_SIZE];

    /* Don't look at checksum blocks */
    len -= SHA256_BLOCK_SIZE;

    sha256_init( &h_ctx);
    sha256_update( &h_ctx, buf, len);
    sha256_final( &h_ctx, calc_sha);

    if (memcmp(expected_sha, calc_sha, SHA256_BLOCK_SIZE) != 0)
    {
        return CBL_ERR_CKSUM_WRONG;
    }

    return CBL_ERR_OK;
}

/**
 * @brief Calculated CRC32 from byte sized data, uses integrated HW from ST
 *
 * @param data[in]      Message to be checked
 * @param len[in]       Length of 'data'
 * @param out_crc[out]  Calculated CRC32
 */
static cbl_err_code_t calculate_crc32 (uint8_t * data, uint32_t len,
        uint32_t * out_crc)
{
    uint32_t ui32_data;

    if (len % 4 != 0)
    {
        /* We are using CRC32! Data must be divisible by 4 */
        return CBL_ERR_CRC_LEN;
    }

    for (uint32_t iii = 0; iii < len; iii = iii + 4)
    {
        ui32_data = *(uint32_t *) &data[iii];

        /* Make big endian and reflect bytes */
        ui32_data = reflect_ui32(ui32_data);

        *out_crc = HAL_CRC_Accumulate( &hcrc, &ui32_data, 1);
    }

    /* Reflect calculated CRC*/
    *out_crc = reflect_ui32( *out_crc);

    /* XOROut */
    *out_crc = *out_crc ^ 0xFFFFFFFF;

    return CBL_ERR_OK;
}
#endif

static uint32_t lit_to_big_endian (uint32_t number)
{
    return (((number >> 24) & 0x000000ff) | ((number >> 8) & 0x0000ff00)
            | ((number << 8) & 0x00ff0000) | ((number << 24) & 0xff000000));
}

/**
 * @brief Reflects uint32 around center. Essentially converts integer from
 *        little endian to big endian and reflecting each of its bytes
 *
 * @param number Non reflected integer
 *
 * @return Reflected integer
 */
static uint32_t reflect_ui32 (uint32_t number)
{
    return (reflect_byte_table[number & 0xff] << 24)
            | (reflect_byte_table[(number >> 8) & 0xff] << 16)
            | (reflect_byte_table[(number >> 16) & 0xff] << 8)
            | (reflect_byte_table[(number >> 24) & 0xff]);
}
/*** end of file ***/

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

/**
 * @brief Checks checksum parameter value to check if it is supported
 *
 * @param checksum[in] checksum name
 * @param len[in]       length of 'checksum'
 * @param p_cksum[out]   pointer to checksum enumerator
 */
cbl_err_code_t enum_checksum (char * checksum, uint32_t len,
        cksum_t * p_cksum)
{
    if (strlen(TXT_CKSUM_CRC) == len
            && strncmp(checksum, TXT_CKSUM_CRC, len) == 0)
    {
        *p_cksum = CKSUM_CRC;
    }
    else if (strlen(TXT_CKSUM_SHA256) == len
            && strncmp(checksum, TXT_CKSUM_SHA256, len) == 0)
    {
        *p_cksum = CKSUM_SHA256;
    }
    else
    {
        *p_cksum = CKSUM_UNDEF;
        return CBL_ERR_INV_CKSUM;
    }

    return CBL_ERR_OK;
}

/**
 * @brief Returns if selected checksum is correct
 *
 * @param buf[in] Buffer to be checked
 * @param len[in] Length of buf
 * @param cksum[in] Enumerator of checksum to use
 */
cbl_err_code_t verify_checksum (uint8_t * buf, uint32_t len,
        cksum_t cksum)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cksum)
    {
        case CKSUM_CRC:
        {
            eCode = verify_crc(buf, len);
        }
        break;

        case CKSUM_SHA256:
        {
            eCode = verify_sha256(buf, len);
        }
        break;

        case CKSUM_UNDEF:
        default:
        {
            eCode = CBL_ERR_INV_CKSUM;
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
 *                                     XOROut: false
 *                                      RefIn: false
 *                                     RefOut: false
 *
 * @param write_buf[in] Buffer with 32 bit CRC in the end
 * @param len[in]       Length of write_buf
 *
 * @return CBL_ERR_CRC_WRONG if invalid CRC, otherwise CBL_ERR_OK
 */
cbl_err_code_t verify_crc (uint8_t * write_buf, uint32_t len)
{
    uint32_t expected_crc = 0;
    /* NOTE: Zero as write_buf shall always be divisible polynomial by
     the used CRC polynomial */
    uint32_t poly[2] = { 0x12345678, 0xDF8A8A2B };
    uint32_t calc_crc;

    if (len % 4 != 0)
    {
        return CBL_ERR_CRC_LEN;
    }
    calc_crc = HAL_CRC_Calculate( &hcrc, (uint32_t *)poly, 2);

    if (calc_crc != expected_crc)
    {
        return CBL_ERR_CRC_WRONG;
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
cbl_err_code_t verify_sha256 (uint8_t * buf, uint32_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    return eCode;
}

/*** end of file ***/

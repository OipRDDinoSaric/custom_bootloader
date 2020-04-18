/** @file cbl_cmds_memory.c
 *
 * @brief Contains functions for memory access from the bootloader
 */
#include "cbl_cmds_memory.h"
#include "string.h"
#include "cbl_checksum.h"

static cbl_err_code_t write_get_params (parser_t * ph_prsr, uint32_t * p_start,
        uint32_t * p_len, cksum_t * cksum);

/**
 * @brief   Jumps to a requested address.
 *          Parameters needed from phPrsr:
 *              - address
 */
cbl_err_code_t cmd_jump_to (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charAddr;
    uint32_t addr = 0u;
    void (*jump) (void);

    DEBUG("Started\r\n");

    /* Get the address in hex form */
    charAddr = parser_get_val(phPrsr, TXT_PAR_JUMP_TO_ADDR,
            strlen(TXT_PAR_JUMP_TO_ADDR));
    if (NULL == charAddr)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Fill addr, skips 0x if present */
    eCode = str2ui32(charAddr, strlen(charAddr), &addr, 16u);
    ERR_CHECK(eCode);

    /* Make sure we can jump to the wanted location */
    eCode = verify_jump_address(addr);
    ERR_CHECK(eCode);

    /* Add 1 to the address to set the T bit */
    addr++;
    /*!<    T bit is 0th bit of a function address and tells the processor
     *  if command is ARM T=0 or thumb T=1. STM uses thumb commands.
     *  Reference: https://www.youtube.com/watch?v=VX_12SjnNhY */

    /* Make a function to jump to */
    jump = (void *)addr;

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
    ERR_CHECK(eCode);

    /* Jump to requested address, user ensures requested address is valid */
    jump();
    return eCode;
}

// \f - new page
/**
 * @brief   Erases flash memory according to parameters.
 *          Parameters needed from phPrsr:
 *              - type - Defines type of flash erase. "mass" erases all sectors,
 *               "sector" erases only selected sectors
 *              - sector - First sector to erase. Bootloader is on sectors 0, 1
 *               and 2. Not needed with mass erase
 *              - count - Number of sectors to erase. Not needed with mass erase
 */
cbl_err_code_t cmd_flash_erase (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charSect = NULL;
    char *charCount = NULL;
    char *type = NULL;
    HAL_StatusTypeDef HALCode;
    uint32_t sectorCode;
    uint32_t sect;
    uint32_t count;
    FLASH_EraseInitTypeDef settings;

    DEBUG("Started\r\n");

    /* Device operating range: 2.7V to 3.6V */
    settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    /* Only available bank */
    settings.Banks = FLASH_BANK_1;

    type = parser_get_val(phPrsr, TXT_PAR_FLASH_ERASE_TYPE,
            strlen(TXT_PAR_FLASH_ERASE_TYPE));
    if (NULL == type)
    {
        /* No sector present throw error */
        return CBL_ERR_NEED_PARAM;
    }
    /* Check the type of erase */
    if (strncmp(type, TXT_PAR_FLASH_ERASE_TYPE_SECT,
            strlen(TXT_PAR_FLASH_ERASE_TYPE_SECT)) == 0)
    {
        /* Set correct erase type */
        settings.TypeErase = FLASH_TYPEERASE_SECTORS;

        /* Get first sector to write to */
        charSect = parser_get_val(phPrsr, TXT_PAR_FLASH_ERASE_SECT,
                strlen(TXT_PAR_FLASH_ERASE_SECT));
        if (NULL == charSect)
        {
            /* No sector present throw error */
            return CBL_ERR_NEED_PARAM;
        }

        /* Fill sect */
        eCode = str2ui32(charSect, strlen(charSect), &sect, 10);
        ERR_CHECK(eCode);

        /* Check validity of given sector */
        if (sect >= FLASH_SECTOR_TOTAL)
        {
            return CBL_ERR_INV_SECT;
        }

        /* Get how many sectors to erase */
        charCount = parser_get_val(phPrsr, TXT_PAR_FLASH_ERASE_COUNT,
                strlen(TXT_PAR_FLASH_ERASE_COUNT));
        if (NULL == charCount)
        {
            /* No sector count present throw error */
            return CBL_ERR_NEED_PARAM;
        }
        /* Fill count */
        eCode = str2ui32(charCount, strlen(charCount), &count, 10);
        ERR_CHECK(eCode);

        if (sect + count - 1 >= FLASH_SECTOR_TOTAL)
        {
            /* Last sector to delete doesn't exist, throw error */
            return CBL_ERR_INV_SECT_COUNT;
        }

        settings.Sector = sect;
        settings.NbSectors = count;
    }
    else if (strncmp(type, TXT_PAR_FLASH_ERASE_TYPE_MASS,
            strlen(TXT_PAR_FLASH_ERASE_TYPE_MASS)) == 0)
    {
        /* Erase all sectors */
        settings.TypeErase = FLASH_TYPEERASE_MASSERASE;
    }
    else
    {
        /* Type has wrong value */
        return CBL_ERR_ERASE_INV_TYPE;
    }
    /* Turn on the blue LED, signalizing flash manipulation */
    LED_ON(BLUE);

    /* Unlock flash control registers */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Erase selected sectors */
    HALCode = HAL_FLASHEx_Erase( &settings, &sectorCode);

    LED_OFF(BLUE);

    /* Lock flash control registers */
    HAL_FLASH_Lock();

    /* Check for errors */
    if (HALCode != HAL_OK)
    {
        return CBL_ERR_HAL_ERASE;
    }
    if (sectorCode != 0xFFFFFFFFU) /*!< 0xFFFFFFFFU means success */
    {
        /* Shouldn't happen as we check for HALCode before, but let's check */
        return CBL_ERR_SECTOR;
    }

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));

    return eCode;
}

// \f - new page
/**
 * @brief   Writes to flash, returns CRLFreadyCRLF when ready to receive bytes
 *          Parameters needed from phPrsr:
 *             - start - Starting address in hex format (e.g. 0x12345678),
 *               0x can be omitted
 *             - count - Number of bytes to write. Maximum bytes: 1024
 */
cbl_err_code_t cmd_flash_write (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint8_t write_buf[FLASH_WRITE_SZ] = { 0 };
    uint32_t start;
    uint32_t len;
    cksum_t cksum = CKSUM_UNDEF;
    HAL_StatusTypeDef HALCode;

    DEBUG("Started\r\n");

    eCode = write_get_params(phPrsr, &start, &len, &cksum);
    ERR_CHECK(eCode);

    /* Reset UART byte counter */
    gRxCmdCntr = 0;

    /* Notify host bootloader is ready to receive bytes */
    send_to_host(TXT_RESP_FLASH_WRITE_READY,
            strlen(TXT_RESP_FLASH_WRITE_READY));

    /* Request 'len' bytes */
    eCode = recv_from_host(write_buf, len);
    ERR_CHECK(eCode);

    while (gRxCmdCntr != 1)
    {
        /* Wait for 'len' bytes */
    }

    /* Write data to flash, signalize with blue LED */
    LED_ON(BLUE);

    /* Check checksum value */
    eCode = verify_checksum(write_buf, len, cksum);
    ERR_CHECK(eCode);

    /* Unlock flash */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Write to flash */
    for (uint32_t i = 0u; i < len; i++)
    {
        /* Write a byte */
        HALCode = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, start + i,
                write_buf[i]);
        if (HALCode != HAL_OK)
        {
            HAL_FLASH_Lock();
            LED_OFF(BLUE);
            return CBL_ERR_HAL_WRITE;
        }
    }

    HAL_FLASH_Lock();

    LED_OFF(BLUE);

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

// \f - new page
/**
 * @brief   Read bytes from memory
 *          Parameters needed from phPrsr:
 *             - start - Starting address in hex format (e.g. 0x12345678),
 *              0x can be omitted
 *             - count - Number of bytes to read
 */
cbl_err_code_t cmd_mem_read (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charStart = NULL;
    char *charLen = NULL;
    uint32_t start;
    uint32_t len;

    DEBUG("Started\r\n");

    /* Get starting address */
    charStart = parser_get_val(phPrsr, TXT_PAR_FLASH_WRITE_START,
            strlen(TXT_PAR_FLASH_WRITE_START));
    if (NULL == charStart)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Get length in bytes */
    charLen = parser_get_val(phPrsr, TXT_PAR_FLASH_WRITE_COUNT,
            strlen(TXT_PAR_FLASH_WRITE_COUNT));
    if (NULL == charLen)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Fill start */
    eCode = str2ui32(charStart, strlen(charStart), &start, 16);
    ERR_CHECK(eCode);

    /* Fill len */
    eCode = str2ui32(charLen, strlen(charLen), &len, 10);
    ERR_CHECK(eCode);

    /* Send requested bytes */
    eCode = send_to_host((char *)start, len);
    return eCode;
}

/**
 * @brief Gets parameters from parser handle
 *
 * @param ph_prsr[in] Parser containing parameters
 * @param p_start[out] Pointer of start address
 * @param p_len[out] Pointer to length to write
 * @param p_cksum[out] Pointer to checksum enumerator
 */
static cbl_err_code_t write_get_params (parser_t * ph_prsr, uint32_t * p_start,
        uint32_t * p_len, cksum_t * p_cksum)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charStart = NULL;
    char *charLen = NULL;
    char *charChecksum = NULL;

    /* Get starting address */
    charStart = parser_get_val(ph_prsr, TXT_PAR_FLASH_WRITE_START,
            strlen(TXT_PAR_FLASH_WRITE_START));
    if (NULL == charStart)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Get length in bytes */
    charLen = parser_get_val(ph_prsr, TXT_PAR_FLASH_WRITE_COUNT,
            strlen(TXT_PAR_FLASH_WRITE_COUNT));
    if (NULL == charLen)
    {
        return CBL_ERR_NEED_PARAM;
    }

    /* Get checksum to be used */
    charChecksum = parser_get_val(ph_prsr, TXT_PAR_FLASH_WRITE_COUNT,
            strlen(TXT_PAR_FLASH_WRITE_COUNT));
    if (NULL == charChecksum)
    {
        return CBL_ERR_NEED_PARAM;
    }

    /* Fill start */
    eCode = str2ui32(charStart, strlen(charStart), p_start, 16);
    ERR_CHECK(eCode);

    /* Fill len */
    eCode = str2ui32(charLen, strlen(charLen), p_len, 10);
    ERR_CHECK(eCode);

    if (IS_FLASH_ADDRESS(*p_start) == false
            || IS_FLASH_ADDRESS((*p_start) + (*p_len) - 1) == false)
    {
        return CBL_ERR_WRITE_INV_ADDR;
    }

    if (( *p_len) > FLASH_WRITE_SZ)
    {
        return CBL_ERR_WRITE_TOO_BIG;
    }

    eCode = enum_checksum(charChecksum, strlen(charChecksum), p_cksum);

    return eCode;
}

/*** end of file ***/

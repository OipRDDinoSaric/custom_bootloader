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
#include "stm32f4xx_hal.h"
#include "cbl_boot_record.h"

static volatile cbl_record_t cbl_record __attribute__((section(".appbr")));

static boot_err_t erase_prev_boot_record (void);

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
boot_err_t cbl_boot_record_set (cbl_record_t * p_new_cbl_record)
{
    boot_err_t eCode = BOOT_OK;
    uint32_t len = (uint32_t)sizeof(cbl_record_t);
    uint8_t *p_new_byte = (uint8_t *)p_new_cbl_record;

    eCode = erase_prev_boot_record();
    if (eCode != BOOT_OK)
    {
        return eCode;
    }

    eCode = cbl_program_bytes(p_new_byte, BOOT_RECORD_START, len);
    return eCode;
}

/**
 * @brief Uses HAL level commands to write data to memory
 * @param p_new_byte Array of bytes to be written
 * @param start Starting address to be written to
 * @param len Length of 'p_new_byte'
 */
boot_err_t cbl_program_bytes (uint8_t * p_new_byte, uint32_t start,
        uint32_t len)
{
    HAL_StatusTypeDef HALCode;

    /* Unlock flash */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return BOOT_ERR;
    }

    /* Write to flash */
    for (uint32_t iii = 0u; iii < len; iii++)
    {
        /* Write a byte */
        HALCode = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
        start + iii, *(p_new_byte + iii));
        if (HALCode != HAL_OK)
        {
            HAL_FLASH_Lock();
            return BOOT_ERR;
        }
    }

    HAL_FLASH_Lock();
    return BOOT_OK;
}

/**
 * @brief Erases a sector where boot record is
 */
static boot_err_t erase_prev_boot_record (void)
{
    FLASH_EraseInitTypeDef settings;
    HAL_StatusTypeDef HALCode;
    uint32_t sectorCode;
    /* Device operating range: 2.7V to 3.6V */
    settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    /* Only available bank */
    settings.Banks = FLASH_BANK_1;

    /* Select sector with boot record */
    settings.TypeErase = FLASH_TYPEERASE_SECTORS;
    settings.Sector = BOOT_RECORD_SECTOR;
    settings.NbSectors = 1;

    /* Unlock flash control registers */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return BOOT_ERR;
    }

    /* Erase selected sectors */
    HALCode = HAL_FLASHEx_Erase( &settings, &sectorCode);

    /* Lock flash control registers */
    HAL_FLASH_Lock();

    /* Check for errors */
    if (HALCode != HAL_OK)
    {
        return BOOT_ERR;
    }

    if (sectorCode != 0xFFFFFFFFU) /*!< 0xFFFFFFFFU means success */
    {
        /* Shouldn't happen as we check for HALCode before */
        return BOOT_ERR;
    }

    return BOOT_OK;
}
/*** end of file ***/

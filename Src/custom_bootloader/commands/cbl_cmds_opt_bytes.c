/** @file cbl_cmds_opt_bytes.c
 *
 * @brief Function handles for handling option bytes
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cbl_common.h"
#include "cbl_cmds_opt_bytes.h"

/**
 * @brief   RDP - Read protection
 *              - Used to protect the software code stored in Flash memory.
 *              - Reference manual - p. 93 - Explanation of RDP
 */
cbl_err_code_t cmd_get_rdp_lvl (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    FLASH_OBProgramInitTypeDef optBytes;
    char buf[32] = "";

    DEBUG("Started\r\n");

    HAL_FLASHEx_OBGetConfig( &optBytes);

    /* Fill buffer with correct value of RDP */
    switch (optBytes.RDPLevel)
    {
        case OB_RDP_LEVEL_0:
        {
            strcpy(buf, "level 0");
        }
        break;

        case OB_RDP_LEVEL_2:
        {
            strcpy(buf, "level 2");
        }
        break;

        default:
        {
            /* Any other value is RDP level 1 */
            strcpy(buf, "level 1");
        }
        break;
    }

    strlcat(buf, CRLF, 32);

    /* Send response */
    eCode = send_to_host(buf, strlen(buf));

    return eCode;
}

/**
 * @brief       Enables write protection on individual flash sectors
 *              Parameters needed from phPrsr:
 *                  - mask - Mask in hex form for sectors where LSB
 *                   corresponds to sector 0
 *
 * @param EnDis Write protection state: OB_WRPSTATE_ENABLE or
 *              OB_WRPSTATE_DISABLE
 */
cbl_err_code_t cmd_change_write_prot (parser_t * phPrsr, uint32_t EnDis)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charMask = NULL;
    uint32_t mask = 0u;
    FLASH_OBProgramInitTypeDef pOBInit;

    DEBUG("Started\r\n");

    /* Assert parameter */
    if (EnDis != OB_WRPSTATE_ENABLE && EnDis != OB_WRPSTATE_DISABLE)
    {
        return CBL_ERR_INV_PARAM;
    }

    /* Mask of sectors to affect */
    charMask = parser_get_val(phPrsr, TXT_PAR_EN_WRITE_PROT_MASK,
            strlen(TXT_PAR_EN_WRITE_PROT_MASK));

    if (NULL == charMask)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Fill mask */
    eCode = str2ui32(charMask, strlen(charMask), &mask, 16); /* Mask is in
     hex */
    ERR_CHECK(eCode);

    /* Put non nWRP bits to 0 */
    mask &= (FLASH_OPTCR_nWRP_Msk >> FLASH_OPTCR_nWRP_Pos);

    /* Unlock option byte configuration */
    if (HAL_FLASH_OB_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }
    /* Wait for past flash operations to be done */
    FLASH_WaitForLastOperation(50000U); /*!< 50 s as in other function
     references */

    /* Get option bytes */
    HAL_FLASHEx_OBGetConfig( &pOBInit);

    /* Want to edit WRP */
    pOBInit.OptionType = OPTIONBYTE_WRP;

    pOBInit.WRPSector = mask;

    /* Setup kind of change */
    pOBInit.WRPState = EnDis;

    /* Write new RWP state in the option bytes register */
    HAL_FLASHEx_OBProgram( &pOBInit);

    /* Process the change */
    HAL_FLASH_OB_Launch();

    /* Lock option byte configuration */
    HAL_FLASH_OB_Lock();

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

// \f - new page
/**
 * @brief   Returns bit array of sector write protection to the user.
 *          LSB corresponds to sector 0
 */
cbl_err_code_t cmd_get_write_prot (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    FLASH_OBProgramInitTypeDef OBInit;
    uint16_t invWRPSector;
    char buf[FLASH_SECTOR_TOTAL + 3] = { 0 };

    DEBUG("Started\r\n");

    /* Unlock option byte configuration */
    if (HAL_FLASH_OB_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }
    /* Get option bytes */
    HAL_FLASHEx_OBGetConfig( &OBInit);

    /* Lock option byte configuration */
    HAL_FLASH_OB_Lock();

    /* Invert WRPSector as we want 1 to represent protected */
    invWRPSector = (uint16_t) ~OBInit.WRPSector
            & (FLASH_OPTCR_nWRP_Msk >> FLASH_OPTCR_nWRP_Pos);

    /* Fill the buffer with binary data */
    ui2binstr(invWRPSector, buf, FLASH_SECTOR_TOTAL);

    /* Send response */
    eCode = send_to_host(buf, strlen(buf));
    return eCode;
}
/*** end of file ***/

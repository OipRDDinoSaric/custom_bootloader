/** @file cbl_cmds_opt_bytes.c
 *
 * @brief Function handles for handling option bytes
 */
#include "commands/cbl_cmds_opt_bytes.h"
#include "etc/cbl_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief   RDP - Read protection
 *              - Used to protect the software code stored in Flash memory.
 *              - Reference manual - p. 93 - Explanation of RDP
 */
cbl_err_code_t cmd_get_rdp_lvl (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char rdp_lvl[32];

    DEBUG("Started\r\n");

    hal_rdp_lvl_get(rdp_lvl, sizeof(rdp_lvl));

    /* Send response */
    eCode = hal_send_to_host(rdp_lvl, strlen(rdp_lvl));

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
cbl_err_code_t cmd_change_write_prot (parser_t * phPrsr, bool EnDis)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charMask = NULL;
    uint32_t mask = 0u;

    DEBUG("Started\r\n");

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

    eCode = hal_change_write_prot(mask, EnDis);
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
    char write_prot[15] = {0};

    DEBUG("Started\r\n");

    eCode = hal_write_prot_get(write_prot, sizeof(write_prot));
    ERR_CHECK(eCode);

    /* Send response */
    eCode = hal_send_to_host(write_prot, strlen(write_prot));

    return eCode;
}
/*** end of file ***/

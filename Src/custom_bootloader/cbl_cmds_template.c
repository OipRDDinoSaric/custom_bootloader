/** @file cbl_cmds_template.c
 *
 * @brief A description of the cmds category.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "cbl_cmds_template.h"

/**
 * @brief               This is a template function. User can call it
 *
 * @param phPrsr[in]    Parser handle containing parameters
 */
cbl_err_code_t cmd_template (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charTemp = NULL;
    char msg[] = "Template success";

    DEBUG("Started\r\n");

    /* Get the parameter value */
    charTemp = parser_get_val(phPrsr, TXT_PAR_TEMPLATE_PARAM1,
            strlen(TXT_PAR_TEMPLATE_PARAM1));
    if (NULL == charTemp)
    {
        return CBL_ERR_NEED_PARAM;
    }

    /* Check if it matches TXT_PAR_TEMPLATE_VAL1 */
    if (strncmp(charTemp, TXT_PAR_TEMPLATE_VAL1, strlen(TXT_PAR_TEMPLATE_VAL1))
            != 0)
    {
        /* Add error enumerator and handler in custom_bootloader.c & .h */
        return CBL_ERR_TEMP_NOT_VAL1;
    }


    /* Do function stuff */
    send_to_host(msg, strlen(msg));

    return eCode;
}

/*** end of file ***/

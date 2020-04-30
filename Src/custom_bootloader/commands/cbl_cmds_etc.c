/** @file cbl_cmds_etc.c
 *
 * @brief Commands that don't fall in no other category, but don't deserve their
 *        own file
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cbl_cmds_etc.h"

/**
 * @brief Returns chip ID to the host
 */
cbl_err_code_t cmd_cid (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char cid[14] = "0x", cidhelp[10];

    DEBUG("Started\r\n");

    /* Convert hex value to text */
    itoa((int)(DBGMCU->IDCODE & 0x00000FFF), cidhelp, 16);

    /* Add 0x to to beginning */
    strlcat(cid, cidhelp, 12);

    /* End with a new line */
    strlcat(cid, CRLF, 12);

    /* Send response */
    eCode = send_to_host(cid, strlen(cid));

    return eCode;
}

/**
 * @brief Makes a request from the system to exit the application
 */
cbl_err_code_t cmd_exit (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    DEBUG("Started\r\n");

    gIsExitReq = true;

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

/*** end of file ***/

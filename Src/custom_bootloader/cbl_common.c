/** @file cbl_common.c
 *
 * @brief File containing all function and variable definitions that are needed
 *        by function handlers and custom bootloader main file
 */
#include "cbl_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "crc.h"
#include "dma.h"
#include "usart.h"

/** Used as a counter in the interrupt routine */
volatile uint32_t gRxCmdCntr;
/** Used to signal an exit request to shell system */
bool gIsExitReq = false;

// \f - new page
/**
 * @brief           Parses a command into parser_t. Command's form is as
 *                  follows: somecmd pname1=pval1 pname2=pval2
 *
 * @note            This function is destructive to input cmd, as it replaces
 *                  all ' ' and '=' with NULL terminator and transform every
 *                  character to lower case
 *
 * @param cmd[in]   NULL terminated string containing command without CR LF,
 *                  parser changes it
 *
 * @param len[in]   Length of string contained in cmd
 *
 * @param p[out]    Function assumes empty parser p on entrance
 */
cbl_err_code_t parser_run (char * cmd, size_t len, parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *pSpa = NULL;
    char *pEqu = NULL;
    char *pLastChar = &cmd[len];
    uint8_t iii;

    /* Convert the string to lower case */
    strlwr(cmd);

    /* Find the first ' ' */
    pSpa = memchr(cmd, ' ', len);

    for (iii = 0u; iii < MAX_ARGS && pSpa != NULL; iii++)
    {
        /* Command name/value name ends with ' ', replace with '\0' */
        *pSpa = '\0';

        /* Find an end of the param name */
        pEqu = memchr(pSpa, '=', pLastChar - pSpa);

        if (NULL == pEqu)
        {
            /* Exit from the loop as there is no value for the argument */
            break;
        }

        /* Argument starts after ' ' */
        phPrsr->args[iii][ARG_NAME] = (pSpa + 1);
        /* Arguments end with '=', replace with '\0' */
        *pEqu = '\0';

        /* Parameter value starts after '=' */
        phPrsr->args[iii][ARG_VAL] = (pEqu + 1);

        /* Find the next space */
        pSpa = memchr(pEqu, ' ', pLastChar - pEqu);
    }

    phPrsr->cmd = cmd;
    phPrsr->len = len;
    phPrsr->numOfArgs = iii;

    return eCode;
}

// \f - new page
/**
 * @brief           Gets value from a parameter
 *
 * @param p         parser
 *
 * @param name      Name of a parameter
 *
 * @param lenName   Name length
 *
 * @return  Pointer to the value, when no parameter it returns NULL
 */
char *parser_get_val (parser_t * phPrsr, char * name, size_t lenName)
{
    size_t lenArgName;

    if (phPrsr == NULL || name == NULL || lenName == 0)
        return NULL;

    /* Walk through all the parameters */
    for (uint32_t iii = 0u; iii < phPrsr->numOfArgs; iii++)
    {
        /* Get the length of ith parameter */
        lenArgName = strlen(phPrsr->args[iii][ARG_NAME]);

        /* Check if ith parameter is the requested one */
        if (lenArgName == lenName
                && strncmp(phPrsr->args[iii][ARG_NAME], name, lenArgName) == 0)
        {
            return phPrsr->args[iii][ARG_VAL];
        }
    }

    /* No parameter with name 'name' found */
    return NULL;
}

// \f - new page
/**
 * @brief           Sends a message to the host over defined output
 *
 * @param buf[in]   Message to send
 *
 * @param len[in]   Length of buf
 */
cbl_err_code_t send_to_host (const char * buf, size_t len)
{
    if (HAL_UART_Transmit(pUARTCmd, (uint8_t *)buf, len, HAL_MAX_DELAY)
            == HAL_OK)
    {
        return CBL_ERR_OK;
    }
    else
    {
        return CBL_ERR_HAL_TX;
    }
}

/**
 * @brief Nonblocking receive of 'len' characters from host.
 *        HAL_UART_RxCpltCallbackUART is triggered when done
 */
cbl_err_code_t recv_from_host_start (uint8_t * buf, size_t len)
{
    if (HAL_UART_Receive_DMA(pUARTCmd, buf, len) == HAL_OK)
    {
        return CBL_ERR_OK;
    }
    else
    {
        return CBL_ERR_HAL_RX;
    }
}

/**
 * @brief Stops waiting of the command
 */
cbl_err_code_t recv_from_host_stop (void)
{
    if (HAL_UART_AbortReceive(pUARTCmd) == HAL_OK)
    {
        return CBL_ERR_OK;
    }
    else
    {
        return CBL_ERR_RX_ABORT;
    }
}

/**
 * @brief           Converts string containing only number (e.g. 0A3F or 0x0A3F)
 *                  to uint32_t
 *
 * @param s[in]     String to convert
 *
 * @param len[in]   Length of s
 *
 * @param num[out]  Output number
 *
 * @param base[in]  Base of digits string is written into, supported 10
 *                  or 16 only
 */
cbl_err_code_t str2ui32 (const char * str, size_t len, uint32_t * num,
        uint8_t base)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    eCode = verify_digits_only(str, len, base);
    ERR_CHECK(eCode);

    *num = strtoul(str, NULL, base);
    return eCode;
}

/**
 * @brief           Verify if the string contains only digit characters
 *                  (or x on index 1 for hex numbers). Supports base 16 and
 *                  base 10.
 *
 * @param str[in]   String to test
 *
 * @param len[in]   Length of str
 *
 * @param base[in]  Number base. Shall be 10 or 16.
 * @return
 */
cbl_err_code_t verify_digits_only (const char * str, size_t len, uint8_t base)
{
    size_t iii = len;

    /* If base is 16, index 1 'x' or 'X' then index 0 must be '0' */
    if (16 == base && (tolower(str[1]) == 'x') && '0' != str[0])
    {
        return CBL_ERR_1ST_NOT_ZERO;
    }
    while (iii)
    {
        --iii;
        if (10 == base)
        {
            if (isdigit(str[iii]) == 0)
            {
                return CBL_ERR_NOT_DIG;
            }
        }
        else if (16 == base)
        {
            /* Index 1 can be a number or 'x' or 'X', other just number */
            if ((iii != 1 || (tolower(str[iii]) != 'x'))
                    && (isxdigit(str[iii]) == 0))
            {
                return CBL_ERR_NOT_DIG;
            }
        }
        else
        {
            return CBL_ERR_UNSUP_BASE;
        }
    }
    return CBL_ERR_OK;
}

// \f - new page
/**
 * @brief   Verifies address is in jumpable region
 *
 * @note    Jumping to peripheral memory locations is NOT permitted
 */
cbl_err_code_t verify_jump_address (uint32_t addr)
{
    cbl_err_code_t eCode = CBL_ERR_JUMP_INV_ADDR;

    if (IS_FLASH_ADDRESS(addr) || IS_CCMDATARAM_ADDRESS(addr)
    || IS_SRAM1_ADDRESS(addr) || IS_SRAM2_ADDRESS(addr)
    || IS_BKPSRAM_ADDRESS(addr) || IS_SYSMEM_ADDRESS(addr))
    {
        eCode = CBL_ERR_OK;
    }

    return eCode;
}

/**
 * @brief               Convert uint32_t to binary string
 *
 * @param str[out]      User must ensure it is at least
 *                      'numofbits' + 3 bytes long
 *
 * @param numofbits[in] Number of bits from num to convert to str
 */
void ui2binstr (uint32_t num, char * str, uint8_t numofbits)
{
    bool bit;
    char iii;

    /* Set num of bits to walk through */
    iii = numofbits;

    *str++ = '0';
    *str++ = 'b';

    do
    {
        iii--;
        /* exclude only ith bit */
        bit = (num >> iii) & 0x0001;

        /* Convert to ASCII value and save*/
        *str++ = (char)bit + '0';
    }
    while (iii);

    *str = '\0';
}

/*** end of file ***/


/**
 * @file custom_bootloader.c
 *
 * @brief   Custom bootloader for STM32F4 Disc1 development board with
 *          STM32F407. Uses UART for communcation
 *
 * @note    Written according to BARR-C:2018 coding standard
 *          Exceptions:
 *              - 3.2 a, c  - Eclipse formater doesn't support
 *              - 6.3 b iv. - ERR_CHECK() macro has return keyword
 *              - 7.1 f     - Variables contain uppercase letters!
 *              - 7.1 h     - Uppercase letter can seperate words
 *              - 7.1 m     - Boolean begins with is, e.g. isExample
 */
#include "custom_bootloader.h"
#include <cbl_common.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"
#include "crc.h"
#include "dma.h"
/* Include what command types you want to support */
#if true
#include "cbl_cmds_memory.h"
#endif
#if true
#include "cbl_cmds_opt_bytes.h"
#endif
#if false
#include "cbl_cmds_template.h"
#endif

#define CMD_BUF_SZ 128 /*!< Size of a new command buffer */

#define TXT_CMD_VERSION "version"
#define TXT_CMD_HELP "help"
#define TXT_CMD_CID "cid"
#define TXT_CMD_EXIT "exit"

typedef enum
{
    STATE_OPER, /*!< Operational state */
    STATE_ERR, /*!< Error state */
    STATE_EXIT /*!< Deconstructor state */
} sys_states_t;

typedef enum
{
    CMD_UNDEF = 0,
    CMD_VERSION,
    CMD_HELP,
    CMD_CID,
    CMD_GET_RDP_LVL,
    CMD_JUMP_TO,
    CMD_FLASH_ERASE,
    CMD_EN_WRITE_PROT,
    CMD_DIS_WRITE_PROT,
    CMD_READ_SECT_PROT_STAT,
    CMD_MEM_READ,
    CMD_FLASH_WRITE,
    CMD_EXIT,
    CMD_TEMPLATE
} cmd_t;

static bool gIsExitReq = false;

static void shell_init (void);
static void go_to_user_app (void);
static cbl_err_code_t run_shell_system (void);
static cbl_err_code_t sys_state_operation (void);
static cbl_err_code_t wait_for_cmd (char * buf, size_t len);
static cbl_err_code_t enum_cmd (char * buf, size_t len, cmd_t * pCmdCode);
static cbl_err_code_t handle_cmd (cmd_t cmdCode, parser_t * phPrsr);
static cbl_err_code_t sys_state_error (cbl_err_code_t eCode);
static cbl_err_code_t cmd_version (parser_t * phPrsr);
static cbl_err_code_t cmd_help (parser_t * phPrsr);
static cbl_err_code_t cmd_cid (parser_t * phPrsr);
static cbl_err_code_t cmd_exit (parser_t * phPrsr);

// \f - new page
/**
 * @brief   Gives control to the bootloader
 */
void CBL_Run ()
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    INFO("Custom bootloader started\r\n");
    if (HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
    {
        INFO("Blue button pressed...\r\n");
    }
    else
    {
        INFO("Blue button not pressed...\r\n");
        eCode = run_shell_system();
    }
    ASSERT(CBL_ERR_OK == eCode, "ErrCode=%d:Restart the application.\r\n",
            eCode);
    go_to_user_app();
    ERROR("Switching to user application failed\r\n");
}

// \f - new page

/**
 * @brief   Notifies the user that bootloader started and initializes
 *          peripherals
 */
static void shell_init (void)
{
    char bufWelcome[] = ""
            "\r\n*********************************************\r\n"
            "Custom bootloader for STM32F4 Discovery board\r\n"
            "*********************************************\r\n"
            "*********************************************\r\n"
            "                     "
    CBL_VERSION
    "                     \r\n"
    "*********************************************\r\n"
    "               Master's thesis               \r\n"
    "                  Dino Saric                 \r\n"
    "            University of Zagreb             \r\n"
    "                     2020                    \r\n"
    "*********************************************\r\n"
    "          If confused type \"help\"          \r\n"
    "*********************************************\r\n";
    MX_DMA_Init();
    MX_USART2_UART_Init();
    send_to_host(bufWelcome, strlen(bufWelcome));

    UNUSED( &stop_recv_from_host);

    /* Bootloader started turn on red LED */
    LED_ON(RED);
}

// \f - new page
/**
 * @brief   Gives controler to the user application
 *          Steps:
 *          1)  Set the main stack pointer (MSP) to the one of the user
 *              application. User application MSP is contained in the first
 *              four bytes of flashed user application.
 *
 *          2)  Set the reset handler to the one of the user application.
 *              User application reset handler is right after the MSP, size of
 *              4 bytes.
 *
 *          3)  Jump to the user application reset handler. Giving control to
 *              the user application.
 *
 * @attention   DO NOT FORGET: In user application VECT_TAB_OFFSET set to the
 *          offset of user application from the start of the flash.
 *          e.g. If our application starts in the 2nd sector we would write
 *          #define VECT_TAB_OFFSET 0x8000.
 *          VECT_TAB_OFFSET is located in system_stm32f4xx.c
 *
 * @return  Procesor never returns from this application
 */
static void go_to_user_app (void)
{
    void (*pUserAppResetHandler) (void);
    uint32_t addressRstHndl;
    volatile uint32_t msp_value = *(volatile uint32_t *)CBL_ADDR_USERAPP;

    char userAppHello[] = "Jumping to user application :)\r\n";

    /* Send hello message to user and debug output */
    send_to_host(userAppHello, strlen(userAppHello));
    INFO("%s", userAppHello);

    addressRstHndl = *(volatile uint32_t *)(CBL_ADDR_USERAPP + 4u);

    pUserAppResetHandler = (void *)addressRstHndl;

    /* WARNING: 32-bit assumed */
    DEBUG("MSP value: %#x\r\n", (unsigned int ) msp_value);
    DEBUG("Reset handler address: %#x\r\n", (unsigned int ) addressRstHndl);

    /* Function from CMSIS */
    __set_MSP(msp_value);

    /* Give control to user application */
    pUserAppResetHandler();

    /* Never reached */
}

// \f - new page
/**
 * @brief   Runs the shell for the bootloader until unrecoverable error happens
 *          or exit is requested
 *
 * @return  CBL_ERR_NO when no error, else returns an error code.
 */
static cbl_err_code_t run_shell_system (void)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    bool isExitNeeded = false;
    sys_states_t state = STATE_ERR;
    sys_states_t nextState = state;
    INFO("Starting bootloader\r\n");

    shell_init();

    while (false == isExitNeeded)
    {
        switch (state)
        {
            case STATE_OPER:
            {
                eCode = sys_state_operation();

                /* Switch state if needed */
                if (eCode != CBL_ERR_OK)
                {
                    nextState = STATE_ERR;
                }
                else if (true == gIsExitReq)
                {
                    nextState = STATE_EXIT;
                }
                else
                {
                    /* Dont change state */
                }
            }
            break;

            case STATE_ERR:
            {
                eCode = sys_state_error(eCode);

                /* Switch state */
                if (eCode != CBL_ERR_OK)
                {
                    nextState = STATE_EXIT;
                }
                else
                {
                    nextState = STATE_OPER;
                }
            }
            break;

            case STATE_EXIT:
            {
                /* Deconstructor */
                char bye[] = "Exiting\r\n\r\n";

                INFO(bye);
                send_to_host(bye, strlen(bye));

                isExitNeeded = true;
            }
            break;

            default:
            {
                eCode = CBL_ERR_STATE;
            }
            break;

        }
        state = nextState;
    }
    /* Botloader done, turn off red LED */
    LED_OFF(RED);

    return eCode;
}

// \f - new page
/**
 * @brief   Function that runs in normal operation, waits for new command from
 *          the host and processes it
 */
static cbl_err_code_t sys_state_operation (void)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    cmd_t cmdCode = CMD_UNDEF;
    parser_t parser = { 0 };
    char cmd[CMD_BUF_SZ] = { 0 };

    LED_ON(GREEN);
    eCode = wait_for_cmd(cmd, CMD_BUF_SZ);
    ERR_CHECK(eCode);
    LED_OFF(GREEN);

    /* Command processing, turn on orange LED */
    LED_ON(ORANGE);
    eCode = parser_run(cmd, strlen(cmd), &parser);
    ERR_CHECK(eCode);

    eCode = enum_cmd(parser.cmd, strlen(cmd), &cmdCode);
    ERR_CHECK(eCode);

    eCode = handle_cmd(cmdCode, &parser);
    /* Command processing success, turn off orange LED */
    LED_OFF(ORANGE);
    return eCode;
}

// \f - new page
/**
 * @brief           Block thread until new command is received from host.
 *                  New command is considered received when CR LF is received
 *                  or buffer for command overflows
 *
 * @param buf[out]  Buffer for command
 *
 * @param len[in]   Length of buf
 */
static cbl_err_code_t wait_for_cmd (char * buf, size_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    bool isLastCharCR = false;
    bool isOverflow = true;
    uint32_t iii = 0u;
    gRxCmdCntr = 0u;

    eCode = send_to_host("\r\n> ", 4);
    ERR_CHECK(eCode);

    /* Read until CRLF or until full DMA */
    while (gRxCmdCntr < len)
    {
        /* Receive one char from host */
        eCode = recv_from_host(buf + iii, 1);
        ERR_CHECK(eCode);

        while (iii != (gRxCmdCntr - 1))
        {
            /* Wait for a new char */
        }

        if (true == isLastCharCR)
        {
            if ('\n' == buf[iii])
            {
                /* CRLF was received, command done */
                /* Replace '\r' with '\0' to make sure every char array ends
                 * with '\0' */
                buf[iii - 1] = '\0';
                isOverflow = false;
                break;
            }
        }

        /* Update isLastCharCR */
        isLastCharCR = '\r' == buf[iii] ? true : false;

        /* Prepare for next char */
        iii++;
    }
    /* If DMA fills the buffer and no CRLF is received throw an error */
    if (true == isOverflow)
    {
        eCode = CBL_ERR_READ_OF;
    }

    /* If no error buf has a new command to process */
    return eCode;
}

// \f - new page
/**
 * @brief               Enumerates the buf for command
 *
 * @param buf[in]       Buffer for command
 *
 * @param len[in]       Length of buffer
 *
 * @param pCmdCode[out] Pointer to command code enum
 *
 * @return              Error status
 */
static cbl_err_code_t enum_cmd (char * buf, size_t len, cmd_t * pCmdCode)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    if (0u == len)
    {
        eCode = CBL_ERR_CMD_SHORT;
    }
    else if (len == strlen(TXT_CMD_VERSION)
            && strncmp(buf, TXT_CMD_VERSION, strlen(TXT_CMD_VERSION)) == 0)
    {
        *pCmdCode = CMD_VERSION;
    }
    else if (len == strlen(TXT_CMD_HELP)
            && strncmp(buf, TXT_CMD_HELP, strlen(TXT_CMD_HELP)) == 0)
    {
        *pCmdCode = CMD_HELP;
    }
    else if (len == strlen(TXT_CMD_CID)
            && strncmp(buf, TXT_CMD_CID, strlen(TXT_CMD_CID)) == 0)
    {
        *pCmdCode = CMD_CID;
    }
    else if (len == strlen(TXT_CMD_EXIT)
            && strncmp(buf, TXT_CMD_EXIT, strlen(TXT_CMD_EXIT)) == 0)
    {
        *pCmdCode = CMD_EXIT;
    }
#ifdef CBL_CMDS_OPT_BYTES_H
    else if (len == strlen(TXT_CMD_GET_RDP_LVL)
            && strncmp(buf, TXT_CMD_GET_RDP_LVL, strlen(TXT_CMD_GET_RDP_LVL))
                    == 0)
    {
        *pCmdCode = CMD_GET_RDP_LVL;
    }
    else if (len == strlen(TXT_CMD_EN_WRITE_PROT)
            && strncmp(buf, TXT_CMD_EN_WRITE_PROT,
                    strlen(TXT_CMD_EN_WRITE_PROT)) == 0)
    {
        *pCmdCode = CMD_EN_WRITE_PROT;
    }
    else if (len == strlen(TXT_CMD_DIS_WRITE_PROT)
            && strncmp(buf, TXT_CMD_DIS_WRITE_PROT,
                    strlen(TXT_CMD_DIS_WRITE_PROT)) == 0)
    {
        *pCmdCode = CMD_DIS_WRITE_PROT;
    }
    else if (len == strlen(TXT_CMD_READ_SECT_PROT_STAT)
            && strncmp(buf, TXT_CMD_READ_SECT_PROT_STAT,
                    strlen(TXT_CMD_READ_SECT_PROT_STAT)) == 0)
    {
        *pCmdCode = CMD_READ_SECT_PROT_STAT;
    }
#endif /* CBL_CMDS_OPT_BYTES_H */
#ifdef CBL_CMDS_MEMORY_H
    else if (len == strlen(TXT_CMD_JUMP_TO)
            && strncmp(buf, TXT_CMD_JUMP_TO, strlen(TXT_CMD_JUMP_TO)) == 0)
    {
        *pCmdCode = CMD_JUMP_TO;
    }
    else if (len == strlen(TXT_CMD_FLASH_ERASE)
            && strncmp(buf, TXT_CMD_FLASH_ERASE, strlen(TXT_CMD_FLASH_ERASE))
                    == 0)
    {
        *pCmdCode = CMD_FLASH_ERASE;
    }

    else if (len == strlen(TXT_CMD_MEM_READ)
            && strncmp(buf, TXT_CMD_MEM_READ, strlen(TXT_CMD_MEM_READ)) == 0)
    {
        *pCmdCode = CMD_MEM_READ;
    }
    else if (len == strlen(TXT_CMD_FLASH_WRITE)
            && strncmp(buf, TXT_CMD_FLASH_WRITE, strlen(TXT_CMD_FLASH_WRITE))
                    == 0)
    {
        *pCmdCode = CMD_FLASH_WRITE;
    }
#endif /* CBL_CMDS_MEMORY_H */
#ifdef CBL_CMDS_TEMPLATE_H
    /* Add a new enum value in cmd_t and check for it here */
    else if (len == strlen(TXT_CMD_TEMPLATE)
            && strncmp(buf, TXT_CMD_TEMPLATE, strlen(TXT_CMD_TEMPLATE)) == 0)
    {
        *pCmdCode = CMD_TEMPLATE;
    }
#endif /* CBL_CMDS_TEMPLATE_H */
    else
    {
        *pCmdCode = CMD_UNDEF;
    }

    if (CBL_ERR_OK == eCode && CMD_UNDEF == *pCmdCode)
    {
        eCode = CBL_ERR_CMD_UNDEF;
    }

    return eCode;
}

// \f - new page
/**
 * @brief               Handler for all defined commands
 *
 * @param cmdCode[in]   Enumerator for the commands
 *
 * @param phPrsr[in]    Handle of the parser containing parameters
 */
static cbl_err_code_t handle_cmd (cmd_t cmdCode, parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cmdCode)
    {
        case CMD_VERSION:
        {
            eCode = cmd_version(phPrsr);
        }
        break;

        case CMD_HELP:
        {
            eCode = cmd_help(phPrsr);
        }
        break;

        case CMD_CID:
        {
            eCode = cmd_cid(phPrsr);
        }
        break;

        case CMD_EXIT:
        {
            eCode = cmd_exit(phPrsr);
        }
        break;

#ifdef CBL_CMDS_OPT_BYTES_H
        case CMD_GET_RDP_LVL:
        {
            eCode = cmd_get_rdp_lvl(phPrsr);
        }
        break;

        case CMD_EN_WRITE_PROT:
        {
            eCode = cmd_change_write_prot(phPrsr, OB_WRPSTATE_ENABLE);
        }
        break;

        case CMD_DIS_WRITE_PROT:
        {
            eCode = cmd_change_write_prot(phPrsr, OB_WRPSTATE_DISABLE);
        }
        break;

        case CMD_READ_SECT_PROT_STAT:
        {
            eCode = cmd_get_write_prot(phPrsr);
        }
        break;
#endif /* CBL_CMDS_OPT_BYTES_H */
#ifdef CBL_CMDS_MEMORY_H
        case CMD_JUMP_TO:
        {
            eCode = cmd_jump_to(phPrsr);
        }
        break;

        case CMD_FLASH_ERASE:
        {
            eCode = cmd_flash_erase(phPrsr);
        }
        break;

        case CMD_MEM_READ:
        {
            eCode = cmd_mem_read(phPrsr);
        }
        break;

        case CMD_FLASH_WRITE:
        {
            eCode = cmd_flash_write(phPrsr);
        }
        break;
#endif /* CBL_CMDS_MEMORY_H */
#ifdef CBL_CMDS_TEMPLATE_H
            /* Add a new case for the enumerator and call the function handler */
        case CMD_TEMPLATE:
        {
            eCode = cmd_template(phPrsr);
        }
        break;
#endif /* CBL_CMDS_TEMPLATE_H */

        case CMD_UNDEF:
            /* No break */
        default:
        {
            eCode = CBL_ERR_CMDCD;
        }
        break;
    }

    DEBUG("Responded\r\n");
    return eCode;
}

// \f - new page
/**
 * @brief           Handler for all errors
 *
 * @param eCode[in] Error code that happened
 */
static cbl_err_code_t sys_state_error (cbl_err_code_t eCode)
{
    DEBUG("Started\r\n");

    /* Turn off all LEDs except red */
    LED_OFF(ORANGE);
    LED_OFF(BLUE);
    LED_OFF(GREEN);

    switch (eCode)
    {
        case CBL_ERR_OK:
            /* FALSE ALARM - no error */
        break;

        case CBL_ERR_READ_OF:
        {
            char msg[] = "\r\nERROR: Command too long\r\n";
            WARNING("Overflow while reading happened\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_WRITE:
        {
            WARNING("Error occurred while writing\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_STATE:
        {
            WARNING("System entered unknown state, "
                    "returning to operational\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_TX:
        {
            WARNING("HAL transmit error happened\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_RX:
        {
            WARNING("HAL receive error happened\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_RX_ABORT:
        {
            WARNING("Error happened while aborting receive\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_CMD_SHORT:
        {
            INFO("Client sent an empty command\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_CMD_UNDEF:
        {
            char msg[] = "\r\nERROR: Invalid command\r\n";
            INFO("Client sent an invalid command\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_NEED_PARAM:
        {
            char msg[] = "\r\nERROR: Missing parameter(s)\r\n";
            INFO("Command is missing parameter(s)");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_JUMP_INV_ADDR:
        {
            char msg[] = "\r\nERROR: Invalid address\r\n"
                    "Jumpable regions: FLASH, SRAM1, SRAM2, CCMRAM, "
                    "BKPSRAM, SYSMEM and EXTMEM (if connected)\r\n";

            INFO("Invalid address inputed for jumping\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_SECTOR:
        {
            char msg[] = "\r\nERROR: Internal error while erasing sectors\r\n";

            WARNING("Error while erasing sectors\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_INV_SECT:
        {
            char msg[] = "\r\nERROR: Wrong sector given\r\n";

            INFO("Wrong sector given\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_INV_SECT_COUNT:
        {
            char msg[] = "\r\nERROR: Wrong sector count given\r\n";

            INFO("Wrong sector count given\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_WRITE_INV_ADDR:
        {
            char msg[] = "\r\nERROR: Invalid address range entered\r\n";

            INFO("Invalid address range entered for writing\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_WRITE_TOO_BIG:
        {
            char msg[] = "\r\nERROR: Inputed too big value\r\n";

            INFO("User requested to write a too big chunk\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_WRITE:
        {
            char msg[] = "\r\nERROR: Error while writing to flash."
                    " Retry last message.\r\n";

            INFO("Error while writing to flash on HAL level\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_ERASE_INV_TYPE:
        {
            char msg[] = "\r\nERROR: Invalid erase type\r\n";

            INFO("User entered invalid erase type\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_ERASE:
        {
            char msg[] = "\r\nERROR: HAL error while erasing sectors \r\n";

            INFO("HAL error while erasing sector\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_UNLOCK:
        {
            char msg[] = "\r\nERROR: Unlocking flash failed\r\n";

            WARNING("Unlocking flash with HAL failed\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_INV_PARAM:
        {
            ERROR("Wrong parameter sent to a function\r\n");
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_NOT_DIG:
        {
            char msg[] = "\r\nERROR: Number parameter contains letters\r\n";

            WARNING("User entered number parameter containing letters\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_1ST_NOT_ZERO:
        {
            char msg[] =
                    "\r\nERROR: Number parameter must have '0' at the start "
                            " when 'x' is present\r\n";

            WARNING("User entered number parameter with 'x', "
                    "but not '0' on index 0\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_CRC_WRONG:
        {
            char msg[] =
                    "\r\nERROR: Data corrupted during transport (Invalid CRC)."
                            " Retry last message.\r\n";

            WARNING("Data corrupted during transport, invalid CRC\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_TEMP_NOT_VAL1:
        {
            char msg[] = "\r\nERROR: Value for parameter invalid...\r\n";

            WARNING("User entered wrong param. value in template function\r\n");
            send_to_host(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        default:
        {
            ERROR("Unhandled error happened\r\n")
            ;
        }
        break;

    }

    return eCode;
}

/**
 * @brief           Interrupt when receiveing is done from UART
 *
 * @param huart[in] Handle for UART that triggered the interrupt
 */
void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart)
{
    if (huart == pUARTCmd)
    {
        gRxCmdCntr++;
    }
}

// \f - new page
/*********************************************************/
/* Fundamental function handles */
/*********************************************************/

/**
 * @brief Gets a version of the bootloader
 */
static cbl_err_code_t cmd_version (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char verbuf[12] = CBL_VERSION;

    DEBUG("Started\r\n");

    /* End with a new line */
    strlcat(verbuf, CRLF, 12);

    /* Send response */
    eCode = send_to_host(verbuf, strlen(verbuf));

    return eCode;
}

// \f - new page
/**
 * @brief Returns string to the host of all commands
 */
static cbl_err_code_t cmd_help (parser_t * pphPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    const char helpPrintout[] =
            "*************************************************************" CRLF
            "*************************************************************" CRLF
            "Custom STM32F4 bootloader shell by Dino Saric - " CBL_VERSION "***"
            "******" CRLF
            "*************************************************************" CRLF
            CRLF
            "*************************************************************" CRLF
            "Commands*****************************************************" CRLF
            "*************************************************************" CRLF
            CRLF
            "Optional parameters are surrounded with [] " CRLF CRLF
            "- " TXT_CMD_VERSION " | Gets the current version of the running "
            "bootloader" CRLF CRLF
            "- " TXT_CMD_HELP " | Makes life easier" CRLF CRLF
            "- " TXT_CMD_CID " | Gets chip identification number" CRLF CRLF
#ifdef CBL_CMDS_OPT_BYTES_H
            "- "
            TXT_CMD_GET_RDP_LVL
            " |  Read protection. Used to protect the"
            " software code stored in Flash memory."
            " Ref. man. p. 93" CRLF CRLF
            "- "
            TXT_CMD_EN_WRITE_PROT
            " | Enables write protection per sector,"
            " as selected with \""
            TXT_PAR_EN_WRITE_PROT_MASK
            "\"." CRLF
            "     "
            TXT_PAR_EN_WRITE_PROT_MASK
            " - Mask in hex form for sectors"
            " where LSB corresponds to sector 0." CRLF CRLF
            "- "
            TXT_CMD_DIS_WRITE_PROT
            " | Disables write protection per "
            "sector, as selected with \""
            TXT_PAR_EN_WRITE_PROT_MASK
            "\"." CRLF
            "     "
            TXT_PAR_EN_WRITE_PROT_MASK
            " - Mask in hex form for sectors"
            " where LSB corresponds to sector 0." CRLF CRLF
            "- "
            TXT_CMD_READ_SECT_PROT_STAT
            " | Returns bit array of sector "
            "write protection. LSB corresponds to sector 0. " CRLF CRLF
#endif /* CBL_CMDS_OPT_BYTES_H */
#ifdef CBL_CMDS_MEMORY_H
            "- " TXT_CMD_JUMP_TO
            " | Jumps to a requested address" CRLF
            "    "
            TXT_PAR_JUMP_TO_ADDR
            " - Address to jump to in hex format "
            "(e.g. 0x12345678), 0x can be omitted. " CRLF CRLF
            "- " TXT_CMD_FLASH_ERASE
            " | Erases flash memory" CRLF "    " TXT_PAR_FLASH_ERASE_TYPE
            " - Defines type of flash erase." " \""
            TXT_PAR_FLASH_ERASE_TYPE_MASS "\" erases all sectors, \""
            TXT_PAR_FLASH_ERASE_TYPE_SECT "\" erases only selected sectors."
            CRLF "    " TXT_PAR_FLASH_ERASE_SECT " - First sector to erase. "
            "Bootloader is on sectors 0, 1 and 2. Not needed with mass erase."
            CRLF "    " TXT_PAR_FLASH_ERASE_COUNT
            " - Number of sectors to erase. Not needed with mass erase." CRLF
            CRLF "- " TXT_CMD_FLASH_WRITE " | Writes to flash, returns "
            TXT_RESP_FLASH_WRITE_READY_HELP " when ready to receive bytes." CRLF
            "     " TXT_PAR_FLASH_WRITE_START " - Starting address in hex "
            "format (e.g. 0x12345678), 0x can be omitted."CRLF
            "     " TXT_PAR_FLASH_WRITE_COUNT " - Number of bytes to write. "
            "Maximum bytes: "
            TXT_FLASH_WRITE_SZ
            CRLF CRLF
            "- "
            TXT_CMD_MEM_READ
            " | Read bytes from memory" CRLF
            "     "
            TXT_PAR_FLASH_WRITE_START
            " - Starting address in hex "
            "format (e.g. 0x12345678), 0x can be omitted."CRLF
            "     "
            TXT_PAR_FLASH_WRITE_COUNT
            " - Number of bytes to read."
            CRLF CRLF
#endif /* CBL_CMDS_MEMORY_H */
#ifdef CBL_CMDS_TEMPLATE_H
            /* Add a description of newly added command */
            TXT_CMD_TEMPLATE " | Explanation of function" CRLF
            "     " TXT_PAR_TEMPLATE_PARAM1 " - Example param, valid value is: "
            TXT_PAR_TEMPLATE_VAL1 CRLF CRLF
#endif /* CBL_CMDS_TEMPLATE_H */
            "- "TXT_CMD_EXIT " | Exits the bootloader and starts the user "
            "application" CRLF CRLF
            "********************************************************" CRLF
            "Examples are contained in README.md" CRLF
            "********************************************************" CRLF
            CRLF;
    DEBUG("Started\r\n");

    /* Send response */
    eCode = send_to_host(helpPrintout, strlen(helpPrintout));

    return eCode;
}

// \f - new page
/**
 * @brief Returns chip ID to the host
 */
static cbl_err_code_t cmd_cid (parser_t * phPrsr)
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

// \f - new page
/**
 * @brief Makes a request from the system to exit the application
 */
static cbl_err_code_t cmd_exit (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    DEBUG("Started\r\n");

    gIsExitReq = true;

    /* Send response */
    eCode = send_to_host(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

/****END OF FILE****/

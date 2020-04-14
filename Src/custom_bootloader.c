/**
 * @file custom_bootloader.c
 *
 * @brief   Custom bootloader for STM32F4 Disc1 development board with
 *          STM32F407. Uses UART for communcation
 *
 * @note    Written according to BARR-C:2018 coding standard
 *          Exceptions:
 *              - 3.2 a, c  - Eclipse formater doesn't support
 *              - 6.1 e     - Functions can contain uppercase letter!
 *              - 6.1 g     - Upercase letter can separate words
 *              - 6.3 b iv. - ERR_CHECK() macro has return keyword
 *              - 7.1 f     - Variables contain uppercase letters!
 *              - 7.1 h     - Uppercase letter can seperate words
 *              - 7.1 m     - boolean begins with is, e.g. isExample
 *
 */

#include "custom_bootloader.h"

#define TXT_FLASH_WRITE_SZ "1024" /*!< Size of a buffer used to write to flash
                                  as char array */
#define FLASH_WRITE_SZ 1024 /*!< Size of a buffer used to write to flash */

#define CMD_BUF_SZ 128 /*!< Size of a new command buffer */
#define MAX_ARGS 8 /*!< Maximum number of arguments in an input cmd */

#define CRLF "\r\n"

#define TXT_SUCCESS "\r\nOK\r\n"
#define TXT_SUCCESS_HELP "\\r\\nOK\\r\\n" /*!< Used in help function */

#define TXT_CMD_VERSION "version"
#define TXT_CMD_HELP "help"
#define TXT_CMD_CID "cid"
#define TXT_CMD_GET_RDP_LVL "get-rdp-level"
#define TXT_CMD_JUMP_TO "jump-to"
#define TXT_CMD_FLASH_ERASE "flash-erase"
#define TXT_CMD_FLASH_WRITE "flash-write"
#define TXT_CMD_MEM_READ "mem-read"
#define TXT_CMD_EN_WRITE_PROT "en-write-prot"
#define TXT_CMD_DIS_WRITE_PROT "dis-write-prot"
#define TXT_CMD_READ_SECT_PROT_STAT "get-write-prot"
#define TXT_CMD_EXIT "exit"

#define TXT_PAR_JUMP_TO_ADDR "addr"

#define TXT_PAR_FLASH_WRITE_START "start"
#define TXT_PAR_FLASH_WRITE_COUNT "count"

#define TXT_PAR_FLASH_ERASE_TYPE "type"
#define TXT_PAR_FLASH_ERASE_SECT "sector"
#define TXT_PAR_FLASH_ERASE_COUNT "count"
#define TXT_PAR_FLASH_ERASE_TYPE_MASS "mass"
#define TXT_PAR_FLASH_ERASE_TYPE_SECT "sector"

#define TXT_PAR_EN_WRITE_PROT_MASK "mask"

#define TXT_RESP_FLASH_WRITE_READY "\r\nready\r\n"
#define TXT_RESP_FLASH_WRITE_READY_HELP "\\r\\nready\\r\\n" /*!< Used in help
                                                                    function */

typedef enum
{
    ARG_NAME = 0, //!< ARG_NAME
    ARG_VAL = 1, //!< ARG_VAL
    ARG_MAX = 2  //!< ARG_MAX
} cmd_arg_t;

typedef struct
{
    char *cmd; /*!< Command buffer */
    size_t len; /*!< length of the whole cmd string */
    char *args[MAX_ARGS][ARG_MAX]; /*!< Pointers to buffers holding name and
     value of an argument */
    uint8_t numOfArgs;
} parser_t;

typedef enum
{
    STATE_OPER, /*!< Operational state */
    STATE_ERR, /*!< Error state */
    STAT_EXIT /*!< Deconstructor state */
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
    CMD_EXIT
} cmd_t;

/** Used as a counter in the interrupt routine */
static volatile uint32_t gRxCmdCntr = 0u;

static bool gIsExitReq = false;

static void shellInit (void);
static void runUserApp (void);
static cbl_err_code_t runShellSystem (void);
static cbl_err_code_t sysState_Operation (void);
static cbl_err_code_t waitForCmd (char * buf, size_t len);
static cbl_err_code_t parser_Run (char * cmd, size_t len, parser_t * phPrsr);
static char *parser_GetVal (parser_t * phPrsr, char * name, size_t lenName);
static cbl_err_code_t enumCmd (char * buf, size_t len, cmd_t * pCmdCode);
static cbl_err_code_t handleCmd (cmd_t cmdCode, parser_t * phPrsr);
static cbl_err_code_t sendToHost (const char * buf, size_t len);
static cbl_err_code_t recvFromHost (char * buf, size_t len);
static cbl_err_code_t stopRecvFromHost (void);
static cbl_err_code_t sysState_Error (cbl_err_code_t eCode);
static cbl_err_code_t cmdHandle_Version (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_Help (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_Cid (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_GetRDPLvl (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_JumpTo (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_FlashErase (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_ChangeWriteProt (parser_t * phPrsr,
        uint32_t EnDis);
static cbl_err_code_t cmdHandle_MemRead (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_GetWriteProt (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_FlashWrite (parser_t * phPrsr);
static cbl_err_code_t cmdHandle_Exit (parser_t * phPrsr);
static cbl_err_code_t str2ui32 (const char * str, size_t len, uint32_t * num,
        uint8_t base);
static cbl_err_code_t verifyDigitsOnly (const char * str, size_t i,
        uint8_t base);
static cbl_err_code_t verifyJumpAddress (uint32_t addr);
static void ui2binstr (uint32_t num, char * str, uint8_t numofbits);

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
        eCode = runShellSystem();
    }
    ASSERT(CBL_ERR_OK == eCode, "ErrCode=%d:Restart the application.\r\n",
            eCode);
    runUserApp();
    ERROR("Switching to user application failed\r\n");
}

// \f - new page
static void shellInit (void)
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
    sendToHost(bufWelcome, strlen(bufWelcome));

    UNUSED( &stopRecvFromHost);

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
 *          offset of user application
 *          from the start of the flash.
 *          e.g. If our application starts in the 2nd sector we would write
 *          #define VECT_TAB_OFFSET 0x8000.
 *          VECT_TAB_OFFSET is located in system_Stm32f4xx.c
 *
 * @return  Procesor never returns from this application
 */
static void runUserApp (void)
{
    void (*pUserAppResetHandler) (void);
    uint32_t addressRstHndl;
    volatile uint32_t msp_value = *(volatile uint32_t *)CBL_ADDR_USERAPP;

    char userAppHello[] = "Jumping to user application :)\r\n";

    /* Send hello message to user and debug output */
    sendToHost(userAppHello, strlen(userAppHello));
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
 * @brief   Runs the shell for the bootloader.
 * @return  CBL_ERR_NO when no error, else returns an error code.
 */
static cbl_err_code_t runShellSystem (void)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    bool isExitNeeded = false;
    sys_states_t state = STATE_ERR;
    sys_states_t nextState = state;
    INFO("Starting bootloader\r\n");

    shellInit();

    while (false == isExitNeeded)
    {
        switch (state)
        {
            case STATE_OPER:
            {
                eCode = sysState_Operation();

                /* Switch state if needed */
                if (eCode != CBL_ERR_OK)
                {
                    nextState = STATE_ERR;
                }
                else if (true == gIsExitReq)
                {
                    nextState = STAT_EXIT;
                }
                else
                {
                    /* Dont change state */
                }
            }
            break;

            case STATE_ERR:
            {
                eCode = sysState_Error(eCode);

                /* Switch state */
                if (eCode != CBL_ERR_OK)
                {
                    nextState = STAT_EXIT;
                }
                else
                {
                    nextState = STATE_OPER;
                }
            }
            break;

            case STAT_EXIT:
            {
                /* Deconstructor */
                char bye[] = "Exiting\r\n\r\n";

                INFO(bye);
                sendToHost(bye, strlen(bye));

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
static cbl_err_code_t sysState_Operation (void)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    cmd_t cmdCode = CMD_UNDEF;
    parser_t parser = { 0 };
    char cmd[CMD_BUF_SZ] = { 0 };

    LED_ON(GREEN);
    eCode = waitForCmd(cmd, CMD_BUF_SZ);
    ERR_CHECK(eCode);
    LED_OFF(GREEN);

    /* Command processing, turn on orange LED */
    LED_ON(ORANGE);
    eCode = parser_Run(cmd, strlen(cmd), &parser);
    ERR_CHECK(eCode);

    eCode = enumCmd(parser.cmd, strlen(cmd), &cmdCode);
    ERR_CHECK(eCode);

    eCode = handleCmd(cmdCode, &parser);
    /* Command processing success, turn off orange LED */
    LED_OFF(ORANGE);
    return eCode;
}

// \f - new page
/**
 * @brief           Block thread until new command is received from host
 * @param buf[out]  Buffer for command
 * @param len[in]   Length of buf
 * @return
 */
static cbl_err_code_t waitForCmd (char * buf, size_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    bool isLastCharCR = false;
    bool isOverflow = true;
    uint32_t iii = 0u;
    gRxCmdCntr = 0u;

    eCode = sendToHost("\r\n> ", 4);
    ERR_CHECK(eCode);

    /* Read until CRLF or until full DMA */
    while (gRxCmdCntr < len)
    {
        /* Receive one char from host */
        eCode = recvFromHost(buf + iii, 1);
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
 * @brief           Parses a command into CBL_Parser_t. Command's form is as
 *                  follows: somecmd pname1=pval1 pname2=pval2
 *
 * @note            This function is destructive to input cmd, as it replaces
 *                  all ' ' and '='
 *                  with NULL terminator and transform all to lower case
 *
 * @param cmd[in]   NULL terminated string containing command without CR LF,
 *                  parser changes it to lower case
 *
 * @param len[in]   Length of string contained in cmd
 *
 * @param p[out]    Function assumes empty parser p on entrance
 *
 * @return
 */
static cbl_err_code_t parser_Run (char * cmd, size_t len, parser_t * phPrsr)
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
 * @param p         parser
 * @param name      Name of a parameter
 * @param lenName   Name length
 * @return  Pointer to the value
 */
static char *parser_GetVal (parser_t * phPrsr, char * name, size_t lenName)
{
    size_t lenArgName;

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
static cbl_err_code_t enumCmd (char * buf, size_t len, cmd_t * pCmdCode)
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
    else if (len == strlen(TXT_CMD_GET_RDP_LVL)
            && strncmp(buf, TXT_CMD_GET_RDP_LVL, strlen(TXT_CMD_GET_RDP_LVL))
                    == 0)
    {
        *pCmdCode = CMD_GET_RDP_LVL;
    }
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
    else if (len == strlen(TXT_CMD_MEM_READ)
            && strncmp(buf, TXT_CMD_MEM_READ, strlen(TXT_CMD_MEM_READ)) == 0)
    {
        *pCmdCode = CMD_MEM_READ;
    }
    else if (len == strlen(TXT_CMD_READ_SECT_PROT_STAT)
            && strncmp(buf, TXT_CMD_READ_SECT_PROT_STAT,
                    strlen(TXT_CMD_READ_SECT_PROT_STAT)) == 0)
    {
        *pCmdCode = CMD_READ_SECT_PROT_STAT;
    }
    else if (len == strlen(TXT_CMD_FLASH_WRITE)
            && strncmp(buf, TXT_CMD_FLASH_WRITE, strlen(TXT_CMD_FLASH_WRITE))
                    == 0)
    {
        *pCmdCode = CMD_FLASH_WRITE;
    }
    else if (len == strlen(TXT_CMD_EXIT)
            && strncmp(buf, TXT_CMD_EXIT, strlen(TXT_CMD_EXIT)) == 0)
    {
        *pCmdCode = CMD_EXIT;
    }

    if (CBL_ERR_OK == eCode && CMD_UNDEF == *pCmdCode)
        eCode = CBL_ERR_CMD_UNDEF;
    return eCode;
}

// \f - new page
static cbl_err_code_t handleCmd (cmd_t cmdCode, parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    switch (cmdCode)
    {
        case CMD_VERSION:
        {
            eCode = cmdHandle_Version(phPrsr);
        }
        break;

        case CMD_HELP:
        {
            eCode = cmdHandle_Help(phPrsr);
        }
        break;

        case CMD_CID:
        {
            eCode = cmdHandle_Cid(phPrsr);
        }
        break;

        case CMD_GET_RDP_LVL:
        {
            eCode = cmdHandle_GetRDPLvl(phPrsr);
        }
        break;

        case CMD_JUMP_TO:
        {
            eCode = cmdHandle_JumpTo(phPrsr);
        }
        break;

        case CMD_FLASH_ERASE:
        {
            eCode = cmdHandle_FlashErase(phPrsr);
        }
        break;

        case CMD_EN_WRITE_PROT:
        {
            eCode = cmdHandle_ChangeWriteProt(phPrsr, OB_WRPSTATE_ENABLE);
        }
        break;

        case CMD_DIS_WRITE_PROT:
        {
            eCode = cmdHandle_ChangeWriteProt(phPrsr, OB_WRPSTATE_DISABLE);
        }
        break;

        case CMD_MEM_READ:
        {
            eCode = cmdHandle_MemRead(phPrsr);
        }
        break;

        case CMD_READ_SECT_PROT_STAT:
        {
            eCode = cmdHandle_GetWriteProt(phPrsr);
        }
        break;

        case CMD_FLASH_WRITE:
        {
            eCode = cmdHandle_FlashWrite(phPrsr);
        }
        break;

        case CMD_EXIT:
        {
            eCode = cmdHandle_Exit(phPrsr);
        }
        break;

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
static cbl_err_code_t sendToHost (const char * buf, size_t len)
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
 *
 * @return Error code
 */
static cbl_err_code_t recvFromHost (char * buf, size_t len)
{
    if (HAL_UART_Receive_DMA(pUARTCmd, (uint8_t *)buf, len) == HAL_OK)
    {
        return CBL_ERR_OK;
    }
    else
    {
        return CBL_ERR_HAL_RX;
    }
}

static cbl_err_code_t stopRecvFromHost (void)
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

// \f - new page
static cbl_err_code_t sysState_Error (cbl_err_code_t eCode)
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
            sendToHost(msg, strlen(msg));
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
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_NEED_PARAM:
        {
            char msg[] = "\r\nERROR: Missing parameter(s)\r\n";
            INFO("Command is missing parameter(s)");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_JUMP_INV_ADDR:
        {
            char msg[] = "\r\nERROR: Invalid address\r\n"
                    "Jumpable regions: FLASH, SRAM1, SRAM2, CCMRAM, "
                    "BKPSRAM, SYSMEM and EXTMEM (if connected)\r\n";

            INFO("Invalid address inputed for jumping\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_SECTOR:
        {
            char msg[] = "\r\nERROR: Internal error while erasing sectors\r\n";

            WARNING("Error while erasing sectors\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_INV_SECT:
        {
            char msg[] = "\r\nERROR: Wrong sector given\r\n";

            INFO("Wrong sector given\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_INV_SECT_COUNT:
        {
            char msg[] = "\r\nERROR: Wrong sector count given\r\n";

            INFO("Wrong sector count given\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_WRITE_INV_ADDR:
        {
            char msg[] = "\r\nERROR: Invalid address range entered\r\n";

            INFO("Invalid address range entered for writing\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_WRITE_TOO_BIG:
        {
            char msg[] = "\r\nERROR: Inputed too big value\r\n";

            INFO("User requested to write a too big chunk\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_WRITE:
        {
            char msg[] = "\r\nERROR: Error while writing to flash\r\n";

            INFO("Error while writing to flash on HAL level\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_ERASE_INV_TYPE:
        {
            char msg[] = "\r\nERROR: Invalid erase type\r\n";

            INFO("User entered invalid erase type\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_ERASE:
        {
            char msg[] = "\r\nERROR: HAL error while erasing sectors \r\n";

            INFO("HAL error while erasing sector\r\n");
            sendToHost(msg, strlen(msg));
            eCode = CBL_ERR_OK;
        }
        break;

        case CBL_ERR_HAL_UNLOCK:
        {
            char msg[] = "\r\nERROR: Unlocking flash failed\r\n";

            WARNING("Unlocking flash with HAL failed\r\n");
            sendToHost(msg, strlen(msg));
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
            sendToHost(msg, strlen(msg));
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
            sendToHost(msg, strlen(msg));
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

void HAL_UART_RxCpltCallback (UART_HandleTypeDef * huart)
{
    if (huart == pUARTCmd)
    {
        gRxCmdCntr++;
    }
}

// \f - new page
/*********************************************************/
/* Function handles */
/*********************************************************/

static cbl_err_code_t cmdHandle_Version (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char verbuf[12] = CBL_VERSION;

    DEBUG("Started\r\n");

    /* End with a new line */
    strlcat(verbuf, CRLF, 12);

    /* Send response */
    eCode = sendToHost(verbuf, strlen(verbuf));

    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_Help (parser_t * pphPrsr)
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
            "- " TXT_CMD_GET_RDP_LVL " |  Read protection. Used to protect the"
            " software code stored in Flash memory."
            " Ref. man. p. 93" CRLF CRLF
            "- " TXT_CMD_JUMP_TO " | Jumps to a requested address" CRLF
            "    " TXT_PAR_JUMP_TO_ADDR " - Address to jump to in hex format "
            "(e.g. 0x12345678), 0x can be omitted. " CRLF CRLF
            "- " TXT_CMD_FLASH_ERASE " | Erases flash memory" CRLF
            "    " TXT_PAR_FLASH_ERASE_TYPE " - Defines type of flash erase."
            " \""TXT_PAR_FLASH_ERASE_TYPE_MASS "\" erases all sectors, "
            "\"" TXT_PAR_FLASH_ERASE_TYPE_SECT "\" erases only selected "
            "sectors." CRLF
            "    " TXT_PAR_FLASH_ERASE_SECT " - First sector to erase. "
            "Bootloader is on sectors 0, 1 and 2."
            " Not needed with mass erase." CRLF
            "    " TXT_PAR_FLASH_ERASE_COUNT " - Number of sectors to erase. "
            "Not needed with mass erase." CRLF CRLF
            "- " TXT_CMD_FLASH_WRITE " | Writes to flash, returns "
            TXT_RESP_FLASH_WRITE_READY_HELP " when ready to receive bytes." CRLF
            "     " TXT_PAR_FLASH_WRITE_START " - Starting address in hex "
            "format (e.g. 0x12345678), 0x can be omitted."CRLF
            "     " TXT_PAR_FLASH_WRITE_COUNT " - Number of bytes to write. "
            "Maximum bytes: " TXT_FLASH_WRITE_SZ CRLF CRLF
            "- " TXT_CMD_MEM_READ " | Read bytes from memory" CRLF
            "     " TXT_PAR_FLASH_WRITE_START " - Starting address in hex "
            "format (e.g. 0x12345678), 0x can be omitted."CRLF
            "     " TXT_PAR_FLASH_WRITE_COUNT " - Number of bytes to read."
            CRLF CRLF
            "- " TXT_CMD_EN_WRITE_PROT " | Enables write protection per sector,"
            " as selected with \"" TXT_PAR_EN_WRITE_PROT_MASK "\"." CRLF
            "     " TXT_PAR_EN_WRITE_PROT_MASK " - Mask in hex form for sectors"
            " where LSB corresponds to sector 0." CRLF CRLF
            "- " TXT_CMD_DIS_WRITE_PROT " | Disables write protection per "
            "sector, as selected with \"" TXT_PAR_EN_WRITE_PROT_MASK "\"." CRLF
            "     " TXT_PAR_EN_WRITE_PROT_MASK " - Mask in hex form for sectors"
            " where LSB corresponds to sector 0." CRLF CRLF
            "- " TXT_CMD_READ_SECT_PROT_STAT " | Returns bit array of sector "
            "write protection. LSB corresponds to sector 0. " CRLF CRLF
            "- "TXT_CMD_EXIT " | Exits the bootloader and starts the user "
            "application" CRLF CRLF
            "********************************************************" CRLF
            "Examples************************************************" CRLF
            "********************************************************" CRLF CRLF
            "- Erase sectors 2, 3 and 4" CRLF CRLF TXT_CMD_FLASH_ERASE
            " " TXT_PAR_FLASH_ERASE_TYPE
            "="TXT_PAR_FLASH_ERASE_TYPE_SECT " " TXT_PAR_FLASH_ERASE_SECT"=2 "
            TXT_PAR_FLASH_ERASE_COUNT"=3\\r\\n" CRLF CRLF CRLF
            "- Get version" CRLF CRLF TXT_CMD_VERSION"\\r\\n" CRLF CRLF CRLF
            "- Jump to address 0x12345678" CRLF CRLF TXT_CMD_JUMP_TO " "
            TXT_PAR_JUMP_TO_ADDR "=0x12345678\\r\\n" CRLF CRLF CRLF
            "- Jump to address 0x12345678" CRLF CRLF TXT_CMD_JUMP_TO " "
            TXT_PAR_JUMP_TO_ADDR "=12345678\\r\\n" CRLF CRLF CRLF
            "- Flash " TXT_FLASH_WRITE_SZ " bytes starting from address "
            "0x12345678" CRLF CRLF TXT_CMD_FLASH_WRITE
            " " TXT_PAR_FLASH_WRITE_COUNT
            "=" TXT_FLASH_WRITE_SZ " " TXT_PAR_FLASH_WRITE_START
            "=0x12345678\\r\\n" CRLF
            "STM32 returns:" TXT_RESP_FLASH_WRITE_READY_HELP CRLF
            "*Enter 1024 bytes*" CRLF
            "STM32 returns: " TXT_SUCCESS_HELP CRLF CRLF
            "********************************************************" CRLF
            "********************************************************" CRLF
            CRLF;
    DEBUG("Started\r\n");

    /* Send response */
    eCode = sendToHost(helpPrintout, strlen(helpPrintout));

    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_Cid (parser_t * phPrsr)
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
    eCode = sendToHost(cid, strlen(cid));

    return eCode;
}

// \f - new page
/**
 * @brief   RDP - Read protection
 *              - Used to protect the software code stored in Flash memory.
 *              - Reference manual - p. 93 - Explanation of RDP
 */
static cbl_err_code_t cmdHandle_GetRDPLvl (parser_t * phPrsr)
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
    eCode = sendToHost(buf, strlen(buf));

    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_JumpTo (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charAddr;
    uint32_t addr = 0u;
    void (*jump) (void);

    DEBUG("Started\r\n");

    /* Get the address in hex form */
    charAddr = parser_GetVal(phPrsr, TXT_PAR_JUMP_TO_ADDR,
            strlen(TXT_PAR_JUMP_TO_ADDR));
    if (NULL == charAddr)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Fill addr, skips 0x if present */
    eCode = str2ui32(charAddr, strlen(charAddr), &addr, 16u);
    ERR_CHECK(eCode);

    /* Make sure we can jump to the wanted location */
    eCode = verifyJumpAddress(addr);
    ERR_CHECK(eCode);

    /* Add 1 to the address to set the T bit */
    addr++;
    /*!<    T bit is 0th bit of a function address and tells the processor
     *  if command is ARM T=0 or thumb T=1. STM uses thumb commands.
     *  Reference: https://www.youtube.com/watch?v=VX_12SjnNhY */

    /* Make a function to jump to */
    jump = (void *)addr;

    /* Send response */
    eCode = sendToHost(TXT_SUCCESS, strlen(TXT_SUCCESS));
    ERR_CHECK(eCode);

    /* Jump to requested address, user ensures requested address is valid */
    jump();
    return eCode;
}

// \f - new page
/**
 * @note    Sending sect=64 erases whole flash
 */
static cbl_err_code_t cmdHandle_FlashErase (parser_t * phPrsr)
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

    type = parser_GetVal(phPrsr, TXT_PAR_FLASH_ERASE_TYPE,
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
        charSect = parser_GetVal(phPrsr, TXT_PAR_FLASH_ERASE_SECT,
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
        charCount = parser_GetVal(phPrsr, TXT_PAR_FLASH_ERASE_COUNT,
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

    /* Turn off the blue LED */
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
    eCode = sendToHost(TXT_SUCCESS, strlen(TXT_SUCCESS));

    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_FlashWrite (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char buf[FLASH_WRITE_SZ] = { 0 };
    char *charStart = NULL;
    char *charLen = NULL;
    uint32_t start;
    uint32_t len;
    HAL_StatusTypeDef HALCode;

    DEBUG("Started\r\n");

    /* Get starting address */
    charStart = parser_GetVal(phPrsr, TXT_PAR_FLASH_WRITE_START,
            strlen(TXT_PAR_FLASH_WRITE_START));
    if (NULL == charStart)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Get length in bytes */
    charLen = parser_GetVal(phPrsr, TXT_PAR_FLASH_WRITE_COUNT,
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

    /* Check validity of input addresses */
    if (IS_FLASH_ADDRESS(start) == false
            || IS_FLASH_ADDRESS(start + len) == false)
    {
        return CBL_ERR_WRITE_INV_ADDR;
    }
    /* Check if len is too big  */
    if (len > FLASH_WRITE_SZ)
    {
        return CBL_ERR_WRITE_TOO_BIG;
    }

    /* Reset UART byte counter */
    gRxCmdCntr = 0;

    /* Notify host bootloader is ready to receive bytes */
    sendToHost(TXT_RESP_FLASH_WRITE_READY, strlen(TXT_RESP_FLASH_WRITE_READY));

    /* Request 'len' bytes */
    eCode = recvFromHost(buf, len);
    ERR_CHECK(eCode);

    while (gRxCmdCntr != 1)
    {
        /* Wait for 'len' bytes */
    }

    /* Write data to flash */
    LED_ON(BLUE);

    /* Unlock flash */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Write to flash */
    for (uint32_t i = 0u; i < len; i++)
    {
        /* Write a byte */
        HALCode = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, start + i, buf[i]);
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
    eCode = sendToHost(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_MemRead (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    char *charStart = NULL;
    char *charLen = NULL;
    uint32_t start;
    uint32_t len;

    DEBUG("Started\r\n");

    /* Get starting address */
    charStart = parser_GetVal(phPrsr, TXT_PAR_FLASH_WRITE_START,
            strlen(TXT_PAR_FLASH_WRITE_START));
    if (NULL == charStart)
    {
        return CBL_ERR_NEED_PARAM;
    }
    /* Get length in bytes */
    charLen = parser_GetVal(phPrsr, TXT_PAR_FLASH_WRITE_COUNT,
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
    eCode = sendToHost((char *)start, len);
    return eCode;
}

// \f - new page
/**
 * @brief       Enables write protection on individual flash sectors
 * @param EnDis Write protection state: OB_WRPSTATE_ENABLE or
 *              OB_WRPSTATE_DISABLE
 */
static cbl_err_code_t cmdHandle_ChangeWriteProt (parser_t * phPrsr,
        uint32_t EnDis)
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
    charMask = parser_GetVal(phPrsr, TXT_PAR_EN_WRITE_PROT_MASK,
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
    eCode = sendToHost(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
}

// \f - new page
/**
 * @brief   Gets write protection status of all sectors in binary form
 */
static cbl_err_code_t cmdHandle_GetWriteProt (parser_t * phPrsr)
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
    eCode = sendToHost(buf, strlen(buf));
    return eCode;
}

// \f - new page
static cbl_err_code_t cmdHandle_Exit (parser_t * phPrsr)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    DEBUG("Started\r\n");

    gIsExitReq = true;

    /* Send response */
    eCode = sendToHost(TXT_SUCCESS, strlen(TXT_SUCCESS));
    return eCode;
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
static cbl_err_code_t str2ui32 (const char * str, size_t len, uint32_t * num,
        uint8_t base)
{
    cbl_err_code_t eCode = CBL_ERR_OK;

    eCode = verifyDigitsOnly(str, len, base);
    ERR_CHECK(eCode);

    *num = strtoul(str, NULL, base);
    return eCode;
}

static cbl_err_code_t verifyDigitsOnly (const char * str, size_t len,
        uint8_t base)
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
 * @note    Jumping to peripheral memory locations not permitted
 */
static cbl_err_code_t verifyJumpAddress (uint32_t addr)
{
    cbl_err_code_t eCode = CBL_ERR_JUMP_INV_ADDR;

    if (IS_FLASH_ADDRESS(addr) ||
    IS_CCMDATARAM_ADDRESS(addr) ||
    IS_SRAM1_ADDRESS(addr) ||
    IS_SRAM2_ADDRESS(addr) ||
    IS_BKPSRAM_ADDRESS(addr) ||
    IS_SYSMEM_ADDRESS(addr))
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
static void ui2binstr (uint32_t num, char * str, uint8_t numofbits)
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

/****END OF FILE****/

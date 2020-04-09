#include "CustomBootLoader.h"

static void cbl_ShellInit(void);
static void cbl_RunUserApp(void);
static CBL_ErrCode_t cbl_RunShellSystem(void);
static CBL_ErrCode_t cbl_StateOperation(void);
static CBL_ErrCode_t cbl_WaitForCmd(out char* buf, size_t len);
static CBL_ErrCode_t cbl_ParseCmd(char *cmd, size_t len, out CBL_Parser_t *p);
static char *cbl_ParserGetArgVal(CBL_Parser_t *p, char * name, size_t lenName);
static CBL_ErrCode_t cbl_EnumCmd(out char* buf, size_t len, out CBL_CMD_t *cmdCode);
static CBL_ErrCode_t cbl_HandleCmd(CBL_CMD_t cmdCode, CBL_Parser_t* p);
static CBL_ErrCode_t cbl_SendToHost(const char *buf, size_t len);
static CBL_ErrCode_t cbl_RecvFromHost(out char *buf, size_t len);
//static CBL_Err_Code_t cbl_StopRecvFromHost();
static CBL_ErrCode_t cbl_StateError(CBL_ErrCode_t eCode);
static CBL_ErrCode_t cbl_HandleCmdVersion(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdHelp(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdCid(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdGetRDPLvl(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdJumpTo(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_VerifyJumpAddress(uint32_t addr);
static CBL_ErrCode_t cbl_HandleCmdFlashErase(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdEnRWPr(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdDisRWPr(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdMemRead(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdGetSectStat(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdOTPRead(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdMemWrite(CBL_Parser_t *p);
static CBL_ErrCode_t cbl_HandleCmdExit(CBL_Parser_t *p);

static uint32_t cntrRecvChar = 0;

static bool isExitReq = false;

static const char *cbl_supported_cmds =
		""
				"Custom STM32F4 bootloader by Dino Saric - " CBL_VERSION CRLF CRLF
		"Optional parameters are surrounded with [] " CRLF CRLF
		"Supported commands:" CRLF
		"- " CBL_TXTCMD_VERSION " | Gets the current version of the running bootloader" CRLF

		"- " CBL_TXTCMD_HELP " | Makes life easier" CRLF

		"- " CBL_TXTCMD_CID " | Gets chip identification number" CRLF

		"- " CBL_TXTCMD_GET_RDP_LVL " |  Read protection. Used to protect the software code stored in Flash memory."
		" Ref. man. p. 93" CRLF
		"- " CBL_TXTCMD_JUMP_TO " | Jumps to a requested address" CRLF
		"    " CBL_TXTCMD_JUMP_TO_ADDR " - address to jump to in hex format (e.g. 0x12345678), 0x can be omitted. " CRLF

		"- " CBL_TXTCMD_FLASH_ERASE " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_EN_RW_PR " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_DIS_RW_PR " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_MEM_READ " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_GET_SECT_STAT " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_OTP_READ " | TODO" CRLF
		"     " CRLF

		"- " CBL_TXTCMD_MEM_WRITE " | TODO" CRLF
		"     " CRLF

		"- "CBL_TXTCMD_EXIT " | Exits the bootloader and starts the user application" CRLF;

void CBL_Start()
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	INFO("Custom bootloader started\r\n");
	if (HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
	{
		INFO("Blue button pressed...\r\n");
//		eCode = cbl_runShellSystem(); TODO UNCOMMENT
	}
	else
	{
		INFO("Blue button not pressed...\r\n");
		eCode = cbl_RunShellSystem(); // TODO REMOVE
	}
	ASSERT(eCode == CBL_ERR_OK, "ErrCode=%d:Restart the application.\r\n", eCode);
	cbl_RunUserApp();
	ERROR("Switching to user application failed\r\n");
}

static void cbl_ShellInit(void)
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
	cbl_SendToHost(bufWelcome, strlen(bufWelcome));
}

/**
 * @brief	Gives controler to the user application
 * 			Steps:
 * 			1) 	Set the main stack pointer (MSP) to the one of the user application.
 * 				User application MSP is contained in the first four bytes of flashed user application.
 *
 * 			2)	Set the reset handler to the one of the user application.
 * 				User application reset handler is right after the MSP, size of 4 bytes.
 *
 * 			3)	Jump to the user application reset handler. Giving control to the user application.
 *
 * @note	DO NOT FORGET: In user application VECT_TAB_OFFSET set to the offset of user application
 * 			from the start of the flash.
 * 			e.g. If our application starts in the 2nd sector we would write #define VECT_TAB_OFFSET 0x8000.
 * 			VECT_TAB_OFFSET is located in system_Stm32f4xx.c
 *
 * @return	Procesor never returns from this application
 */
static void cbl_RunUserApp(void)
{
	void (*userAppResetHandler)(void);
	uint32_t addressRstHndl;
	volatile uint32_t msp_value = *(volatile uint32_t *)CBL_ADDR_USERAPP;

	char userAppHello[] = "Jumping to user application :)\r\n";

	/* Send hello message to user and debug output */
	cbl_SendToHost(userAppHello, strlen(userAppHello));
	INFO("%s", userAppHello);

	addressRstHndl = *(volatile uint32_t *) (CBL_ADDR_USERAPP + 4);

	userAppResetHandler = (void *)addressRstHndl;

	DEBUG("MSP value: %#x\r\n", (unsigned int ) msp_value);
	DEBUG("Reset handler address: %#x\r\n", (unsigned int ) addressRstHndl);

	/* function from CMSIS */
	__set_MSP(msp_value);

	/* Give control to user application */
	userAppResetHandler();

	/* Never reached */
}
/**
 * @brief	Runs the shell for the bootloader.
 * @return	CBL_ERR_NO when no error, else returns an error code.
 */
static CBL_ErrCode_t cbl_RunShellSystem(void)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	bool isExitNeeded = false;
	CBL_sysStates_t state = CBL_STAT_ERR, nextState;

	INFO("Starting bootloader\r\n");

	nextState = state;

	cbl_ShellInit();

	while (isExitNeeded == false)
	{
		switch (state)
		{
			case CBL_STAT_OPER:
			{
				eCode = cbl_StateOperation();

				/* Switch state if needed */
				if (eCode != CBL_ERR_OK)
				{
					nextState = CBL_STAT_ERR;
				}
				else if (isExitReq == true)
				{
					nextState = CBL_STAT_EXIT;
				}
				break;
			}
			case CBL_STAT_ERR:
			{
				eCode = cbl_StateError(eCode);

				/* Switch state */
				if (eCode != CBL_ERR_OK)
				{
					nextState = CBL_STAT_EXIT;
				}
				else
				{
					nextState = CBL_STAT_OPER;
				}
				break;
			}
			case CBL_STAT_EXIT:
			{
				/* Deconstructor */
				char bye[] = "Exiting shell :(\r\n\r\n";

				INFO(bye);
				cbl_SendToHost(bye, strlen(bye));

				isExitNeeded = true;

				break;
			}
			default:
			{
				eCode = CBL_ERR_STATE;
				break;
			}
		}
		state = nextState;
	}
	return eCode;
}

static CBL_ErrCode_t cbl_StateOperation(void)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	CBL_CMD_t cmdCode = CBL_CMD_UNDEF;
	CBL_Parser_t parser = { 0 };
	char cmd[CBL_CMD_BUF_SZ] = { 0 };

	eCode = cbl_WaitForCmd(cmd, CBL_CMD_BUF_SZ);
	ERR_CHECK(eCode);

	eCode = cbl_ParseCmd(cmd, strlen(cmd), &parser);
	ERR_CHECK(eCode);

	eCode = cbl_EnumCmd(parser.cmd, strlen(cmd), &cmdCode);
	ERR_CHECK(eCode);

	eCode = cbl_HandleCmd(cmdCode, &parser);
	return eCode;
}

static CBL_ErrCode_t cbl_WaitForCmd(out char* buf, size_t len)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	bool isLastCharCR = false, isOverflow = true;
	uint32_t i = 0;
	cntrRecvChar = 0;

	eCode = cbl_SendToHost("\r\n>", 3);
	ERR_CHECK(eCode);

	/* Read until CRLF or until full DMA*/
	while (cntrRecvChar < len)
	{
		/* Receive one char from host */
		eCode = cbl_RecvFromHost(buf + i, 1);
		ERR_CHECK(eCode);

		/* Wait for a new char */
		while (i != (cntrRecvChar - 1))
		{
		}

		/* If last char was \r  */
		if (isLastCharCR == true)
		{
			/* Check if \n was received */
			if (buf[i] == '\n')
			{
				/* CRLF was received, command done */
				/*!< Replace '\r' with '\0' to make sure every char array ends with '\0' */
				buf[i - 1] = '\0';
				isOverflow = false;
				break;
			}
		}

		/* update isLastCharCR */
		isLastCharCR = buf[i] == '\r' ? true: false;

		/* prepare for next char */
		i++;
	}
	/* If DMA fills the buffer and no CRLF is received throw an error */
	if (isOverflow == true)
	{
		eCode = CBL_ERR_READ_OF;
	}
	/* If no error buf has a new command to process */
	return eCode;
}

/**
 * @brief		Parses a command into CBL_Parser_t. Command's form is as follows:
 * 				somecmd pname1=pval1 pname2=pval2
 * @note		This function is destructive to input cmd, as it replaces all ' ' and '='
 * 				with '\0'
 * @param cmd	NULL terminated string containing command without \r\n
 * @param len	length of string contained in cmd
 * @param p		Function assumes empty parser p on entrance
 * @return
 */
static CBL_ErrCode_t cbl_ParseCmd(char *cmd, size_t len, out CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char *pSpa = NULL, *pEqu = NULL, *pLastChar = &cmd[len];
	uint8_t i;

	/* Convert the string to lower case */
	strlwr(cmd);

	/* Find the first ' ' */
	pSpa = memchr(cmd, ' ', len);

	for (i = 0; i < CBL_MAX_ARGS && pSpa != NULL; i++)
	{
		/* Command name/value name ends with ' ', replace with '\0' */
		*pSpa = '\0';

		/* Find an end of the param name */
		pEqu = memchr(pSpa, '=', pLastChar - pSpa);

		if (pEqu == NULL)
		{
			/* Exit from the loop as there is no value for the argument */
			break;
		}

		/* Argument starts after ' ' */
		p->args[i][CBL_ARG_NAME] = (pSpa + 1);
		/* Arguments end with '=', replace with '\0' */
		*pEqu = '\0';

		/* Parameter value starts after '=' */
		p->args[i][CBL_ARG_VAL] = (pEqu + 1);

		/* Find the next space */
		pSpa = memchr(pEqu, ' ', pLastChar - pEqu);
	}

	p->cmd = cmd;
	p->len = len;
	p->numOfArgs = i;

	return eCode;
}

static char *cbl_ParserGetArgVal(CBL_Parser_t *p, char * name, size_t lenName)
{
	size_t lenArgName;

	/* Walk through all the parameters */
	for (uint32_t i = 0; i < p->numOfArgs; i++)
	{
		/* Get the length of ith parameter */
		lenArgName = strlen(p->args[i][CBL_ARG_NAME]);

		/* Check if ith parameter is the requested one */
		if (lenArgName == lenName && strncmp(p->args[i][CBL_ARG_NAME], name, lenArgName) == 0)
			return p->args[i][CBL_ARG_VAL];
	}

	/* No parameter with name 'name' found */
	return NULL;
}

static CBL_ErrCode_t cbl_EnumCmd(char* buf, size_t len, out CBL_CMD_t *cmdCode)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	if (len == 0)
	{
		eCode = CBL_ERR_CMD_SHORT;
	}
	else if (len == strlen(CBL_TXTCMD_VERSION)
			&& strncmp(buf, CBL_TXTCMD_VERSION, strlen(CBL_TXTCMD_VERSION)) == 0)
	{
		*cmdCode = CBL_CMD_VERSION;
	}
	else if (len == strlen(CBL_TXTCMD_HELP)
			&& strncmp(buf, CBL_TXTCMD_HELP, strlen(CBL_TXTCMD_HELP)) == 0)
	{
		*cmdCode = CBL_CMD_HELP;
	}
	else if (len == strlen(CBL_TXTCMD_CID)
			&& strncmp(buf, CBL_TXTCMD_CID, strlen(CBL_TXTCMD_CID)) == 0)
	{
		*cmdCode = CBL_CMD_CID;
	}
	else if (len == strlen(CBL_TXTCMD_GET_RDP_LVL)
			&& strncmp(buf, CBL_TXTCMD_GET_RDP_LVL, strlen(CBL_TXTCMD_GET_RDP_LVL)) == 0)
	{
		*cmdCode = CBL_CMD_GET_RDP_LVL;
	}
	else if (len == strlen(CBL_TXTCMD_JUMP_TO)
			&& strncmp(buf, CBL_TXTCMD_JUMP_TO, strlen(CBL_TXTCMD_JUMP_TO)) == 0)
	{
		*cmdCode = CBL_CMD_JUMP_TO;
	}
	else if (len == strlen(CBL_TXTCMD_FLASH_ERASE)
			&& strncmp(buf, CBL_TXTCMD_FLASH_ERASE, strlen(CBL_TXTCMD_FLASH_ERASE)) == 0)
	{
		*cmdCode = CBL_CMD_FLASH_ERASE;
	}
	else if (len == strlen(CBL_TXTCMD_EN_RW_PR)
			&& strncmp(buf, CBL_TXTCMD_EN_RW_PR, strlen(CBL_TXTCMD_EN_RW_PR)) == 0)
	{
		*cmdCode = CBL_CMD_EN_RW_PR;
	}
	else if (len == strlen(CBL_TXTCMD_DIS_RW_PR)
			&& strncmp(buf, CBL_TXTCMD_DIS_RW_PR, strlen(CBL_TXTCMD_DIS_RW_PR)) == 0)
	{
		*cmdCode = CBL_CMD_DIS_RW_PR;
	}
	else if (len == strlen(CBL_TXTCMD_MEM_READ)
			&& strncmp(buf, CBL_TXTCMD_MEM_READ, strlen(CBL_TXTCMD_MEM_READ)) == 0)
	{
		*cmdCode = CBL_CMD_MEM_READ;
	}
	else if (len == strlen(CBL_TXTCMD_GET_SECT_STAT)
			&& strncmp(buf, CBL_TXTCMD_GET_SECT_STAT, strlen(CBL_TXTCMD_GET_SECT_STAT)) == 0)
	{
		*cmdCode = CBL_CMD_GET_SECT_STAT;
	}
	else if (len == strlen(CBL_TXTCMD_OTP_READ)
			&& strncmp(buf, CBL_TXTCMD_OTP_READ, strlen(CBL_TXTCMD_OTP_READ)) == 0)
	{
		*cmdCode = CBL_CMD_OTP_READ;
	}
	else if (len == strlen(CBL_TXTCMD_MEM_WRITE)
			&& strncmp(buf, CBL_TXTCMD_MEM_WRITE, strlen(CBL_TXTCMD_MEM_WRITE)) == 0)
	{
		*cmdCode = CBL_CMD_MEM_WRITE;
	}
	else if (len == strlen(CBL_TXTCMD_EXIT)
			&& strncmp(buf, CBL_TXTCMD_EXIT, strlen(CBL_TXTCMD_EXIT)) == 0)
	{
		*cmdCode = CBL_CMD_EXIT;
	}

	if (eCode == CBL_ERR_OK && *cmdCode == CBL_CMD_UNDEF)
		eCode = CBL_ERR_CMD_UNDEF;
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmd(CBL_CMD_t cmdCode, CBL_Parser_t* p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	switch (cmdCode)
	{
		case CBL_CMD_VERSION:
		{
			eCode = cbl_HandleCmdVersion(p);
			break;
		}
		case CBL_CMD_HELP:
		{
			eCode = cbl_HandleCmdHelp(p);
			break;
		}
		case CBL_CMD_CID:
		{
			eCode = cbl_HandleCmdCid(p);
			break;
		}
		case CBL_CMD_GET_RDP_LVL:
		{
			eCode = cbl_HandleCmdGetRDPLvl(p);
			break;
		}
		case CBL_CMD_JUMP_TO:
		{
			eCode = cbl_HandleCmdJumpTo(p);
			break;
		}
		case CBL_CMD_FLASH_ERASE:
		{
			eCode = cbl_HandleCmdFlashErase(p);
			break;
		}
		case CBL_CMD_EN_RW_PR:
		{
			eCode = cbl_HandleCmdEnRWPr(p);
			break;
		}
		case CBL_CMD_DIS_RW_PR:
		{
			eCode = cbl_HandleCmdDisRWPr(p);
			break;
		}
		case CBL_CMD_MEM_READ:
		{
			eCode = cbl_HandleCmdMemRead(p);
			break;
		}
		case CBL_CMD_GET_SECT_STAT:
		{
			eCode = cbl_HandleCmdGetSectStat(p);
			break;
		}
		case CBL_CMD_OTP_READ:
		{
			eCode = cbl_HandleCmdOTPRead(p);
			break;
		}
		case CBL_CMD_MEM_WRITE:
		{
			eCode = cbl_HandleCmdMemWrite(p);
			break;
		}
		case CBL_CMD_EXIT:
		{
			eCode = cbl_HandleCmdExit(p);
			break;
		}
		case CBL_CMD_UNDEF:
		default:
		{
			eCode = CBL_ERR_CMDCD;
		}
	}
	DEBUG("Responded\r\n");
	return eCode;
}

static CBL_ErrCode_t cbl_SendToHost(const char *buf, size_t len)
{
	if (HAL_UART_Transmit(pUARTCmd, (uint8_t *)buf, len, HAL_MAX_DELAY) == HAL_OK)
	{
		return CBL_ERR_OK;
	}
	else
	{
		return CBL_ERR_HAL_TX;
	}
}

static CBL_ErrCode_t cbl_RecvFromHost(out char *buf, size_t len)
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

//static CBL_Err_Code_t cbl_StopRecvFromHost()
//{
//	if (HAL_UART_AbortReceive(pUARTCmd) == HAL_OK)
//	{
//		return CBL_ERR_OK;
//	}
//	else
//	{
//		return CBL_ERR_RX_ABORT;
//	}
//}

static CBL_ErrCode_t cbl_StateError(CBL_ErrCode_t eCode)
{
	DEBUG("Started\r\n");
	switch (eCode)
	{
		case CBL_ERR_OK:
		{
			/* FALSE ALARM - no error */
			break;
		}
		case CBL_ERR_READ_OF:
		{
			char msg[] = "\r\nERROR: Command too long\r\n";
			WARNING("Overflow while reading happened\r\n");
			cbl_SendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_WRITE:
		{
			WARNING("Error occurred while writing\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_STATE:
		{
			WARNING("System entered unknown state, returning to operational\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_HAL_TX:
		{
			WARNING("HAL transmit error happened\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_HAL_RX:
		{
			WARNING("HAL receive error happened\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_RX_ABORT:
		{
			WARNING("Error happened while aborting receive\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_CMD_SHORT:
		{
			INFO("Client sent an empty command\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_CMD_UNDEF:
		{
			char msg[] = "\r\nERROR: Invalid command\r\n";
			INFO("Client sent an invalid command\r\n");
			cbl_SendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_NEED_PARAM:
		{
			char msg[] = "\r\nERROR: Missing parameter(s)\r\n";
			INFO("Command is missing parameter(s)");
			cbl_SendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_INV_ADDR:
		{
			char msg[] = "\r\nERROR: Invalid address\r\nJumpable regions: FLASH, SRAM1, SRAM2, CCMRAM, BKPSRAM and SYSMEM\r\n";
			INFO("Invalid address inputed\r\n");
			cbl_SendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		default:
		{
			WARNING("Unhandled error happened\r\n");
			break;
		}
	}
	return eCode;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == pUARTCmd)
	{
		cntrRecvChar++;
	}
}

/*********************************************************/
/**Function handles**/
/*********************************************************/

/**
 * @brief
 * @return
 */
static CBL_ErrCode_t cbl_HandleCmdVersion(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char verbuf[12] = CBL_VERSION;

	DEBUG("Started\r\n");

	/* End with a new line */
	strlcat(verbuf, CRLF, 12);

	/* Send response */
	eCode = cbl_SendToHost(verbuf, strlen(verbuf));

	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdHelp(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(cbl_supported_cmds, strlen(cbl_supported_cmds));

	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdCid(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char cid[14] = "0x", cidhelp[10];

	DEBUG("Started\r\n");

	/* Convert hex value to text */
	itoa((int) (DBGMCU->IDCODE & 0x00000FFF), cidhelp, 16);

	/* Add 0x to to beginning */
	strlcat(cid, cidhelp, 12);

	/* End with a new line */
	strlcat(cid, CRLF, 12);

	/* Send response */
	eCode = cbl_SendToHost(cid, strlen(cid));

	return eCode;
}

/**
 * @brief	RDP - Read protection
 * 				- Used to protect the software code stored in Flash memory.
 * 				- Reference manual - p. 93 - Explanation of RDP
 */
static CBL_ErrCode_t cbl_HandleCmdGetRDPLvl(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
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
			break;
		}
		case OB_RDP_LEVEL_2:
		{
			strcpy(buf, "level 2");
			break;
		}
		default:
		{
			/* Any other value is RDP level 1 */
			strcpy(buf, "level 1");
		}
	}

	strlcat(buf, CRLF, 32);

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));

	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdJumpTo(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "Jumping to requested address", *charAddr;
	uint32_t addr = 0;
	void (*jump)(void);

	DEBUG("Started\r\n");

	/* Get the address in hex form */
	charAddr = cbl_ParserGetArgVal(p, CBL_TXTCMD_JUMP_TO_ADDR, strlen(CBL_TXTCMD_JUMP_TO_ADDR));

	if (charAddr == NULL)
		return CBL_ERR_NEED_PARAM;

	/* Cast the address to uint32_t, skips 0x if present */
	addr = (uint32_t)strtoul(charAddr, NULL, 16);

	/* Make sure we can jump to the wanted location */
	eCode = cbl_VerifyJumpAddress(addr);
	ERR_CHECK(eCode);

	/* Add one to the address to set the T bit */
	addr++;
	/**	T bit is 0th bit of a function address and tells the processor
	 *	if command is ARM T=0 or thumb T=1. STM uses thumb commands.
	 *	@ref https://www.youtube.com/watch?v=VX_12SjnNhY */

	/* Make a function to jump to */
	jump = (void *)addr;

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	ERR_CHECK(eCode);

	/* Jump to requested address, user ensures requested address is valid */
	jump();
	return eCode;
}

/**
 * @brief	Verifies address is in jumpable region
 * @note	Jumping to peripheral memory locations not permitted
 */
static CBL_ErrCode_t cbl_VerifyJumpAddress(uint32_t addr)
{
	CBL_ErrCode_t eCode = CBL_ERR_INV_ADDR;

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

static CBL_ErrCode_t cbl_HandleCmdFlashErase(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));

	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdEnRWPr(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdDisRWPr(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdMemRead(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdGetSectStat(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdOTPRead(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdMemWrite(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[32] = "";

	DEBUG("Started\r\n");

	/* Send response */
	eCode = cbl_SendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cbl_HandleCmdExit(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	DEBUG("Started\r\n");

	isExitReq = true;

	return eCode;
}

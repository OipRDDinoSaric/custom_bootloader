#include "CustomBootLoader.h"

static void shellInit(void);
static void runUserApp(void);
static CBL_ErrCode_t runShellSystem(void);
static CBL_ErrCode_t sysState_Operation(void);
static CBL_ErrCode_t waitForCmd(out char* buf, size_t len);
static CBL_ErrCode_t parser_Run(char *cmd, size_t len, out CBL_Parser_t *p);
static char *parser_GetVal(CBL_Parser_t *p, char * name, size_t lenName);
static CBL_ErrCode_t enumCmd(out char* buf, size_t len, out CBL_CMD_t *cmdCode);
static CBL_ErrCode_t handleCmd(CBL_CMD_t cmdCode, CBL_Parser_t* p);
static CBL_ErrCode_t sendToHost(const char *buf, size_t len);
static CBL_ErrCode_t recvFromHost(out char *buf, size_t len);
static CBL_ErrCode_t stopRecvFromHost(void);
static CBL_ErrCode_t sysState_Error(CBL_ErrCode_t eCode);
static CBL_ErrCode_t cmdHandle_Version(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_Help(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_Cid(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_GetRDPLvl(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_JumpTo(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_FlashErase(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_ChangeWriteProt(CBL_Parser_t *p, uint32_t EnDis);
static CBL_ErrCode_t cmdHandle_MemRead(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_GetWriteProt(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_GetOTPBytes(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_FlashWrite(CBL_Parser_t *p);
static CBL_ErrCode_t cmdHandle_Exit(CBL_Parser_t *p);
static CBL_ErrCode_t str2ui32(const char *s, size_t len, out uint32_t *num, uint8_t base);
static CBL_ErrCode_t verifyDigitsOnly(const char *s, size_t i, uint8_t base);
static CBL_ErrCode_t verifyJumpAddress(uint32_t addr);
static void ui2binstr(uint32_t num, out char *str, uint8_t numofbits);

static uint32_t gRxCmdCntr = 0;

static bool isExitReq = false;

static char *helpPrintout =
		"*************************************************************" CRLF
		"*************************************************************" CRLF
		"Custom STM32F4 bootloader shell by Dino Saric - " CBL_VERSION "*********" CRLF
		"*************************************************************" CRLF CRLF
		"*************************************************************" CRLF
		"Examples*****************************************************" CRLF
		"*************************************************************" CRLF CRLF
		"Optional parameters are surrounded with [] " CRLF CRLF
		"- " CBL_TXTCMD_VERSION " | Gets the current version of the running bootloader" CRLF CRLF
		"- " CBL_TXTCMD_HELP " | Makes life easier" CRLF CRLF
		"- " CBL_TXTCMD_CID " | Gets chip identification number" CRLF CRLF
		"- " CBL_TXTCMD_GET_RDP_LVL " |  Read protection. Used to protect the software code stored in Flash memory."
		" Ref. man. p. 93" CRLF CRLF
		"- " CBL_TXTCMD_JUMP_TO " | Jumps to a requested address" CRLF
		"    " CBL_TXTCMD_JUMP_TO_ADDR " - Address to jump to in hex format (e.g. 0x12345678), 0x can be omitted. " CRLF CRLF
		"- " CBL_TXTCMD_FLASH_ERASE " | Erases flash memory" CRLF
		"    " CBL_TXTCMD_FLASH_ERASE_TYPE " - Defines type of flash erase." " \""CBL_TXTCMD_FLASH_ERASE_TYPE_MASS "\" erases all sectors, "
		"\"" CBL_TXTCMD_FLASH_ERASE_TYPE_SECT "\" erases only selected sectors." CRLF
		"    " CBL_TXTCMD_FLASH_ERASE_SECT " - First sector to erase. Bootloader is on sectors 0 and 1. Not needed with mass erase." CRLF
		"    " CBL_TXTCMD_FLASH_ERASE_COUNT " - Number of sectors to erase. Not needed with mass erase." CRLF CRLF
		"- " CBL_TXTCMD_EN_WRITE_PROT " | Enables write protection per sector, as selected with \"" CBL_TXTCMD_EN_WRITE_PROT_MASK "\"." CRLF
		"     " CBL_TXTCMD_EN_WRITE_PROT_MASK " - Mask in hex form for sectors where LSB corresponds to sector 0." CRLF CRLF
		"- " CBL_TXTCMD_DIS_WRITE_PROT " | Disables write protection on all sectors" CRLF
		"     " CBL_TXTCMD_EN_WRITE_PROT_MASK " - Mask in hex form for sectors where LSB corresponds to sector 0." CRLF CRLF

		"- " CBL_TXTCMD_READ_SECT_PROT_STAT " | Returns bit array of sector write protection. MSB corresponds to sector with highest number." CRLF
		"     " CRLF CRLF
		"- " CBL_TXTCMD_MEM_READ " | TODO" CRLF
		"     " CRLF CRLF
		"- " CBL_TXTCMD_GET_OTP_BYTES " | TODO" CRLF
		"     " CRLF CRLF
		"- " CBL_TXTCMD_FLASH_WRITE " | Writes to flash, returns " CBL_TXTRESP_FLASH_WRITE_READY_HELP " when ready to receive bytes." CRLF
		"     " CBL_TXTCMD_FLASH_WRITE_START " - Starting address in hex format (e.g. 0x12345678), 0x can be omitted."CRLF
		"     " CBL_TXTCMD_FLASH_WRITE_COUNT " - Number of bytes to write. Maximum bytes: " CBL_FLASH_WRITE_SZ_TXT CRLF CRLF
		"- "CBL_TXTCMD_EXIT " | Exits the bootloader and starts the user application" CRLF CRLF
		"********************************************************" CRLF
		"Examples************************************************" CRLF
		"********************************************************" CRLF CRLF
		"- Erase sectors 2, 3 and 4" CRLF CRLF CBL_TXTCMD_FLASH_ERASE " " CBL_TXTCMD_FLASH_ERASE_TYPE
		"="CBL_TXTCMD_FLASH_ERASE_TYPE_SECT " " CBL_TXTCMD_FLASH_ERASE_SECT"=2 " CBL_TXTCMD_FLASH_ERASE_COUNT"=3\\r\\n" CRLF CRLF CRLF
		"- Get version" CRLF CRLF CBL_TXTCMD_VERSION"\\r\\n" CRLF CRLF CRLF
		"- Jump to address 0x12345678" CRLF CRLF CBL_TXTCMD_JUMP_TO " " CBL_TXTCMD_JUMP_TO_ADDR "=0x12345678\\r\\n" CRLF CRLF CRLF
		"- Jump to address 0x12345678" CRLF CRLF CBL_TXTCMD_JUMP_TO " " CBL_TXTCMD_JUMP_TO_ADDR "=12345678\\r\\n" CRLF CRLF CRLF
		"- Flash " CBL_FLASH_WRITE_SZ_TXT " bytes starting from address 0x12345678" CRLF CRLF CBL_TXTCMD_FLASH_WRITE " " CBL_TXTCMD_FLASH_WRITE_COUNT
		"=" CBL_FLASH_WRITE_SZ_TXT " " CBL_TXTCMD_FLASH_WRITE_START "=0x12345678\\r\\n" CRLF
		"STM32 returns:" CBL_TXTRESP_FLASH_WRITE_READY_HELP CRLF
		"*Enter 1024 bytes*" CRLF
		"STM32 returns: " CBL_TXT_SUCCESS_HELP CRLF CRLF
		"********************************************************" CRLF
		"********************************************************" CRLF CRLF;
void CBL_Run()
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
		eCode = runShellSystem(); // TODO REMOVE
	}
	ASSERT(eCode == CBL_ERR_OK, "ErrCode=%d:Restart the application.\r\n", eCode);
	runUserApp();
	ERROR("Switching to user application failed\r\n");
}

static void shellInit(void)
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
static void runUserApp(void)
{
	void (*userAppResetHandler)(void);
	uint32_t addressRstHndl;
	volatile uint32_t msp_value = *(volatile uint32_t *)CBL_ADDR_USERAPP;

	char userAppHello[] = "Jumping to user application :)\r\n";

	/* Send hello message to user and debug output */
	sendToHost(userAppHello, strlen(userAppHello));
	INFO("%s", userAppHello);

	addressRstHndl = *(volatile uint32_t *) (CBL_ADDR_USERAPP + 4);

	userAppResetHandler = (void *)addressRstHndl;

	DEBUG("MSP value: %#x\r\n", (unsigned int ) msp_value);
	DEBUG("Reset handler address: %#x\r\n", (unsigned int ) addressRstHndl);

	/* Function from CMSIS */
	__set_MSP(msp_value);

	/* Give control to user application */
	userAppResetHandler();

	/* Never reached */
}
/**
 * @brief	Runs the shell for the bootloader.
 * @return	CBL_ERR_NO when no error, else returns an error code.
 */
static CBL_ErrCode_t runShellSystem(void)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	bool isExitNeeded = false;
	CBL_sysStates_t state = CBL_STAT_ERR, nextState;

	INFO("Starting bootloader\r\n");

	nextState = state;

	shellInit();

	while (isExitNeeded == false)
	{
		switch (state)
		{
			case CBL_STAT_OPER:
			{
				eCode = sysState_Operation();

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
				eCode = sysState_Error(eCode);

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
				sendToHost(bye, strlen(bye));

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
	/* Botloader done, turn off red LED */
	LED_OFF(RED);

	return eCode;
}

static CBL_ErrCode_t sysState_Operation(void)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	CBL_CMD_t cmdCode = CBL_CMD_UNDEF;
	CBL_Parser_t parser = { 0 };
	char cmd[CBL_CMD_BUF_SZ] = { 0 };

	LED_ON(GREEN);
	eCode = waitForCmd(cmd, CBL_CMD_BUF_SZ);
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

static CBL_ErrCode_t waitForCmd(out char* buf, size_t len)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	bool isLastCharCR = false, isOverflow = true;
	uint32_t i = 0;
	gRxCmdCntr = 0;

	eCode = sendToHost("\r\n> ", 4);
	ERR_CHECK(eCode);

	/* Read until CRLF or until full DMA*/
	while (gRxCmdCntr < len)
	{
		/* Receive one char from host */
		eCode = recvFromHost(buf + i, 1);
		ERR_CHECK(eCode);

		/* Wait for a new char */
		while (i != (gRxCmdCntr - 1))
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
 * 				with '\0' and transform all to lower case
 * @param cmd	NULL terminated string containing command without \r\n, parser changes it to lower case
 * @param len	length of string contained in cmd
 * @param p		Function assumes empty parser p on entrance
 * @return
 */
static CBL_ErrCode_t parser_Run(char *cmd, size_t len, out CBL_Parser_t *p)
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
		/* Command name(first pass)/value name ends with ' ', replace with '\0' */
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

/**
 * @brief			Gets value from a parameter
 * @param p			parser
 * @param name		Name of a parameter
 * @param lenName	Name length
 * @return	Pointer to the value
 */
static char *parser_GetVal(CBL_Parser_t *p, char * name, size_t lenName)
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

static CBL_ErrCode_t enumCmd(char* buf, size_t len, out CBL_CMD_t *cmdCode)
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
	else if (len == strlen(CBL_TXTCMD_EN_WRITE_PROT)
			&& strncmp(buf, CBL_TXTCMD_EN_WRITE_PROT, strlen(CBL_TXTCMD_EN_WRITE_PROT)) == 0)
	{
		*cmdCode = CBL_CMD_EN_WRITE_PROT;
	}
	else if (len == strlen(CBL_TXTCMD_DIS_WRITE_PROT)
			&& strncmp(buf, CBL_TXTCMD_DIS_WRITE_PROT, strlen(CBL_TXTCMD_DIS_WRITE_PROT)) == 0)
	{
		*cmdCode = CBL_CMD_DIS_WRITE_PROT;
	}
	else if (len == strlen(CBL_TXTCMD_MEM_READ)
			&& strncmp(buf, CBL_TXTCMD_MEM_READ, strlen(CBL_TXTCMD_MEM_READ)) == 0)
	{
		*cmdCode = CBL_CMD_MEM_READ;
	}
	else if (len == strlen(CBL_TXTCMD_READ_SECT_PROT_STAT)
			&& strncmp(buf, CBL_TXTCMD_READ_SECT_PROT_STAT, strlen(CBL_TXTCMD_READ_SECT_PROT_STAT))
					== 0)
	{
		*cmdCode = CBL_CMD_READ_SECT_PROT_STAT;
	}
	else if (len == strlen(CBL_TXTCMD_GET_OTP_BYTES)
			&& strncmp(buf, CBL_TXTCMD_GET_OTP_BYTES, strlen(CBL_TXTCMD_GET_OTP_BYTES)) == 0)
	{
		*cmdCode = CBL_CMD_GET_OTP_BYTES;
	}
	else if (len == strlen(CBL_TXTCMD_FLASH_WRITE)
			&& strncmp(buf, CBL_TXTCMD_FLASH_WRITE, strlen(CBL_TXTCMD_FLASH_WRITE)) == 0)
	{
		*cmdCode = CBL_CMD_FLASH_WRITE;
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

static CBL_ErrCode_t handleCmd(CBL_CMD_t cmdCode, CBL_Parser_t* p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	switch (cmdCode)
	{
		case CBL_CMD_VERSION:
		{
			eCode = cmdHandle_Version(p);
			break;
		}
		case CBL_CMD_HELP:
		{
			eCode = cmdHandle_Help(p);
			break;
		}
		case CBL_CMD_CID:
		{
			eCode = cmdHandle_Cid(p);
			break;
		}
		case CBL_CMD_GET_RDP_LVL:
		{
			eCode = cmdHandle_GetRDPLvl(p);
			break;
		}
		case CBL_CMD_JUMP_TO:
		{
			eCode = cmdHandle_JumpTo(p);
			break;
		}
		case CBL_CMD_FLASH_ERASE:
		{
			eCode = cmdHandle_FlashErase(p);
			break;
		}
		case CBL_CMD_EN_WRITE_PROT:
		{
			eCode = cmdHandle_ChangeWriteProt(p, OB_WRPSTATE_ENABLE);
			break;
		}
		case CBL_CMD_DIS_WRITE_PROT:
		{
			eCode = cmdHandle_ChangeWriteProt(p, OB_WRPSTATE_DISABLE);
			break;
		}
		case CBL_CMD_MEM_READ:
		{
			eCode = cmdHandle_MemRead(p);
			break;
		}
		case CBL_CMD_READ_SECT_PROT_STAT:
		{
			eCode = cmdHandle_GetWriteProt(p);
			break;
		}
		case CBL_CMD_GET_OTP_BYTES:
		{
			eCode = cmdHandle_GetOTPBytes(p);
			break;
		}
		case CBL_CMD_FLASH_WRITE:
		{
			eCode = cmdHandle_FlashWrite(p);
			break;
		}
		case CBL_CMD_EXIT:
		{
			eCode = cmdHandle_Exit(p);
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

static CBL_ErrCode_t sendToHost(const char *buf, size_t len)
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

static CBL_ErrCode_t recvFromHost(out char *buf, size_t len)
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

static CBL_ErrCode_t stopRecvFromHost(void)
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

static CBL_ErrCode_t sysState_Error(CBL_ErrCode_t eCode)
{
	DEBUG("Started\r\n");

	/* Turn off all LEDs except red */
	LED_OFF(ORANGE);
	LED_OFF(BLUE);
	LED_OFF(GREEN);

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
			sendToHost(msg, strlen(msg));
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
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_NEED_PARAM:
		{
			char msg[] = "\r\nERROR: Missing parameter(s)\r\n";
			INFO("Command is missing parameter(s)");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_JUMP_INV_ADDR:
		{
			char msg[] =
					"\r\nERROR: Invalid address\r\n"
							"Jumpable regions: FLASH, SRAM1, SRAM2, CCMRAM, BKPSRAM, SYSMEM and EXTMEM (if connected)\r\n";

			INFO("Invalid address inputed for jumping\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_SECTOR:
		{
			char msg[] = "\r\nERROR: Internal error while erasing sectors\r\n";

			WARNING("Error while erasing sectors\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_INV_SECT:
		{
			char msg[] = "\r\nERROR: Wrong sector given\r\n";

			INFO("Wrong sector given\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_INV_SECT_COUNT:
		{
			char msg[] = "\r\nERROR: Wrong sector count given\r\n";

			INFO("Wrong sector count given\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_WRITE_INV_ADDR:
		{
			char msg[] = "\r\nERROR: Invalid address range entered\r\n";

			INFO("Invalid address range entered for writing\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_WRITE_TOO_BIG:
		{
			char msg[] = "\r\nERROR: Inputed too big value\r\n";

			INFO("User requested to write a too big chunk\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_HAL_WRITE:
		{
			char msg[] = "\r\nERROR: Error while writing to flash\r\n";

			INFO("Error while writing to flash on HAL level\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_ERASE_INV_TYPE:
		{
			char msg[] = "\r\nERROR: Invalid erase type\r\n";

			INFO("User entered invalid erase type\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_HAL_ERASE:
		{
			char msg[] = "\r\nERROR: HAL error while erasing sectors \r\n";

			INFO("HAL error while erasing sector\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_HAL_UNLOCK:
		{
			char msg[] = "\r\nERROR: Unlocking flash failed\r\n";

			WARNING("Unlocking flash with HAL failed\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_INV_PARAM:
		{
			ERROR("Wrong parameter sent to a function\r\n");
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_NOT_DIG:
		{
			char msg[] = "\r\nERROR: Number parameter contains letters\r\n";

			WARNING("User entered number parameter containing letters\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		case CBL_ERR_1ST_NOT_ZERO:
		{
			char msg[] =
					"\r\nERROR: Number parameter must have '0' at the start when 'x' is present\r\n";

			WARNING("User entered number parameter with 'x', but not '0' on index 0\r\n");
			sendToHost(msg, strlen(msg));
			eCode = CBL_ERR_OK;
			break;
		}
		default:
		{
			ERROR("Unhandled error happened\r\n");
			break;
		}
	}

	return eCode;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == pUARTCmd)
	{
		gRxCmdCntr++;
	}
}

/*********************************************************/
/**Function handles**/
/*********************************************************/

/**
 * @brief
 * @return
 */
static CBL_ErrCode_t cmdHandle_Version(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char verbuf[12] = CBL_VERSION;

	DEBUG("Started\r\n");

	/* End with a new line */
	strlcat(verbuf, CRLF, 12);

	/* Send response */
	eCode = sendToHost(verbuf, strlen(verbuf));

	return eCode;
}

static CBL_ErrCode_t cmdHandle_Help(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	DEBUG("Started\r\n");

	/* Send response */
	eCode = sendToHost(helpPrintout, strlen(helpPrintout));

	return eCode;
}

static CBL_ErrCode_t cmdHandle_Cid(CBL_Parser_t *p)
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
	eCode = sendToHost(cid, strlen(cid));

	return eCode;
}

/**
 * @brief	RDP - Read protection
 * 				- Used to protect the software code stored in Flash memory.
 * 				- Reference manual - p. 93 - Explanation of RDP
 */
static CBL_ErrCode_t cmdHandle_GetRDPLvl(CBL_Parser_t *p)
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
	eCode = sendToHost(buf, strlen(buf));

	return eCode;
}

static CBL_ErrCode_t cmdHandle_GetOTPBytes(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	size_t len = FLASH_OTP_END - FLASH_OTP_BASE + 1;

	DEBUG("Started\r\n");

	/* Read len bytes from FLASH_OTP_BASE */

	ERR_CHECK(eCode);

	/* Send response */
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	return eCode;
}

static CBL_ErrCode_t cmdHandle_JumpTo(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char *charAddr;
	uint32_t addr = 0;
	void (*jump)(void);

	DEBUG("Started\r\n");

	/* Get the address in hex form */
	charAddr = parser_GetVal(p, CBL_TXTCMD_JUMP_TO_ADDR, strlen(CBL_TXTCMD_JUMP_TO_ADDR));
	if (charAddr == NULL)
		return CBL_ERR_NEED_PARAM;

	/* Fill addr, skips 0x if present */
	eCode = str2ui32(charAddr, strlen(charAddr), &addr, 16);
	ERR_CHECK(eCode);

	/* Make sure we can jump to the wanted location */
	eCode = verifyJumpAddress(addr);
	ERR_CHECK(eCode);

	/* Add one to the address to set the T bit */
	addr++;
	/**	T bit is 0th bit of a function address and tells the processor
	 *	if command is ARM T=0 or thumb T=1. STM uses thumb commands.
	 *	@ref https://www.youtube.com/watch?v=VX_12SjnNhY */

	/* Make a function to jump to */
	jump = (void *)addr;

	/* Send response */
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	ERR_CHECK(eCode);

	/* Jump to requested address, user ensures requested address is valid */
	jump();
	return eCode;
}

/**
 * @note	Sending sect=64 erases whole flash
 */
static CBL_ErrCode_t cmdHandle_FlashErase(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char *charSect, *charCount, *type;
	HAL_StatusTypeDef HALCode;
	uint32_t sectorCode, sect, count;
	FLASH_EraseInitTypeDef settings;

	DEBUG("Started\r\n");

	/* Device operating range: 2.7V to 3.6V */
	settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	/* Only available bank */
	settings.Banks = FLASH_BANK_1;

	type = parser_GetVal(p, CBL_TXTCMD_FLASH_ERASE_TYPE, strlen(CBL_TXTCMD_FLASH_ERASE_TYPE));
	if (type == NULL)
		/* No sector present throw error */
		return CBL_ERR_NEED_PARAM;

	/* Check the type of erase */
	if (strncmp(type, CBL_TXTCMD_FLASH_ERASE_TYPE_SECT, strlen(CBL_TXTCMD_FLASH_ERASE_TYPE_SECT)) == 0)
	{
		/* Set correct erase type */
		settings.TypeErase = FLASH_TYPEERASE_SECTORS;

		/* Get first sector to write to */
		charSect = parser_GetVal(p, CBL_TXTCMD_FLASH_ERASE_SECT, strlen(CBL_TXTCMD_FLASH_ERASE_SECT));
		if (charSect == NULL)
			/* No sector present throw error */
			return CBL_ERR_NEED_PARAM;

		/* Fill sect */
		eCode = str2ui32(charSect, strlen(charSect), &sect, 10);
		ERR_CHECK(eCode);

		/* Check validity of given sector */
		if (sect >= FLASH_SECTOR_TOTAL)
		{
			return CBL_ERR_INV_SECT;
		}

		/* Get how many sectors to erase */
		charCount = parser_GetVal(p, CBL_TXTCMD_FLASH_ERASE_COUNT,
				strlen(CBL_TXTCMD_FLASH_ERASE_COUNT));
		if (charCount == NULL)
			/* No sector count present throw error */
			return CBL_ERR_NEED_PARAM;

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
	else if (strncmp(type, CBL_TXTCMD_FLASH_ERASE_TYPE_MASS, strlen(CBL_TXTCMD_FLASH_ERASE_TYPE_MASS))
			== 0)
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
		return CBL_ERR_HAL_UNLOCK;

	/* Erase selected sectors */
	HALCode = HAL_FLASHEx_Erase( &settings, &sectorCode);

	/* Turn off the blue LED */
	LED_OFF(BLUE);

	/* Lock flash control registers */
	HAL_FLASH_Lock();

	/* Check for errors */
	if (HALCode != HAL_OK)
		return CBL_ERR_HAL_ERASE;

	if (sectorCode != 0xFFFFFFFFU) /*!< 0xFFFFFFFFU means success */
		/* Shouldn't happen as we check for HALCode before, but let's check */
		return CBL_ERR_SECTOR;

	/* Send response */
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));

	return eCode;
}

static CBL_ErrCode_t cmdHandle_FlashWrite(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char buf[CBL_FLASH_WRITE_SZ] = { 0 }, *charStart, *charLen;
	uint32_t start, len;
	HAL_StatusTypeDef HALCode;

	DEBUG("Started\r\n");

	/* Get starting address */
	charStart = parser_GetVal(p, CBL_TXTCMD_FLASH_WRITE_START, strlen(CBL_TXTCMD_FLASH_WRITE_START));
	if (charStart == NULL)
		return CBL_ERR_NEED_PARAM;

	/* Get length in bytes */
	charLen = parser_GetVal(p, CBL_TXTCMD_FLASH_WRITE_COUNT, strlen(CBL_TXTCMD_FLASH_WRITE_COUNT));
	if (charLen == NULL)
		return CBL_ERR_NEED_PARAM;

	/* Fill start */
	eCode = str2ui32(charStart, strlen(charStart), &start, 16);
	ERR_CHECK(eCode);

	/* Fill len */
	eCode = str2ui32(charLen, strlen(charLen), &len, 10);
	ERR_CHECK(eCode);

	/* Check validity of input addresses */
	if (IS_FLASH_ADDRESS(start) == false || IS_FLASH_ADDRESS(start + len) == false)
		return CBL_ERR_WRITE_INV_ADDR;

	/* Check if len is too big  */
	if (len > CBL_FLASH_WRITE_SZ)
		return CBL_ERR_WRITE_TOO_BIG;

	/* Reset UART byte counter */
	gRxCmdCntr = 0;

	/* Notify host bootloader is ready to receive bytes */
	sendToHost(CBL_TXTRESP_FLASH_WRITE_READY, strlen(CBL_TXTRESP_FLASH_WRITE_READY));

	/* Request 'len' bytes */
	eCode = recvFromHost(buf, len);
	ERR_CHECK(eCode);

	/* Wait for 'len' bytes */
	while (gRxCmdCntr != 1)
	{
	}

	/* Write data to flash */
	LED_ON(BLUE);

	/* Unlock flash */
	if (HAL_FLASH_Unlock() != HAL_OK)
		return CBL_ERR_HAL_UNLOCK;

	/* Write to flash */
	for (int i = 0; i < len; i++)
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
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	return eCode;
}

static CBL_ErrCode_t cmdHandle_MemRead(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	DEBUG("Started\r\n");

	/* Send response */
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	return eCode;
}

/**
 * @brief		Enables write protection on individual flash sectors
 * @param EnDis	Write protection state: OB_WRPSTATE_ENABLE or OB_WRPSTATE_DISABLE
 */
static CBL_ErrCode_t cmdHandle_ChangeWriteProt(CBL_Parser_t *p, uint32_t EnDis)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	char *charMask;
	uint32_t mask = 0;
	FLASH_OBProgramInitTypeDef pOBInit;

	DEBUG("Started\r\n");

	/* Assert parameter */
	if (EnDis != OB_WRPSTATE_ENABLE && EnDis != OB_WRPSTATE_DISABLE)
	{
		return CBL_ERR_INV_PARAM;
	}

	/* Mask of sectors to affect */
	charMask = parser_GetVal(p, CBL_TXTCMD_EN_WRITE_PROT_MASK, strlen(CBL_TXTCMD_EN_WRITE_PROT_MASK));

	if (charMask == NULL)
		return CBL_ERR_NEED_PARAM;

	/* Fill mask */
	eCode = str2ui32(charMask, strlen(charMask), &mask, 16); /*!< Mask is in hex */
	ERR_CHECK(eCode);

	/* Put non nWRP bits to 0 */
	mask &= (FLASH_OPTCR_nWRP_Msk >> FLASH_OPTCR_nWRP_Pos);

	/* Unlock option byte configuration */
	if (HAL_FLASH_OB_Unlock() != HAL_OK)
		return CBL_ERR_HAL_UNLOCK;

	/* Wait for past flash operations to be done */
	FLASH_WaitForLastOperation(50000U); /*!< 50 s as in other function references */

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
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	return eCode;
}

/**
 * @brief	Gets write protection status of all sectors in binary form
 */
static CBL_ErrCode_t cmdHandle_GetWriteProt(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	FLASH_OBProgramInitTypeDef OBInit;
	uint16_t invWRPSector;
	char buf[FLASH_SECTOR_TOTAL + 3] = { 0 };

	DEBUG("Started\r\n");

	/* Unlock option byte configuration */
	if (HAL_FLASH_OB_Unlock() != HAL_OK)
		return CBL_ERR_HAL_UNLOCK;

	/* Get option bytes */
	HAL_FLASHEx_OBGetConfig( &OBInit);

	/* Lock option byte configuration */
	HAL_FLASH_OB_Lock();

	/* Invert WRPSector as we want 1 to represent protected */
	invWRPSector = (uint16_t) ~OBInit.WRPSector & (FLASH_OPTCR_nWRP_Msk >> FLASH_OPTCR_nWRP_Pos);

	/* Fill the buffer with binary data */
	ui2binstr(invWRPSector, buf, FLASH_SECTOR_TOTAL);

	/* Send response */
	eCode = sendToHost(buf, strlen(buf));
	return eCode;
}

static CBL_ErrCode_t cmdHandle_Exit(CBL_Parser_t *p)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	DEBUG("Started\r\n");

	isExitReq = true;

	/* Send response */
	eCode = sendToHost(CBL_TXT_SUCCESS, strlen(CBL_TXT_SUCCESS));
	return eCode;
}

/**
 * @brief		Converts string containing only number (e.g. 0A3F or 0x0A3F) to uint32_t
 *
 * @param s		String to convert
 * @param len	Length of s
 * @param num	Output number
 * @param base	Base of digits string is written into, supported 10 or 16 only
 */
static CBL_ErrCode_t str2ui32(const char *s, size_t len, out uint32_t *num, uint8_t base)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;

	eCode = verifyDigitsOnly(s, len, base);
	ERR_CHECK(eCode);

	*num = (uint8_t)strtoul(s, NULL, base);
	return eCode;
}

static CBL_ErrCode_t verifyDigitsOnly(const char *s, size_t len, uint8_t base)
{
	size_t i = len;

	/* If base is 16, index 1 'x' or 'X' then index 0 must be '0' */
	if (base == 16 && (tolower(s[1]) == 'x') && s[0] != '0')
		return CBL_ERR_1ST_NOT_ZERO;

	while (i)
	{
		i--;
		if (base == 10)
		{
			if (isdigit(s[--i]) == 0)
				return CBL_ERR_NOT_DIG;
		}
		else if (base == 16)
		{
			/* Index 1 can be a number or 'x' or 'X', other just number */
			if ( (i != 1 || (tolower(s[i]) != 'x')) && isxdigit(s[i]) == 0)
				return CBL_ERR_NOT_DIG;
		}
		else
			return CBL_ERR_UNSUP_BASE;
	}
	return CBL_ERR_OK;
}

/**
 * @brief	Verifies address is in jumpable region
 * @note	Jumping to peripheral memory locations not permitted
 */
static CBL_ErrCode_t verifyJumpAddress(uint32_t addr)
{
	CBL_ErrCode_t eCode = CBL_ERR_JUMP_INV_ADDR;

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
 * @brief			Convert uint32_t to binary string
 * @param str		User must ensure it is at least 'numofbits' + 3 bytes long
 * @param numofbits	Number of bits from num to convert to str
 */
static void ui2binstr(uint32_t num, out char *str, uint8_t numofbits)
{
	bool bit;
	char i;

	/* Set num of bits to walk through */
	i = numofbits;

	*str++ = '0';
	*str++ = 'b';

	do
	{
		i--;
		/* exclude only ith bit */
		bit = (bool) ( (num >> i) & 0x0001);

		/* Convert to ASCII value and save*/
		*str++ = (char)bit + '0';
	}
	while (i);

	*str = '\0';
}


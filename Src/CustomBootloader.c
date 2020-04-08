#include "CustomBootLoader.h"

static void cbl_ShellInit(void);
static void cbl_RunUserApp(void);
static CBL_ErrCode_t cbl_RunShellSystem(void);
static CBL_ErrCode_t cbl_StateOperation(void);
static CBL_ErrCode_t cbl_WaitForCmd(out char* buf, size_t len);
static CBL_ErrCode_t cbl_ParseCmd(char *cmd, size_t len, out CBL_Parser_t *p);
static CBL_ErrCode_t cbl_EnumCmd(out char* buf, size_t len, out CBL_CMD_t *cmdCode);
static CBL_ErrCode_t cbl_SendToHost(out char *buf, size_t len);
static CBL_ErrCode_t cbl_RecvFromHost(out char *buf, size_t len);
//static CBL_Err_Code_t cbl_StopRecvFromHost();
static CBL_ErrCode_t cbl_StateError(CBL_ErrCode_t eCode);

static uint32_t cntrRecvChar = 0;

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
	"      User manual is present under /docs     \r\n"
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

	DEBUG("MSP value: %#x\r\n", (unsigned int ) msp_value);
	/* function from CMSIS */
	__set_MSP(msp_value);

	addressRstHndl = *(volatile uint32_t *) (CBL_ADDR_USERAPP + 4);
	DEBUG("Reset handler address: %#x\r\n", (unsigned int ) addressRstHndl);

	userAppResetHandler = (void *)addressRstHndl;
	INFO("Jumping to user application\r\n");
	userAppResetHandler();
}
/**
 * @brief	Runs the shell for the bootloader.
 * @return	CBL_ERR_NO when no error, else returns an error code.
 */
static CBL_ErrCode_t cbl_RunShellSystem(void)
{
	CBL_ErrCode_t eCode = CBL_ERR_OK;
	bool isExitReq = false;
	CBL_sysStates_t state = CBL_STAT_ERR, nextState;

	INFO("Starting bootloader\r\n");

	nextState = state;
	cbl_ShellInit();

	while (isExitReq == false)
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

				/* Switch state if needed */
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
				isExitReq = true;
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

//	eCode = cbl_HandleCmd();
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
	/* If DMA fills the buffer and no command is received throw an error */
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
 * @param p		Function assumes empty parser p
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
	/* Command name ends with ' ' */
	*pSpa = '\0';

	for (i = 0; i < CBL_MAX_ARGS; i++)
	{

		/* Find an end of the param name */
		pEqu = memchr(pSpa, '=', pLastChar - pSpa);

		if (pEqu == NULL)
		{
			/* Exit from the loop as there is no value for the argument */
			break;
		}

		/* Argument starts after ' ' */
		p->args[i][CBL_ARG_NAME] = (pSpa + 1);
		/* Arguments end with '=' */
		*pEqu = '\0';

		/* Parameter value starts after '=' */
		p->args[i][CBL_ARG_VAL] = (pEqu + 1);

		/* Find the next space */
		pSpa = memchr(pEqu, ' ', pLastChar - pEqu);

		if (pSpa == NULL)
		{
			/* Exit the loop as there are parameters */
			break;
		}
		/* Argument value ends with ' ' */
		*pSpa = '\0';
	}

	p->cmd = cmd;
	p->len = len;
	p->numOfArgs = i;

	return eCode;
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
	else if (len == strlen(CBL_TXTCMD_RDP_STATUS)
			&& strncmp(buf, CBL_TXTCMD_RDP_STATUS, strlen(CBL_TXTCMD_RDP_STATUS)) == 0)
	{
		*cmdCode = CBL_CMD_RDP_STATUS;
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

static CBL_ErrCode_t cbl_SendToHost(out char *buf, size_t len)
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

	switch (eCode)
	{
		case CBL_ERR_OK:
		{
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

			break;
		}
		case CBL_ERR_STATE:
		{

			break;
		}
		case CBL_ERR_HAL_TX:
		{
			WARNING("HAL transmit error happened\r\n");
			break;
		}
		case CBL_ERR_HAL_RX:
		{
			WARNING("HAL receive error happened\r\n");
			break;
		}
		case CBL_ERR_RX_ABORT:
		{
			WARNING("Error happened while aborting receive\r\n");
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


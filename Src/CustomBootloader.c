#include "CustomBootLoader.h"

static void cbl_shellInit(void);
static void cbl_runUserApp(void);
static CBL_Err_Code_t cbl_runShellSystem(void);
static CBL_sysStates_t cbl_stateOperation(CBL_sysStates_t *nextState);
static CBL_sysStates_t cbl_stateError(CBL_sysStates_t *nextState, CBL_Err_Code_t eCode);

void CBL_init()
{
	CBL_Err_Code_t eCode = CBL_ERR_NO;
	INFO("Custom bootloader started\r\n");
	if (HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
	{
		INFO("Blue button pressed...\r\n");
		eCode = cbl_runShellSystem();
	}
	else
	{
		INFO("Blue button not pressed...\r\n");
	}
	ASSERT(eCode == CBL_ERR_NO, "ErrCode:%d:Restart the application.\r\n", eCode);
	cbl_runUserApp();
	ERROR("Switching to user application failed\r\n");
}

static void cbl_shellInit(void)
{
/// TODO move init UART here
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
static void cbl_runUserApp(void)
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
CBL_Err_Code_t cbl_runShellSystem(void)
{
	CBL_Err_Code_t eCode = CBL_ERR_NO;
	bool exit = false;

	INFO("Starting bootloader\r\n");
	cbl_shellInit();

	while (exit == false)
	{
		CBL_sysStates_t state = CBL_STAT_ERR, nextState;

		switch (state)
		{
			case CBL_STAT_OPER:
			{
				cbl_stateOperation( &nextState);
				break;
			}
			case CBL_STAT_ERR:
			{
				eCode = cbl_stateError( &nextState, eCode);
				if (eCode != CBL_ERR_NO)
				{
					nextState = CBL_STAT_EXIT;
				}
				break;
			}
			case CBL_STAT_EXIT:
			{
				/* deconstruct needed */
				exit = true;
				break;
			}
			default:
			{
				eCode = CBL_ERR_NO;
				break;
			}
		}
		state = nextState;
	}
	return eCode;
}

static CBL_sysStates_t cbl_stateOperation(CBL_sysStates_t *nextState)
{
	CBL_Err_Code_t eCode = CBL_ERR_NO;
	return eCode;
}

static CBL_sysStates_t cbl_stateError(CBL_sysStates_t *nextState, CBL_Err_Code_t eCode)
{
	return eCode;
}


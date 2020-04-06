#include "CustomBootLoader.h"

static void cbl_initShell(void);
static void cbl_goToUserApp(void);

void CBL_init()
{
	INFO("Custom bootloader started\r\n");
	INFO("v0.1\r\n");
	if (HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
	{
		INFO("Blue button pressed... Staying in bootloader mode\r\n");
		cbl_initShell();
	}
	else
	{
		INFO("Blue button not pressed... Jumping to user app\r\n");
		cbl_goToUserApp();
	}
}

static void cbl_initShell(void)
{

}

static void cbl_goToUserApp(void)
{

}

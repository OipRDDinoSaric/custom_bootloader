#include "CustomBootLoader.h"


void CBL_init()
{
	INFO("Custom bootloader started\r\n");
	INFO("v0.1\r\n");
	if(HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
	{
		INFO("Blue button pressed...Staying in bootloader mode\r\n");
	}
	else
	{
		INFO("Blue button not pressed...Jumping to user app\r\n");
	}
}

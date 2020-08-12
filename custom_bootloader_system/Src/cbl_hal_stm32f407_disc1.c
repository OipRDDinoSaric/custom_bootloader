/** @file cbl_hal_stm32f407_disc1.c
 *
 * @brief HAL level function wrap for stm32f407 disc1 development board
 */
#include "cbl_hal_stm32f407_disc1.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx_hal.h"

#define LED_GREEN_Pin            GPIO_PIN_12
#define LED_GREEN_GPIO_Port      GPIOD
#define LED_ORANGE_Pin           GPIO_PIN_13
#define LED_ORANGE_GPIO_Port     GPIOD
#define LED_RED_Pin              GPIO_PIN_14
#define LED_RED_GPIO_Port        GPIOD
#define LED_BLUE_Pin             GPIO_PIN_15
#define LED_BLUE_GPIO_Port       GPIOD

#define BTN_BLUE_Pin             GPIO_PIN_0
#define BTN_BLUE_GPIO_Port       GPIOA

/* Colors: RED, ORANGE, GREEN and BLUE */
#define LED_ON(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port,               \
                                            LED_##COLOR##_Pin, GPIO_PIN_SET);
#define LED_OFF(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port,              \
                                            LED_##COLOR##_Pin, GPIO_PIN_RESET);

/**
 * @brief Switches requested colored LED on
 *
 * @param led_color LED color to switch
 */
void hal_led_on (cbl_led_color_t led_color)
{
    switch (led_color)
    {
        case LED_GREEN:
        {
            LED_ON(GREEN);
        }
        break;

        case LED_ORANGE:
        {
            LED_ON(ORANGE);
        }
        break;

        case LED_RED:
        {
            LED_ON(RED);
        }
        break;

        case LED_BLUE:
        {
            LED_ON(BLUE);
        }
        break;

        default:
        {
            /* Ignore */
        }
        break;
    }
}

/**
 * @brief Switches requested colored LED off
 *
 * @param led_color LED color to switch
 */
void hal_led_off (cbl_led_color_t led_color)
{
    switch (led_color)
    {
        case LED_GREEN:
        {
            LED_OFF(GREEN);
        }
        break;

        case LED_ORANGE:
        {
            LED_OFF(ORANGE);
        }
        break;

        case LED_RED:
        {
            LED_OFF(RED);
        }
        break;

        case LED_BLUE:
        {
            LED_OFF(BLUE);
        }
        break;

        default:
        {
            /* Ignore */
        }
        break;
    }
}
/**
 * @brief Gets the state of blue button
 *
 * @return State of blue button
 */
bool hal_blue_btn_state_get (void)
{

    if (HAL_GPIO_ReadPin(BTN_BLUE_GPIO_Port, BTN_BLUE_Pin) == GPIO_PIN_SET)
    {
        return true;
    }
    return false;
}

/*** end of file ***/

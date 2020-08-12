/** @file cbl_hal_stm32f4xx.h
 *
 * @brief HAL level function wrap for stm32f4xx
 */
#ifndef CBL_HAL_STM32F4xx_H
#define CBL_HAL_STM32F4xx_H
#include "etc/cbl_common.h"

typedef enum
{
    LED_GREEN = 0,
    LED_ORANGE,
    LED_RED,
    LED_BLUE
} cbl_led_color_t;

#define LED_MEMORY   LED_BLUE
#define LED_POWER_ON LED_RED
#define LED_READY    LED_GREEN
#define LED_BUSY     LED_ORANGE

void hal_periph_init(void);
void hal_init(void);
void hal_system_restart(void);
cbl_err_code_t hal_write_prot_get (char * write_prot, uint32_t len);
void hal_rdp_lvl_get (char * rdp_lvl, uint32_t len);
cbl_err_code_t hal_change_write_prot (uint32_t mask, bool EnDis);
cbl_err_code_t hal_write_program_bytes (uint32_t addr, uint8_t * data,
        uint32_t len);
cbl_err_code_t hal_flash_erase_sector (uint32_t sect, uint32_t count);
cbl_err_code_t hal_flash_erase_mass (void);
cbl_err_code_t hal_verify_flash_address (uint32_t addr);
cbl_err_code_t hal_verify_jump_address (uint32_t addr);
cbl_err_code_t hal_send_to_host (const char * buf, size_t len);
cbl_err_code_t hal_recv_from_host_start (uint8_t * buf, size_t len);
cbl_err_code_t hal_recv_from_host_stop (void);
void hal_vtor_set (uint32_t new_vtor);
void hal_msp_set (uint32_t topOfMainStack);
uint32_t hal_id_code_get (void);
void hal_led_on (cbl_led_color_t led_color); /* Specific to implementation */
void hal_led_off (cbl_led_color_t led_color); /* Specific to implementation */
bool hal_blue_btn_state_get (void); /* Specific to implementation */
#endif /* CBL_HAL_STM32F4xx_H */
/*** end of file ***/

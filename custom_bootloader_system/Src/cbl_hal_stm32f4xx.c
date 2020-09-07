/** @file cbl_hal_stm32f4xx.c
 *
 * @brief HAL level function wrap for stm32f4xx
 */
#include "cbl_hal_stm32f4xx.h"
#include <stm32f4xx_hal.h>
#include <crc.h>
#include <dma.h>
#include <gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <usart.h>

#define pUARTCmd &huart2 /*!< UART used for shell communication */

/* Missing address locations in stm32f407xx.h */
#define SRAM1_END   0x2001BFFFUL
#define SRAM2_END   0x2001FFFFUL
#define BKPSRAM_END 0x40024FFFUL
#define SYSMEM_BASE 0x2001FFFFUL
#define SYSMEM_END  0x1FFF77FFUL

#define IS_CCMDATARAM_ADDRESS(x) (((x) >= CCMDATARAM_BASE) &&               \
                                                    ((x) <= CCMDATARAM_END))
#define IS_SRAM1_ADDRESS(x)      (((x) >= SRAM1_BASE) && ((x) <= SRAM1_END))
#define IS_SRAM2_ADDRESS(x)      (((x) >= SRAM2_BASE) && ((x) <= SRAM2_END))
#define IS_BKPSRAM_ADDRESS(x)    (((x) >= BKPSRAM_BASE) && ((x) <= BKPSRAM_END))
#define IS_SYSMEM_ADDRESS(x)     (((x) >= SYSMEM_BASE) && ((x) <= SYSMEM_END))

#define PC14_OSC32_IN_Pin        GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port  GPIOC
#define PC15_OSC32_OUT_Pin       GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin           GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port     GPIOH
#define PH1_OSC_OUT_Pin          GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port    GPIOH
#define BOOT1_Pin                GPIO_PIN_2
#define BOOT1_GPIO_Port          GPIOB
#define DBG_RX_Pin               GPIO_PIN_9
#define DBG_RX_GPIO_Port         GPIOD
#define SWDIO_Pin                GPIO_PIN_13
#define SWDIO_GPIO_Port          GPIOA
#define SWCLK_Pin                GPIO_PIN_14
#define SWCLK_GPIO_Port          GPIOA
#define SWO_Pin                  GPIO_PIN_3
#define SWO_GPIO_Port            GPIOB

#define CMD_TX_Pin               GPIO_PIN_2
#define CMD_TX_GPIO_Port         GPIOA
#define CMD_RX_Pin               GPIO_PIN_3
#define CMD_RX_GPIO_Port         GPIOA

/**
 * @brief Initializes HAL library
 */
void hal_init (void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
}

/**
 * @brief Initialize all configured peripherals
 */
void hal_periph_init(void)
{
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CRC_Init();
    MX_USART2_UART_Init();
}

void hal_system_restart (void)
{
    NVIC_SystemReset();
    /* Never returns */
}

void hal_deinit(void)
{
    HAL_RCC_DeInit();
    HAL_DeInit();
}

void hal_stop_systick(void)
{
    // Stop sys tick
    uint32_t temp = SysTick->CTRL;
    temp &= ~0x02;
    SysTick->CTRL = temp;
}

void hal_disable_interrupts(void)
{
    // stop all interrupts
    __disable_irq();

   // Disable IRQs
   for(int i = 0;i < 8;i++)
   {
       NVIC->ICER[i] = 0xFFFFFFFF;
   }
   // Clear pending IRQs
   for(int i = 0;i < 8;i++)
   {
       NVIC->ICPR[i] = 0xFFFFFFFF;
   }
}
/**
 * @brief Fills the char array with write protection. LSB corresponds to
 *        sector 0
 *
 * @param write_prot[out] Buffer to fill
 * @param len[in]         Size of 'write_prot'
 * @return
 */
cbl_err_code_t hal_write_prot_get (char * write_prot, uint32_t len)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    FLASH_OBProgramInitTypeDef OBInit;
    uint16_t invWRPSector;

    if (len < FLASH_SECTOR_TOTAL + 3)
    {
        return CBL_ERR_INV_PARAM;
    }

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
    ui2binstr(invWRPSector, write_prot, FLASH_SECTOR_TOTAL);

    return eCode;
}

/**
 * @brief Gets the read protection level
 *
 * @param rdp_lvl[out] Buffer for read protection level
 * @param len[in]      Length of 'rdp_lvl'
 */
void hal_rdp_lvl_get (char * rdp_lvl, uint32_t len)
{
    FLASH_OBProgramInitTypeDef optBytes;

    HAL_FLASHEx_OBGetConfig( &optBytes);

    /* Fill buffer with correct value of RDP */
    switch (optBytes.RDPLevel)
    {
        case OB_RDP_LEVEL_0:
        {
            strlcpy(rdp_lvl, "level 0", len);
        }
        break;

        case OB_RDP_LEVEL_2:
        {
            strlcpy(rdp_lvl, "level 2", len);
        }
        break;

        default:
        {
            /* Any other value is RDP level 1 */
            strlcpy(rdp_lvl, "level 1", len);
        }
        break;
    }

    strlcat(rdp_lvl, CRLF, len);
}

/**
 * @brief HAL level abstraction of change of write protection
 *
 * @param mask[in]  Choose what sector to change, 1 means change
 * @param EnDis[in] Enable or disable write protection on masked sectors
 */
cbl_err_code_t hal_change_write_prot (uint32_t mask, bool EnDis)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    uint32_t state;

    FLASH_OBProgramInitTypeDef pOBInit;

    if (true == EnDis)
    {
        state = OB_WRPSTATE_ENABLE;
    }
    else if (false == EnDis)
    {
        state = OB_WRPSTATE_DISABLE;
    }

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
    pOBInit.WRPState = state;

    /* Write new RWP state in the option bytes register */
    HAL_FLASHEx_OBProgram( &pOBInit);

    /* Process the change */
    HAL_FLASH_OB_Launch();

    /* Lock option byte configuration */
    HAL_FLASH_OB_Lock();

    return eCode;
}

/**
 * @brief Uses HAL level commands to write data to memory
 * @param addr[in] Starting address to write bytes to
 * @param data[in] Array of bytes to be writen
 * @param len[in]  Length of 'data'd
 */
cbl_err_code_t hal_write_program_bytes (uint32_t addr, uint8_t * data,
        uint32_t len)
{
    HAL_StatusTypeDef HALCode;

    /* Unlock flash */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Write to flash */
    for (uint32_t iii = 0u; iii < len; iii++)
    {
        /* Write a byte */
        HALCode = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + iii,
                data[iii]);
        if (HALCode != HAL_OK)
        {
            HAL_FLASH_Lock();
            return CBL_ERR_HAL_WRITE;
        }
    }

    HAL_FLASH_Lock();
    return CBL_ERR_OK;
}

/**
 * @brief Erase flash sectors
 *
 * @param sect Initial sector to erase
 * @param count Number of sectors to erase
 */
cbl_err_code_t hal_flash_erase_sector (uint32_t sect, uint32_t count)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    FLASH_EraseInitTypeDef settings;
    HAL_StatusTypeDef HALCode;
    uint32_t sectorCode;

    /* Check validity of given sector */
    if (sect >= FLASH_SECTOR_TOTAL)
    {
        return CBL_ERR_INV_SECT;
    }

    if (sect + count - 1 >= FLASH_SECTOR_TOTAL)
    {
        /* Last sector to delete doesn't exist, throw error */
        return CBL_ERR_INV_SECT_COUNT;
    }

    /* Device operating range: 2.7V to 3.6V */
    settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    /* Only available bank */
    settings.Banks = FLASH_BANK_1;
    settings.TypeErase = FLASH_TYPEERASE_SECTORS;
    settings.Sector = sect;
    settings.NbSectors = count;

    /* Turn on the blue LED, signalizing flash manipulation */
    hal_led_on(LED_MEMORY);

    /* Unlock flash control registers */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Erase selected sectors */
    HALCode = HAL_FLASHEx_Erase( &settings, &sectorCode);

    hal_led_off(LED_MEMORY);

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

    return eCode;
}

/**
 * @brief Erase whole flash
 */
cbl_err_code_t hal_flash_erase_mass (void)
{
    cbl_err_code_t eCode = CBL_ERR_OK;
    FLASH_EraseInitTypeDef settings;
    HAL_StatusTypeDef HALCode;
    uint32_t sectorCode;

    /* Device operating range: 2.7V to 3.6V */
    settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    /* Only available bank */
    settings.Banks = FLASH_BANK_1;

    /* Erase all sectors */
    settings.TypeErase = FLASH_TYPEERASE_MASSERASE;

    /* Turn on the blue LED, signalizing flash manipulation */
    hal_led_on(LED_MEMORY);

    /* Unlock flash control registers */
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return CBL_ERR_HAL_UNLOCK;
    }

    /* Erase selected sectors */
    HALCode = HAL_FLASHEx_Erase( &settings, &sectorCode);

    hal_led_off(LED_MEMORY);

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

    return eCode;
}

/**
 * @brief   Verifies address is in jumpable region
 *
 * @note    Jumping to peripheral memory locations is NOT permitted
 */
cbl_err_code_t hal_verify_jump_address (uint32_t addr)
{
    cbl_err_code_t eCode = CBL_ERR_JUMP_INV_ADDR;

    if (IS_FLASH_ADDRESS(addr) || IS_CCMDATARAM_ADDRESS(addr)
    || IS_SRAM1_ADDRESS(addr) || IS_SRAM2_ADDRESS(addr)
    || IS_BKPSRAM_ADDRESS(addr) || IS_SYSMEM_ADDRESS(addr))
    {
        eCode = CBL_ERR_OK;
    }

    return eCode;
}

/**
 * @brief Verifies address of flash
 * @param addr Address to verify
 * @return CBL_ERR_WRITE_INV_ADDR on error, otherwise OK
 */
cbl_err_code_t hal_verify_flash_address (uint32_t addr)
{
    if (IS_FLASH_ADDRESS(addr) == false)
    {
        return CBL_ERR_WRITE_INV_ADDR;
    }
    return CBL_ERR_OK;
}

/**
 * @brief           Sends a message to the host over defined output
 *
 * @param buf[in]   Message to send
 *
 * @param len[in]   Length of buf
 */
cbl_err_code_t hal_send_to_host (const char * buf, size_t len)
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
 */
cbl_err_code_t hal_recv_from_host_start (uint8_t * buf, size_t len)
{
    if (HAL_UART_Receive_DMA(pUARTCmd, buf, len) == HAL_OK)
    {
        return CBL_ERR_OK;
    }
    else
    {
        return CBL_ERR_HAL_RX;
    }
}

/**
 * @brief Stops waiting of the command
 */
cbl_err_code_t hal_recv_from_host_stop (void)
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

/**
 * @brief Sets the vector table offset register
 *
 * @param new_vtor[in] New value
 */
void hal_vtor_set (uint32_t new_vtor)
{
    SCB->VTOR = new_vtor;
}

/**
 * @brief   Set Main Stack Pointer
 *
 * @details Assigns the given value to the Main Stack Pointer (MSP)
 *
 * @param topOfMainStack[in]  Main Stack Pointer value to set
 */
void hal_msp_set (uint32_t topOfMainStack)
{
    /* Function from CMSIS */
    __set_MSP(topOfMainStack);
}

/**
 * @brief Gets ID code of the MCU
 * @return ID code
 */
uint32_t hal_id_code_get (void)
{
    return DBGMCU->IDCODE;
}

/**
 * @brief Switches requested colored LED on
 *
 * @param led_color LED color to switch
 */
__weak void hal_led_on (cbl_led_color_t led_color)
{
    return;
}

/**
 * @brief Switches requested colored LED off
 *
 * @param led_color LED color to switch
 */
__weak void hal_led_off (cbl_led_color_t led_color)
{
    return;
}

/**
 * @brief Gets the state of blue button
 *
 * @return State of blue button
 */
__weak bool hal_blue_btn_state_get (void)
{
    return false;
}

/**
 * @brief           Interrupt when receiving is done from UART
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
/*** end of file ***/

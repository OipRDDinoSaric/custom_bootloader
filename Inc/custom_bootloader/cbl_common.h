/** @file cbl_common.h
 *
 * @brief File containing all function and variable definitions that are needed
 *        by function handlers and custom bootloader main file
 */
#ifndef CBL_CMDS_COMMON_H
#define CBL_CMDS_COMMON_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <custom_bootloader.h>
#include "main.h"

/* Colors: RED, ORANGE, GREEN and BLUE */
#define LED_ON(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port,               \
                                            LED_##COLOR##_Pin, GPIO_PIN_SET);
#define LED_OFF(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port,              \
                                            LED_##COLOR##_Pin, GPIO_PIN_RESET);

#ifndef NDEBUG
/**
 * Tutorial for semihosting to Console:
 * System Workbench for STM32 -> Help -> Help Contents -> Semihosting
 */
#   define INFO(f_, ...) printf("INFO:%s:", __func__); \
                         printf((f_), ##__VA_ARGS__)

#   define DEBUG(f_, ...) printf("DEBG:%s:", __func__); \
                          printf((f_), ##__VA_ARGS__)

#   define WARNING(f_, ...) printf("WARN:%s:", __func__); \
                            printf((f_), ##__VA_ARGS__)

#   define ERROR(f_, ...) printf("ERRO:%s:%d:%s:", __FILE__, \
                                        __LINE__, __func__); \
                          printf((f_), ##__VA_ARGS__)

#   define ASSERT(expr, f_, ...)        \
      do {                              \
        if (!(expr)) {                  \
          ERROR((f_), ##__VA_ARGS__);   \
          while(1);                     \
        }                               \
      } while (0)
#else /* #ifndef NDEBUG */

#   define INFO(f_, ...);
#   define DEBUG(f_, ...);
#   define WARNING(f_, ...);
#   define ERROR(f_, ...);
#   define ASSERT(expr, f_, ...);

#endif /* #ifndef NDEBUG */

#define MAX_ARGS 8 /*!< Maximum number of arguments in an input cmd */

#define TXT_SUCCESS "\r\nOK\r\n"
#define TXT_SUCCESS_HELP "\\r\\nOK\\r\\n" /*!< Used in help function */

#define TXT_RESP_FLASH_WRITE_READY "\r\nready\r\n"
#define TXT_RESP_FLASH_WRITE_READY_HELP "\\r\\nready\\r\\n" /*!< Used in help
                                                                    function */

#define CRLF "\r\n"

/* Missing address locations in stm32f407xx.h */
#define SRAM1_END 0x2001BFFFUL
#define SRAM2_END 0x2001FFFFUL
#define BKPSRAM_END 0x40024FFFUL
#define SYSMEM_BASE 0x2001FFFFUL
#define SYSMEM_END  0x1FFF77FFUL

#define IS_CCMDATARAM_ADDRESS(x) (((x) >= CCMDATARAM_BASE) &&               \
                                                    ((x) <= CCMDATARAM_END))
#define IS_SRAM1_ADDRESS(x) (((x) >= SRAM1_BASE) && ((x) <= SRAM1_END))
#define IS_SRAM2_ADDRESS(x) (((x) >= SRAM2_BASE) && ((x) <= SRAM2_END))
#define IS_BKPSRAM_ADDRESS(x) (((x) >= BKPSRAM_BASE) && ((x) <= BKPSRAM_END))
#define IS_SYSMEM_ADDRESS(x) (((x) >= SYSMEM_BASE) && ((x) <= SYSMEM_END))

#define ERR_CHECK(x)    do                          \
                        {                           \
                            if((x) != CBL_ERR_OK)   \
                            {                       \
                              return (x);           \
                            }                       \
                        }                           \
                        while(0)

extern volatile uint32_t gRxCmdCntr;

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

cbl_err_code_t parser_run (char * cmd, size_t len, parser_t * phPrsr);
char *parser_get_val (parser_t * phPrsr, char * name, size_t lenName);

cbl_err_code_t send_to_host (const char * buf, size_t len);
cbl_err_code_t recv_from_host (uint8_t * buf, size_t len);
cbl_err_code_t stop_recv_from_host (void);

cbl_err_code_t str2ui32 (const char * str, size_t len, uint32_t * num,
        uint8_t base);
cbl_err_code_t verify_digits_only (const char * str, size_t i, uint8_t base);
cbl_err_code_t verify_jump_address (uint32_t addr);
void ui2binstr (uint32_t num, char * str, uint8_t numofbits);

#endif /* CBL_CMDS_COMMON_H */
/*** end of file ***/

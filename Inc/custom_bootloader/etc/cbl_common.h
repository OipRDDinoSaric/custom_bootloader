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
#include "cbl_hal_stm32f407_disc1.h"

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

#   define ERROR(f_, ...) printf("ERRO:%s:", __func__); \
                          printf((f_), ##__VA_ARGS__)

#   define ASSERT(expr, f_, ...)        \
      do {                              \
        if (!(expr)) {                  \
          ERROR((f_), ##__VA_ARGS__);   \
          while(1);                     \
        }                               \
      } while (0)
#else /* #ifndef NDEBUG */

#   define INFO(f_, ...)         ((void)0U)
#   define DEBUG(f_, ...)        ((void)0U)
#   define WARNING(f_, ...)      ((void)0U)
#   define ERROR(f_, ...)        ((void)0U)
#   define ASSERT(expr, f_, ...) ((void)0U)

#endif /* #ifndef NDEBUG */

#define MAX_ARGS 8 /*!< Maximum number of arguments in an input cmd */

#define TXT_SUCCESS      "\r\nOK\r\n"
#define TXT_SUCCESS_HELP "\\r\\nOK\\r\\n" /*!< Used in help function */

#define TXT_RESP_FLASH_WRITE_READY      "\r\nready\r\n"
#define TXT_RESP_FLASH_WRITE_READY_HELP "\\r\\nready\\r\\n" /*!< Used in help
                                                                    function */
#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */

#define CRLF "\r\n"

#define ERR_CHECK(x)    do                          \
                        {                           \
                            if((x) != CBL_ERR_OK)   \
                            {                       \
                              return (x);           \
                            }                       \
                        }                           \
                        while(0)


extern volatile uint32_t gRxCmdCntr;
extern bool gIsExitReq;

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

cbl_err_code_t str2ui32 (const char * str, size_t len, uint32_t * num,
        uint8_t base);
cbl_err_code_t verify_digits_only (const char * str, size_t i, uint8_t base);
void ui2binstr (uint32_t num, char * str, uint8_t numofbits);
uint32_t ui32_min (uint32_t num1, uint32_t num2);
cbl_err_code_t two_hex_chars2ui8 (uint8_t high_half, uint8_t low_half,
        uint8_t * p_result);
cbl_err_code_t four_hex_chars2ui16 (uint8_t * array, uint32_t len,
        uint16_t * p_result);
cbl_err_code_t eight_hex_chars2ui32 (uint8_t * array, uint32_t len,
        uint32_t * p_result);

#endif /* CBL_CMDS_COMMON_H */
/*** end of file ***/

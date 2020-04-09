#ifndef __CBL_H
#define __CBL_H
#include <main.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"
#include "dma.h"

#define CBL_VERSION "v0.2"

#define pUARTCmd &huart2 /*!< UART used for shell communication */

#define CBL_ADDR_USERAPP 0x08008000UL /*!< Address of MSP of user application */
#define CBL_CMD_BUF_SZ 128 /*!< Size of a new command buffer */
#define CBL_MAX_ARGS 8 /*!< Maximum number of arguments in an input cmd */

#define ERR_CHECK(x) 	do							\
						{							\
							if((x) != CBL_ERR_OK) 	\
								return (x);			\
						}							\
						while(0)

#define CRLF "\r\n"

#define CBL_TXTCMD_VERSION "version"
#define CBL_TXTCMD_HELP "help"
#define CBL_TXTCMD_CID "cid"
#define CBL_TXTCMD_GET_RDP_LVL "get-rdp-level"
#define CBL_TXTCMD_JUMP_TO "jump-to"
#define CBL_TXTCMD_FLASH_ERASE "flash-erase"
#define CBL_TXTCMD_EN_RW_PR "en-rw-protect" // TODO
#define CBL_TXTCMD_DIS_RW_PR "dis-rw-protect" // TODO
#define CBL_TXTCMD_MEM_READ "mem-read" // TODO
#define CBL_TXTCMD_GET_SECT_STAT "get-sect-status" // TODO
#define CBL_TXTCMD_OTP_READ "otp-read" // TODO
#define CBL_TXTCMD_MEM_WRITE "mem-write" // TODO
#define CBL_TXTCMD_EXIT "exit"

#define CBL_TXTCMD_JUMP_TO_ADDR "addr"
#define CBL_TXTCMD_FLASH_ERASE_SECT "sector"
#define CBL_TXTCMD_FLASH_ERASE_COUNT "count"

/* Missing address locations in stm32f407xx.h */
#define SRAM1_END 0x2001BFFFUL
#define SRAM2_END 0x2001FFFFUL
#define BKPSRAM_END 0x40024FFFUL
#define SYSMEM_BASE 0x2001FFFFUL
#define SYSMEM_END  0x1FFF77FFUL

#define IS_CCMDATARAM_ADDRESS(x) (((x) >= CCMDATARAM_BASE) && ((x) <= CCMDATARAM_END))
#define IS_SRAM1_ADDRESS(x) (((x) >= SRAM1_BASE) && ((x) <= SRAM1_END))
#define IS_SRAM2_ADDRESS(x) (((x) >= SRAM2_BASE) && ((x) <= SRAM2_END))
#define IS_BKPSRAM_ADDRESS(x) (((x) >= BKPSRAM_BASE) && ((x) <= BKPSRAM_END))
#define IS_SYSMEM_ADDRESS(x) (((x) >= SYSMEM_BASE) && ((x) <= SYSMEM_END))

/* Colors: RED, ORANGE, GREEN and BLUE */
#define LED_ON(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port, LED_##COLOR##_Pin, GPIO_PIN_SET);
#define LED_OFF(COLOR) HAL_GPIO_WritePin(LED_##COLOR##_GPIO_Port, LED_##COLOR##_Pin, GPIO_PIN_RESET);

typedef enum CBL_ErrCode_e
{
	CBL_ERR_OK = 0, /*!< No error, we gucci */
	CBL_ERR_READ_OF, /*!< Buffer overflowed while reading */
	CBL_ERR_WRITE, /*!< Error while writing */
	CBL_ERR_STATE, /*!< Unexpected state requested */
	CBL_ERR_HAL_TX, /*!< Error happened in HAL library while transmiting */
	CBL_ERR_HAL_RX, /*!< Error happened in HAL library while receiving */
	CBL_ERR_RX_ABORT, /*!< Error happened while aborting receive */
	CBL_ERR_CMD_SHORT, /*!< Received command is of length 0 */
	CBL_ERR_CMD_UNDEF, /*!< Received command is invalid */
	CBL_ERR_CMDCD, /*!< Invalid command code enumerator */
	CBL_ERR_NEED_PARAM, /*!< Called command is missing a parameter */
	CBL_ERR_INV_ADDR, /*!< Given address is invalid */
	CBL_ERR_HAL_ERASE, /*!< HAL error happened while erasing */
	CBL_ERR_SECTOR, /*!< Error happened while erasing sector */
	CBL_ERR_INV_SECT, /*!< Wrong sector number given */
	CBL_ERR_INV_SECT_COUNT, /*!< Wrong sector count given */
} CBL_ErrCode_t;

typedef enum CBL_CmdArg_e
{
	CBL_ARG_NAME = 0, CBL_ARG_VAL = 1, CBL_ARG_MAX = 2
} CBL_CmdArg_t;

typedef struct CBL_Parser_s
{
	char *cmd; /*!< Command buffer */
	size_t len; /*!< length of the whole cmd string */
	char *args[CBL_MAX_ARGS][CBL_ARG_MAX]; /*!< Pointers to a buffers holding name and value of an argument */
	uint8_t numOfArgs;
} CBL_Parser_t;

typedef enum CBL_CMD_e
{
	CBL_CMD_UNDEF = 0,
	CBL_CMD_VERSION,
	CBL_CMD_HELP,
	CBL_CMD_CID,
	CBL_CMD_GET_RDP_LVL,
	CBL_CMD_JUMP_TO,
	CBL_CMD_FLASH_ERASE,
	CBL_CMD_EN_RW_PR,
	CBL_CMD_DIS_RW_PR,
	CBL_CMD_MEM_READ,
	CBL_CMD_GET_SECT_STAT,
	CBL_CMD_OTP_READ,
	CBL_CMD_MEM_WRITE,
	CBL_CMD_EXIT
} CBL_CMD_t;

typedef enum CBL_ShellStates_e
{
	CBL_STAT_OPER, /*!< Operational state */
	CBL_STAT_ERR, /*!< Error state */
	CBL_STAT_EXIT /*!< Deconstructor state */
} CBL_sysStates_t;

void CBL_Start(void);

#endif /* __CBL_H */

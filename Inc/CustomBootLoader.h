#ifndef __CBL_H
#define __CBL_H
#include <main.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "dma.h"

#define CBL_VERSION "v0.1"

#define pUARTCmd &huart2 /*!< UART used for shell communication */

#define CBL_ADDR_USERAPP 0x08008000U /*!< address of MSP of user application */
#define CBL_CMD_BUF_SZ 128 /*!< size of a new command buffer */
#define CBL_MAX_ARGS 8 /*!< maximum number of arguments in an input cmd */

#define ERR_CHECK(x) 	do						\
						{						\
							if(x != CBL_ERR_OK) \
								return x;		\
						}						\
						while(0)

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
	CBL_ERR_CMD_UNDEF /*!< Received command is invalid */
} CBL_ErrCode_t;

typedef struct CBL_Parser_s
{
	char *cmd; /*!< Command buffer */
	size_t len; /*!< length of the whole cmd string */
	char *args[CBL_MAX_ARGS][2]; /*!< Pointers to a buffers holding name and value of an argument */
	uint8_t numOfArgs;
} CBL_Parser_t;

typedef enum CBL_CmdArg_e
{
	CBL_ARG_NAME = 0,
	CBL_ARG_VAL = 1
} CBL_CmdArg_t;

#define CBL_TXTCMD_VERSION "version"
#define CBL_TXTCMD_HELP "help"
#define CBL_TXTCMD_CID "cid"
#define CBL_TXTCMD_RDP_STATUS "rdp-status"
#define CBL_TXTCMD_JUMP_TO "jump-to"
#define CBL_TXTCMD_FLASH_ERASE "flash-erase"
#define CBL_TXTCMD_EN_RW_PR "en-rw-protect"
#define CBL_TXTCMD_DIS_RW_PR "dis-rw-protect"
#define CBL_TXTCMD_MEM_READ "mem-read"
#define CBL_TXTCMD_GET_SECT_STAT "get-sect-status"
#define CBL_TXTCMD_OTP_READ "otp-read"
#define CBL_TXTCMD_MEM_WRITE "mem-write"
#define CBL_TXTCMD_EXIT "exit"

typedef enum CBL_CMD_e
{
	CBL_CMD_UNDEF = 0,
	CBL_CMD_VERSION,
	CBL_CMD_HELP,
	CBL_CMD_CID,
	CBL_CMD_RDP_STATUS,
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

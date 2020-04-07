#ifndef __CBL_H
#define __CBL_H
#include <main.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"

#define CBL_ADDR_USERAPP 0x08008000U /* address of MSP of user application */

typedef enum CBL_ErrCode_e
{
	CBL_ERR_NO = 0, /* No error, all gucci */
	CBL_ERR_READ, /* Error happened while reading */
	CBL_ERR_WRITE, /* Error while writing */
	CBL_ERR_STATE /* Unexpected state requested */
} CBL_Err_Code_t;

typedef struct CBL_Handle_s
{

} CBL_Handle_t;

typedef enum CBL_ShellStates_e
{
	CBL_STAT_OPER, CBL_STAT_ERR, CBL_STAT_EXIT
} CBL_sysStates_t;

void CBL_init(void);

#endif /* __CBL_H */

/** @file cbl_cmds_etc.h
 *
 * @brief Commands that don't fall in no other category, but don't deserve their
 *        own file
 */
#ifndef CBL_CMDS_ETC_H
#define CBL_CMDS_ETC_H
#include "etc/cbl_common.h"

#define TXT_CMD_CID "cid"
#define TXT_CMD_EXIT "exit"

cbl_err_code_t cmd_cid (parser_t * phPrsr);
cbl_err_code_t cmd_exit (parser_t * phPrsr);

#endif /* CBL_CMDS_ETC_H */
/*** end of file ***/

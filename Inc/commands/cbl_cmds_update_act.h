/** @file cbl_cmds_update_act.h
 *
 * @brief Adds command for updating bytes for active application, writes to boot
 *        record
 */
#ifndef CBL_CMDS_UPDATE_ACT_H
#define CBL_CMDS_UPDATE_ACT_H
#include "etc/cbl_common.h"

#define TXT_CMD_UPDATE_ACT "update-act"
#define TXT_PAR_UP_ACT_FORCE "force"
#define TXT_PAR_UP_ACT_TRUE "true"
#define TXT_PAR_UP_ACT_FALSE "false"

cbl_err_code_t cmd_update_act (parser_t * phPrsr);

#endif /* CBL_CMDS_UPDATE_ACT_H */
/*** end of file ***/

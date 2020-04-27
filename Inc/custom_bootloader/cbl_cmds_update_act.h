/** @file cbl_cmds_update_act.h
 *
 * @brief Adds command for updating bytes for active application, writes to boot
 *        record
 */
#ifndef CBL_CMDS_UPDATE_ACT_H
#define CBL_CMDS_UPDATE_ACT_H
#include "cbl_common.h"

#define TXT_CMD_UPDATE_ACT "update-act"

cbl_err_code_t cmd_update_act (parser_t * phPrsr);

#endif /* CBL_CMDS_UPDATE_ACT_H */
/*** end of file ***/

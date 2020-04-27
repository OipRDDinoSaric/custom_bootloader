/** @file cbl_cmds_update_act.h
 *
 * @brief Adds command for updating bytes for active application, writes to boot
 *        record
 */
#ifndef CBL_CMDS_UPDATE_ACT_H
#define CBL_CMDS_UPDATE_ACT_H
#include "cbl_common.h"

#define TXT_CMD_UPDATE_ACT "update-act"
/* Also takes count parameter from cbl_cmds_memory.h */
/* Also takes checksum parameter from cbl_checksum.h */
/* Also takes application type parameter from cbl_boot_record.h */

cbl_err_code_t cmd_update_act (parser_t * phPrsr);

#endif /* CBL_CMDS_UPDATE_ACT_H */
/*** end of file ***/

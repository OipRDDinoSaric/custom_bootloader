/** @file cbl_cmds_update_new.h
 *
 * @brief Adds command for updating bytes for new application, writes to boot
 *        record
 */
#ifndef CBL_CMDS_UPDATE_NEW_H
#define CBL_CMDS_UPDATE_NEW_H
#include "etc/cbl_common.h"
#include "etc/cbl_boot_record.h"

#define TXT_CMD_UPDATE_NEW "update-new"
#define TXT_PAR_UP_NEW_COUNT "count"
/* Also takes checksum parameter from cbl_checksum.h */
/* Also takes application type parameter from cbl_boot_record.h */

cbl_err_code_t cmd_update_new (parser_t * phPrsr);

#endif /* CBL_CMDS_UPDATE_NEW_H */
/*** end of file ***/

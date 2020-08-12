/** @file cbl_cmds_opt_bytes.h
 *
 * @brief Function handles for handling option bytes
 */
#ifndef CBL_CMDS_OPT_BYTES_H
#define CBL_CMDS_OPT_BYTES_H
#include "etc/cbl_common.h"

#define TXT_CMD_GET_RDP_LVL "get-rdp-level"
#define TXT_CMD_EN_WRITE_PROT "en-write-prot"
#define TXT_CMD_DIS_WRITE_PROT "dis-write-prot"
#define TXT_CMD_READ_SECT_PROT_STAT "get-write-prot"

#define TXT_PAR_EN_WRITE_PROT_MASK "mask"

cbl_err_code_t cmd_get_rdp_lvl (parser_t * phPrsr);
cbl_err_code_t cmd_change_write_prot (parser_t * phPrsr, bool EnDis);
cbl_err_code_t cmd_get_write_prot (parser_t * phPrsr);

#endif /* CBL_CMDS_OPT_BYTES_H */
/*** end of file ***/

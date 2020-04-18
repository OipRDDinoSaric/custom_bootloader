/** @file cbl_checksum.h
 *
 * @brief All checksum implementations available
 */
#ifndef CBL_CMDS_TEMPLATE_H
#define CBL_CMDS_TEMPLATE_H
#include "cbl_common.h"

typedef enum
{
    CKSUM_UNDEF = 0,
    CKSUM_SHA256,
    CKSUM_CRC
} cksum_t;

#define TXT_CKSUM_SHA256 "sha256"
#define TXT_CKSUM_CRC "crc"

cbl_err_code_t verify_checksum (uint8_t * buf, uint32_t len, cksum_t cksum);
cbl_err_code_t enum_checksum (char * checksum, uint32_t len, cksum_t * p_cksum);
cbl_err_code_t verify_crc (uint8_t * write_buf, uint32_t len);
cbl_err_code_t verify_sha256 (uint8_t * buf, uint32_t len);

#endif /* CBL_CHECKSUM_H */
/*** end of file ***/

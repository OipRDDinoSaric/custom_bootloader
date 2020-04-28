/** @file cbl_checksum.h
 *
 * @brief All checksum implementations available
 */
#ifndef CBL_CHECKSUM_H
#define CBL_CHECKSUM_H
#include "cbl_common.h"
#include "sha256.h"

typedef enum
{
    CKSUM_UNDEF = 0,
    CKSUM_SHA256,
    CKSUM_CRC32,
    CKSUM_NO
} cksum_t;

#define TXT_PAR_CKSUM "cksum"
#define TXT_CKSUM_SHA256 "sha256"
#define TXT_CKSUM_CRC "crc32"
#define TXT_CKSUM_NO "no"

cbl_err_code_t enum_checksum (char * checksum, uint32_t len, cksum_t * p_cksum);
void init_checksum (cksum_t cksum, SHA256_CTX * ph_sha256);
cbl_err_code_t accumulate_checksum (uint8_t * buf, uint32_t len, cksum_t cksum,
        SHA256_CTX * ph_sha256);
cbl_err_code_t accumulate_crc32 (uint8_t * buf, uint32_t len);
cbl_err_code_t accumulate_sha256 (uint8_t * buf, uint32_t len,
        SHA256_CTX * ph_sha256);
cbl_err_code_t verify_checksum (uint8_t * buf, uint32_t len, cksum_t cksum,
        SHA256_CTX * ph_sha256);
cbl_err_code_t verify_crc32 (uint8_t * p_recv_cksum, uint32_t cksum_len);
cbl_err_code_t verify_sha256 (uint8_t * p_recv_cksum, uint32_t cksum_len,
        SHA256_CTX * ph_sha256);
uint32_t checksum_get_length(cksum_t cksum);
#if 0
cbl_err_code_t verify_checksum_old (uint8_t * buf, uint32_t len, cksum_t cksum);
cbl_err_code_t verify_crc_old (uint8_t * write_buf, uint32_t len);
cbl_err_code_t verify_sha256_old (uint8_t * buf, uint32_t len);
#endif

#endif /* CBL_CHECKSUM_H */
/*** end of file ***/

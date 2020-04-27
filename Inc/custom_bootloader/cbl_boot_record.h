/** @file cbl_boot_record.h
 *
 * @brief Boot record hold useful data about current version of user application
 *        and of a new one, if it is available
 */
#ifndef CBL_BOOT_RECORD_H
#define CBL_BOOT_RECORD_H
#include <stdbool.h>
#include "cbl_common.h"
#include "cbl_checksum.h"

/* These values are from linker file ***.ld */
#define BOOT_RECORD_START 0x800C000UL
#define BOOT_RECORD_SECTOR 3
#define BOOT_RECORD_MAX_SECTORS 1

#define BOOT_ACT_APP_START 0x08010000UL
#define BOOT_ACT_APP_MAX_LEN (448 * 1024)
#define BOOT_ACT_APP_START_SECTOR 4
#define BOOT_ACT_APP_MAX_SECTORS 4

#define BOOT_NEW_APP_START 0x08080000UL
#define BOOT_NEW_APP_MAX_LEN (448 * 1024) /* If it is bigger it can't fit into
                                           active app */
#define BOOT_NEW_APP_START_SECTOR 8
#define BOOT_NEW_APP_MAX_SECTORS 4

#define TXT_PAR_APP_TYPE "type"
#define TXT_PAR_APP_TYPE_BIN "bin"
#define TXT_PAR_APP_TYPE_HEX "hex"
#define TXT_PAR_APP_TYPE_SREC "srec"

typedef enum
{
    TYPE_UNDEF = 0,
    TYPE_BIN,
    TYPE_HEX,
    TYPE_SREC
} app_type_t;

typedef struct
{
    cksum_t cksum_type; /* Size of 4 bytes assumed */
    app_type_t app_type; /* Size of 4 bytes assumed */
    char cksum_val[256]; /* WARNING: Char of size 1 byte assumed */
    uint32_t len;
} cbl_app_meta_t;

typedef struct
{
    bool new_app_ready; /* WARNING: Size of 1 byte assumed */
    cbl_app_meta_t act_app; /*!< Active application meta data */
    cbl_app_meta_t new_app; /*!< New application meta data */
    uint8_t reserved[255];
} cbl_record_t;

volatile cbl_record_t * cbl_boot_record_get (void);
cbl_err_code_t cbl_boot_record_set (cbl_record_t * new_bl_record);

#endif /* CBL_CMDS_TEMPLATE_H */
/*** end of file ***/

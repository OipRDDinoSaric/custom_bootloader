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
    /*WARNING: Size of 4 bytes assumed */
    cksum_t cksum_used; /*!< Checksum used for transmition */
    /*WARNING: Size of 4 bytes assumed */
    app_type_t app_type;
    uint32_t len;
} app_meta_t;

typedef struct
{
    bool is_new_app_ready; /* WARNING: Size of 1 byte assumed */
    app_meta_t act_app; /*!< Active application meta data */
    app_meta_t new_app; /*!< New application meta data */
    uint32_t key; /*!< Used to check if boot_record was initialized.
                       Boot record user shall ignore */
    uint8_t reserved[255];
} boot_record_t;

boot_record_t * boot_record_get (void);
cbl_err_code_t boot_record_set (boot_record_t * p_new_boot_record);
cbl_err_code_t enum_app_type (char *char_app_type, uint32_t len,
        app_type_t * p_app_type);

#endif /* CBL_CMDS_TEMPLATE_H */
/*** end of file ***/

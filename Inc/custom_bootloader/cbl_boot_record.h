/** @file cbl_boot_record.h
 *
 * @brief Boot record hold useful data about current version of user application
 *        and of a new one, if it is available
 */
#ifndef CBL_BOOT_RECORD_H
#define CBL_BOOT_RECORD_H
#include <stdbool.h>

/* These values are from linker file ***.ld */
#define BOOT_RECORD_START 0x800C000UL
#define BOOT_RECORD_SECTOR 3
#define BOOT_ACT_APP_START 0x08010000UL
#define BOOT_NEW_APP_START 0x08080000UL

typedef enum
{
    BOOT_CKSUM_UNDEF = 0,
    BOOT_CKSUM_SHA256,
    BOOT_CKSUM_CRC,
    BOOT_CKSUM_NO,
    BOOT_CKSUM_UNDEF2 = 0xFFFFFFFF
} boot_cksum_t;

typedef enum
{
    BOOT_OK = 0,
    BOOT_ERR
} boot_err_t;

typedef struct
{
    boot_cksum_t cksum_type; /* Size of 4 bytes assumed */
    char cksum_val[256]; /* Char of size 1 byte assumed */
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
boot_err_t cbl_boot_record_set (cbl_record_t * new_bl_record);
boot_err_t cbl_program_bytes (uint8_t * p_new_byte, uint32_t start,
        uint32_t len);

#endif /* CBL_CMDS_TEMPLATE_H */
/*** end of file ***/

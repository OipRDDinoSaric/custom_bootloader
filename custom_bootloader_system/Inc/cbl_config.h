/** @file cbl_config.h
 *
 * @brief Configuration for custom bootloader.
 */
#ifndef CBL_CONFIG_H
#define CBL_CONFIG_H

#if true
#define CBL_HAL_CHOSEN
#include "cbl_hal_stm32f407_disc1.h" /* Choose what HAL to use. */
#endif


#define USE_CMDS_MEMORY     1
#define USE_CMDS_OPT_BYTES  1
#define USE_CMDS_ETC        1
#define USE_CMDS_UPDATE_NEW 1
#define USE_CMDS_UPDATE_ACT 1
#define USE_CMDS_TEMPLATE   1

#ifndef CBL_HAL_CHOSEN
#   error "Set what HAL to use in cbl_config.h!"
#endif

#endif /* CBL_CONFIG_H */
/*** end of file ***/

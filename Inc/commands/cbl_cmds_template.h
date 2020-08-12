/** @file cbl_cmds_template.h
 *
 * @brief A description of the module’s purpose.
 *
 * @note  This file shall not be edited. It is for reference on how to add a
 *        new command category
 */
#ifndef CBL_CMDS_TEMPLATE_H
#define CBL_CMDS_TEMPLATE_H
#include "etc/cbl_common.h"

/* To add a new command:
 *  1. Create a command handler, from this template's .c file
 *  2. Add an enumerator in cmd_t (custom_bootloader.c)
 *  3. Add defines in header file for command name and parameters
 *     (as demonstrated below)
 *  4. Add, in enum_cmd (custom_bootloader.c), new 'if' to check if input
 *   command matches
 *  5. Add, in handle_cmd, new case with a call for command handler
 *  6. Add an enumerator in cbl_err_code_t (custom_bootloader.h) for errors
 *     from the function
 *  7. In sys_state_error (custom_bootloader.c) add handlers for new errors
 *  8. In cmd_help (custom_bootloader.c) add description of newly added command
 *
 *  All steps have been done for function below, just include this header in
 *  custom_bootloader.c for demonstration of cmd_template
 */

/* Add a cmd name */
#define TXT_CMD_TEMPLATE "template"
/* Add parameter, if needed */
#define TXT_PAR_TEMPLATE_PARAM1 "param1"
/* Add parameter value, if needed */
#define TXT_PAR_TEMPLATE_VAL1 "val1"

cbl_err_code_t cmd_template (parser_t * phPrsr);

#endif /* CBL_CMDS_TEMPLATE_H */
/*** end of file ***/

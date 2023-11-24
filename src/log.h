/*
 * MADAI
 *
 * Copyright (C) 2016 DesigNET, INC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LOG_H
#define _LOG_H

#include <errno.h>
#include <string.h>

#define SYSLOG_IDENT "madai"

#define F                      __FILE__
#define L                      __LINE__
#define FL                     __FILE__,__LINE__

/* Milter LOG */
#define ERR_MILTER_START         "Cannot start milter.: (%s)"
#define ERR_MILTER_SET_SOCKET    "Cannot set milter socket.: (%s)"
#define ERR_MILTER_SET_TIMEOUT   "Cannot set milter timeout.: (%s)"
#define ERR_MILTER_REGISTER      "Cannot set milter register.: (%s)"
#define ERR_MILTER_SET_TIMEOUT   "Cannot set milter timeout.: (%s)"
#define ERR_MILTER_START         "Cannot start milter.: (%s)"
#define ERR_CREATE_SOCKET        "Cannot create socket."
#define ERR_CONNECT_SOCKET       "Cannot connect socket."
#define ERR_CLOSE_SOCKET         "Cannot close socket."
#define ERR_WRITE                "%s(line:%d):Failed to send message from server.(ipaddr->%s, port->%d)"
#define ERR_READ                 "%s(line:%d):Failed to receive message from server.(ipaddr->%s, port->%d)"

#define STDERR_CONFIG_READ       "Cannot load madai config.:(%s)"
#define ERR_GET_PRIV             "Cannot get private data pointer.:(%s)"

/* reload LOG */
#define ERR_RLD_CONF_FAILURE     "Failed to load the configuration file."
#define ERR_USE_OLD_PARAM        "MADAI is running with old parameters."
#define SUCCESS_CONF_CHANGE      "Success reloading the configuration file."

/* load LOG */
#define ERR_SET_MEMBER_NULL      "%s(line:%d):Set all member to NULL Failed."
#define ERR_READ_CONFIG_FILE     "%s(line:%d):Read config file error."
#define ERR_GET_CONFIG_DATA      "%s(line:%d):Get config data Failed."
#define SUCCESS_READ_CONFIG_FILE "Success loading the configuration file."

#define ERR_FUNC_MAKE_C_SET      "%s(line:%d):function_failed(make_config_set)"
#define ERR_FUNC_MAKE_C_DATA     "%s(line:%d):function_failed(make_config_data)"
#define ERR_FUNC_INIT_C_SET      "%s(line:%d):function_failed(init_config_set)"
#define ERR_FUNC_INIT_C_DATA     "%s(line:%d):function_failed(init_config_data)"
#define ERR_SETUP_CONFIG_SET     "%s(line:%d):function_failed(setup_config_set)"
#define ERR_FUNC_GETHOSTNAME     "%s(line:%d):function_failed(gethostname)"

/* signal LOG */
#define ERR_BLOCK_SIGNAL         "Cannot cover signal with mask."
#define ERR_UNBLOCK_SIGNAL       "Cannot discover signal with mask."
#define ERR_CATCH_SIGNAL         "Cannot catch signal for Reloading." 
#define ERR_THREAD_CREATE        "Cannot create new thread for realoading."

/* memory error */
#define ERR_MEMORY_ALLOCATE      "%s(line:%d):Cannot allocate memory."

void (*logfunc)(const char *, ...);
void errorlog(char *, ...);
void switch_log(char *);
void systemlog(char *, ...);
void init_log();

extern int debugmode;

#define LOG (*logfunc)
#define DEBUG_LOG (!debugmode) ?: (*logfunc)
#endif /* _LOG_H */

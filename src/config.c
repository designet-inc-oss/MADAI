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

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <syslog.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <linux/limits.h>

#include "madai.h"
#include "thread_control.h"
#include "log.h"
#include "config.h"
#include "libdg/libdgconfig.h"

/* config file path save */
static char _config_filepath[PATH_MAX];
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static struct config_set *_cur_config = NULL;

/* Configuration terms for Encryption mode */
struct cfentry cfe[] = {
    {
        "SyslogFacility", CF_STRING, "local1",
        OFFSET(struct config_data, cd_syslogfacility), is_syslog_facility,
    },
    {
        "ListenIP", CF_STRING, "127.0.0.1",
        OFFSET(struct config_data, cd_listenip), is_ipaddr
    },
    {
        "ListenPort", CF_INT_PLUS, "20026",
        OFFSET(struct config_data, cd_listenport), is_port
    },
    {
        "MilterTimeout", CF_INT_PLUS, "120",
        OFFSET(struct config_data, cd_miltertimeout), is_plus
    },
    {
        "CopyServerIP", CF_STRING, NULL,
        OFFSET(struct config_data, cd_copyserverip), is_ipaddr
    },
    {
        "CopyPort", CF_INT_PLUS, NULL,
        OFFSET(struct config_data, cd_copyport), is_port
    },
    {
        "CopyMailTimeout", CF_INT_PLUS, "30",
        OFFSET(struct config_data, cd_copymailtimeout), is_plus
    },
    {
        "HeloDomain", CF_STRING, "",
        OFFSET(struct config_data, cd_helodomain), NULL
    },
};

#define NCONFIG (sizeof(cfe) / sizeof(struct cfentry))

/***************************************
 * _free_config_data_data()
 * 
 * First read configuration file
 *
 * Args:
 *  struct config_data *cfg     configure file data
 *
 * Return value:    none
 ***************************************/
static void
_free_config_data(struct config_data *cfg_data)
{
    int i;

    for (i = 0; i < NCONFIG; i++) {
        int len = 0;
        len = (cfe + i)->cf_dataoffset ;
        if ((cfe + i)->cf_type == CF_STRING) {
            free(*(char **)((void *)cfg_data + len));
        }
    }

    free(cfg_data);
}

/***************************************
 * free_config_set()
 * 
 * First read configuration file
 *
 * Args:
 *  struct config_data *cfg     configure file data
 *
 * Return value:    none
 ***************************************/
void free_config_set(struct config_set *cfg_set)
{
    _free_config_data(cfg_set->cs_config);
    free(cfg_set);
}

/***************************************
 * print_config_data()
 * 
 * First read configuration file
 *
 * Args:
 *  struct config_data *cfg_data
 *
 * Return value:    none
 ***************************************/
void print_config_data()
{
    printf("-----------configure file data-----------\n");
    int i;
    for (i = 0; i < NCONFIG; i++) {
        int len = 0;
        len = (cfe + i)->cf_dataoffset ;
        if ((cfe + i)->cf_type == CF_INT_PLUS) {
            printf("%s=%d\n", (cfe + i)->cf_name, *(int *)((void *)_cur_config->cs_config + len));
        } else if ((cfe + i)->cf_type == CF_STRING) {
            printf("%s=%s\n", (cfe + i)->cf_name, *(char **)((void *)_cur_config->cs_config + len));
        }
    }
    printf("-----------------------------------------\n");
}

/**************************************
 * setup_config_set()
 *
 * [Arg]
 * none
 *
 * [Ret]
 * struct config_set    *_cur_config
 * NULL                            : ERROR
 **************************************/
struct config_set *
setup_config_set()
{
    struct config_data *cfg_data;
    struct config_set *cfg_set;

    /* init struct config_data */
    cfg_data = init_config_data();
    if (cfg_data == NULL) {
        DEBUG_LOG(ERR_FUNC_INIT_C_DATA, FL);
        return NULL;
    }
 
    /* set struct config_data */
    cfg_data = make_config_data(cfg_data);
    if (cfg_data == NULL) {
        DEBUG_LOG(ERR_FUNC_MAKE_C_DATA, FL);
        return NULL;
    }

    /* init struct config_set */
    cfg_set = init_config_set();
    if (cfg_set == NULL) {
        DEBUG_LOG(ERR_FUNC_INIT_C_SET, FL);
        return NULL;
    }
    
    /* set struct config_set */
    make_config_set(cfg_set, cfg_data);
    if (cfg_set == NULL) {
        DEBUG_LOG(ERR_FUNC_MAKE_C_SET, FL);
        return NULL;
    }

    return cfg_set;
}

/**************************************
 * startup_config()
 *
 * [Arg]
 * char *filepath   configure file path
 *
 * [Ret]
 * struct config_set    *_cur_config
 * NULL                            : ERROR
 **************************************/
struct config_set *
startup_config(char *filepath)
{
    struct config_set *cfg_set;

    /* save filepath */
    strcpy(_config_filepath, filepath);

    /* dglib/dglibconfig dgloginit */
    dgloginit();

    /* setup struct config_set */
    cfg_set = setup_config_set();
    if (cfg_set == NULL) {
        DEBUG_LOG(ERR_SETUP_CONFIG_SET, FL);
        return NULL;
    }

    /* switch log */
    dglogchange(SYSLOG_IDENT, cfg_set->cs_config->cd_syslogfacility);

    _cur_config = cfg_set;

    return _cur_config;
}

/**************************************
 * init_config_set()
 *
 * [Arg]
 * none.
 *
 * [Ret]
 * struct config_set    *_cur_config
 * NULL                            : ERROR
 **************************************/
struct config_set *
init_config_set()
{
    struct config_set *cfg_set;

    /* init structure of configuration */
    cfg_set = (struct config_set *)malloc(sizeof(struct config_set));
    if (cfg_set == NULL) {
        LOG(ERR_MEMORY_ALLOCATE, FL);
        return NULL;
    }

    cfg_set->cs_config = NULL;
    cfg_set->cs_refcount = 0;
    cfg_set->cs_delflg = 0;

    return cfg_set;
}

/**************************************
 * make_config_set()
 *
 * [Arg]
 * none.
 *
 * [Ret]
 * struct config_set    *_cur_config
 * NULL                            : ERROR
 **************************************/
void
make_config_set(struct config_set *cfg_set, struct config_data *cfg_data)
{
    /* read configuration file */
    cfg_set->cs_config = cfg_data;
    cfg_set->cs_refcount = 0;
    cfg_set->cs_delflg = FALSE;
}

/***************************************
 * init_config_data()
 * 
 * init struct config_data memory
 *
 * Args:
 *  char *file      Configuration file name
 *
 * Return value:
 *  0           Success
 *  1           Error
 ***************************************/
struct config_data *
init_config_data()
{
    struct config_data *cfg_data = NULL;

    // allocate memory
    cfg_data = (struct config_data *)malloc(sizeof(struct config_data));
    if (cfg_data == NULL) {
        LOG(ERR_MEMORY_ALLOCATE, FL);
        return NULL;
    }

    // set all members to NULL
    memset(cfg_data, 0, sizeof(struct config_data));

    return cfg_data;

}

/***************************************
 * make_config_data()
 * 
 * init struct config_data memory
 *
 * Args:
 *  char *file      Configuration file name
 *
 * Return value:
 *  0           Success
 *  1           Error
 ***************************************/
struct config_data *
make_config_data(struct config_data *cfg_data)
{
    int ret = 0;

    // read config file
    ret = read_config(_config_filepath, cfe, NCONFIG, cfg_data);

    if (ret > 0) {
        // case config file error
        DEBUG_LOG(ERR_READ_CONFIG_FILE, FL);
        _free_config_data(cfg_data);
        return NULL;
    }

    if (ret < 0) {
        // case system error
        _free_config_data(cfg_data);
        return NULL;
    }

    return cfg_data;
}

/**************************************
 * reload_config()
 *
 * [Arg]
 * none.
 *
 * [Ret]
 * struct config_set    *_cur_config
 * NULL                            : ERROR
 **************************************/
int
reload_config()
{
    struct config_set *old_config;
    struct config_set *new_config;
    int    conf_delflg = FALSE;

    new_config = setup_config_set();
    /* read configuration file */
    if (new_config == NULL) {
        return 1;
    }

    /* point change */
    pthread_mutex_lock(&config_lock);
    old_config = _cur_config;
    old_config->cs_delflg = TRUE;
    _cur_config = new_config;
    if (old_config->cs_refcount == 0) {
        conf_delflg = TRUE;
    }

    /* change current config to new config */
    pthread_mutex_unlock(&config_lock);

    /* switch log */
    switch_log(_cur_config->cs_config->cd_syslogfacility);

    /* switch log */
    dglogchange(SYSLOG_IDENT, _cur_config->cs_config->cd_syslogfacility);

    /* If no one use old configuration, */
    if (conf_delflg == TRUE) {
        /* Free old configuraton */
        free_config_set(old_config);
    }

    return 0;
}

/**************************************
 * get_config()
 * [Description]
 * count up refcount of config.
 *
 * [Arg]
 * none.
 * [Ret]
 * struct conifig_set   *_cur_config: config that refcount is counted up.
 *
 * [Note]
 **************************************/
struct config_set *
get_config()
{
    struct config_set *cfg_set;

    /* count up reference counter */
    pthread_mutex_lock(&config_lock);
    cfg_set = _cur_config;
    cfg_set->cs_refcount++;
    pthread_mutex_unlock(&config_lock);

    return cfg_set;
}

/**************************************
 * release_config()
 * [Description]
 *
 * [Arg]
 * struct config_set *confset: read now config data.
 *
 * [Ret]
 * none.
 **************************************/
void
release_config(struct config_set *confset)
{
    int cfg_delflg = FALSE;

    /* count down reference counter */
    pthread_mutex_lock(&config_lock);
    confset->cs_refcount--;
    if (confset->cs_refcount == 0 && confset->cs_delflg == TRUE) {
        cfg_delflg = TRUE;
    }
    pthread_mutex_unlock(&config_lock);

    /* No one use the configuration, */
    if (cfg_delflg == TRUE) {
        /* It's old configuration, Then free all. */
        free_config_set(confset);
    }

    return;
}

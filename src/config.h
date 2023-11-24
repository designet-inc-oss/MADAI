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

#ifndef _CONFIG_H
#define _CONFIG_H

#define CF_INTEGER      1
#define CF_INT_PLUS     2
#define CF_STRING       3
#define CF_FUNCTION     4
#define CFG_NG -1

/* config file data struct */
struct config_data {
    char *cd_syslogfacility;
    char *cd_listenip;
    int   cd_listenport;
    int   cd_miltertimeout;
    char *cd_copyserverip;
    int   cd_copyport;
    int   cd_copymailtimeout;
    char *cd_helodomain;
};

/* config data struct */
struct config_set
{
    struct config_data *cs_config;    /* setting contents of /etc/scanmail.cfg */
    int                 cs_refcount;  /* reference counter for reload method */
    int                 cs_delflg;    /* delete frag of config_set */
};

/* config functions */
void release_config(struct config_set *confset);
int reload_config();
struct config_data * init_config_data();
struct config_data * make_config_data(struct config_data *cfg_data);
struct config_set * init_config_set();
void make_config_set(struct config_set *cfg_set, struct config_data *cfg_data);
void print_config_data();
struct config_set * startup_config(char *filepath);
struct config_set * setup_config_set();
struct config_set * get_config();

#endif /* config.h */

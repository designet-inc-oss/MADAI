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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <linux/limits.h>

#include "madai.h"
#include "thread_control.h"
#include "config.h"
#include "milter.h"
#include "log.h"

/*
 * usage
 * 
 */
void
usage(char *arg)
{
    fprintf(stderr, "Usage: %s [-d] [-f configure_file]\n", arg);
}

int main(int argc, char *argv[])
{
    char  *args = "df:";
    char  c;
    char oconn[OCONN_LENGTH];
    char filepath[PATH_MAX] = DEFAULT_CONFFILE;
    int pthread_ret;
    struct config_set *config_set;
    struct config_data *config_data;
    opterr = 0;
    sigset_t sig_set;
    pthread_t hup_thread;

    while ((c = getopt(argc, argv, args)) != -1) {
        switch(c) {
            case 'd':
                debugmode = 1;
                break;
            case 'f':
                if((optarg == NULL) || (optarg[0] == '\0')) {
                    usage(argv[0]);
                    exit(1);
                }
                strcpy(filepath, optarg);
                break;
            default:
                usage(argv[0]);
                exit(1);
        }
    }

    /* check arg */
    if (optind < argc) {
        usage(argv[0]);
        exit(1);
    }

    /* set error output to stderr */
    init_log();

    /* set configure file */
    config_set = startup_config(filepath);
    if (config_set == NULL) {
        LOG(STDERR_CONFIG_READ, filepath);
        exit(1);
    }

    config_data = config_set->cs_config;

    /* switch log */
    switch_log(config_data->cd_syslogfacility);

    LOG(SUCCESS_READ_CONFIG_FILE);

    if (debugmode) {
        print_config_data();
    }

    /* create a thread waiting signal for reloading */
    pthread_ret = pthread_create(&hup_thread, NULL, wait_hup, NULL);
    if (pthread_ret != 0) {
        LOG(ERR_THREAD_CREATE);
        exit(1);
    }

    /* set blocked signal type */
    sigemptyset(&sig_set);
    sigaddset(&sig_set, RELOAD_SIG);

    /* block HUNG UP signal.                  *
     * all threads are covered with a mask.   *
     * if threads catch signal for reloading, it's ignored. */
    pthread_ret = pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
    if (pthread_ret != 0) {
        LOG(ERR_BLOCK_SIGNAL);
        exit(1);
    }

    /* prepare socket string */
    if (sprintf(oconn, OCONN, config_data->cd_listenport, 
                            config_data->cd_listenip) < 0) {
        exit(1);
    }

    /* milter_connection_start */
    if(milter_start(oconn, config_data->cd_miltertimeout)) {
        exit(1);
    }

    return 0;
}


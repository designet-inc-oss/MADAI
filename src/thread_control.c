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

#include <syslog.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "madai.h"
#include "config.h"
#include "thread_control.h"
#include "log.h"

/**************************************
 * do_hup()
 * [Description]
 * if a thread catches signal for reloading, reload configuration.
 *
 * [Arg]
 * none.
 * [Ret]
 * none.
 **************************************/
void
do_hup()
{
    /* definition for working */
    int reloaded_cfg = 0;
    int pthread_ret;
    sigset_t sig_set;

    /* block signal while reload */
    sigemptyset(&sig_set);
    sigaddset(&sig_set, RELOAD_SIG);
    pthread_ret = pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
    if (pthread_ret == -1) {
        LOG(ERR_UNBLOCK_SIGNAL);
        return;
    }

    /* read new configuration files */
    reloaded_cfg = reload_config();
    if (reloaded_cfg != 0) {
        LOG(ERR_USE_OLD_PARAM);
    } else {
        LOG(SUCCESS_CONF_CHANGE);
    }

    /* unblock signal */
    sigaddset(&sig_set, RELOAD_SIG);
    pthread_ret = pthread_sigmask(SIG_UNBLOCK, &sig_set, NULL);
    if (pthread_ret == -1) {
        LOG(ERR_UNBLOCK_SIGNAL);
    }

    if (debugmode) {
        print_config_data();
    }

    return;
}

/**************************************
 * wait_hup()
 * [Description]
 * wait until a thread for catching signal
 * for reloading catches signal.
 * if the thread catches signal, this func runs do_hup().
 *
 * [Arg]
 * void    *arg: configuration file path.
 *
 * [Ret]
 * NULL        : ERROR
 **************************************/
void *
wait_hup(void *arg)
{
    /* definition for working */
    int sig_ret;
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = do_hup;
    act.sa_flags = SA_RESTART;

    /* If this thread catch signal for reloading... */
    sig_ret = sigaction(RELOAD_SIG, &act, NULL);
    if (sig_ret == -1) {
        LOG(ERR_CATCH_SIGNAL);
        return NULL;
    }
    
    /* sleep until next signal */
    while (1) {
        sleep(1000);
    }

    return NULL;
}

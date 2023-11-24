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
#include <stdarg.h>
#include <strings.h>
#include <syslog.h>
#define SYSLOG_NAMES

#include "log.h"
#include "libdg/libdgconfig.h"

int debugmode = 0;

/*
 * init_log()
 *
 * switch log output to stderr
 *
 * args:
 * none
 * return value:
 * none
 * 
 */
void
init_log()
{
    logfunc = (void *) errorlog;
}

/*
 * errorlog()
 *
 * output error log to stderr
 *
 * args:
 *  *fmt, ...
 *
 * return value:
 *  none
 */
void
errorlog(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

/*
 * systemlog()
 *
 * output error log to syslog
 *
 * args:
 *  *fmt, ...
 *
 * return value:
 *  none
 */
void
systemlog(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
}

/*
 * switch_log()
 *
 * switch log output
 *
 * args:
 *  *newfacility    new facility name or NULL
 *          - facility name -> calls openlog() and switch to syslog
 *          - NULL -> switch to stderr
 * return value:
 *  none
 *
 */
void
switch_log(char *newfacility)
{
    int facility;

    closelog();

    if (newfacility != NULL &&
        (facility = syslog_facility(newfacility)) >= 0) {
        openlog(SYSLOG_IDENT, LOG_PID, facility);
        logfunc = (void *) systemlog;
    } else {
        logfunc = (void *) errorlog;
    }
}

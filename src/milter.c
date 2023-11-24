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
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libmilter/mfapi.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "log.h"
#include "madai.h"
#include "milter.h"
#include "config.h"
#include "smtp.h"

#define IDENT           "madai"
#define MLFIPRIV(ctx)        ((struct session_data *) smfi_getpriv(ctx))

/* init milter struct */
struct session_data *session_data_init();
void free_session_data(struct session_data * session_list);

/* milter */
sfsistat mdfi_connect(SMFICTX *, char *, _SOCK_ADDR *);
sfsistat mdfi_envfrom(SMFICTX *, char **);
sfsistat mdfi_envrcpt(SMFICTX *, char **);
sfsistat mdfi_header(SMFICTX *, char *, char *);
sfsistat mdfi_eoh(SMFICTX *);
sfsistat mdfi_body(SMFICTX *, u_char *, size_t);
sfsistat mdfi_eom(SMFICTX *);
sfsistat mdfi_abort(SMFICTX *);
sfsistat mdfi_close(SMFICTX *);
sfsistat mdfi_data(SMFICTX *);

/* call back struct */
struct smfiDesc mdfilter =
{
    IDENT,
    SMFI_VERSION,   /* version code -- do not change */
    0,              /* flags */
    mdfi_connect,   /* connection info filter */
    NULL,           /* SMTP HELO command filter */
    mdfi_envfrom,   /* envelope sender filter */
    mdfi_envrcpt,   /* envelope recipient filter */
    mdfi_header,    /* header filter */
    mdfi_eoh,           /* end of header */
    mdfi_body,      /* body block filter */
    mdfi_eom,       /* end of message */
    mdfi_abort,     /* message aborted */
    mdfi_close,      /* connection cleanup */
    NULL,
    mdfi_data,      /* DATA command */
    NULL
};


/*
 * void start_session
 * session 
 */
int
milter_start(char *oconn, int timeout)
{
    /* set socket */
    if(smfi_setconn(oconn) == MI_FAILURE) {
        fprintf(stderr, ERR_MILTER_SET_SOCKET, strerror(errno));
        fprintf(stderr, "\n");
        return 1;
    }

    /* set timeout */
    if(smfi_settimeout(timeout) == MI_FAILURE) {
        fprintf(stderr, ERR_MILTER_SET_TIMEOUT, strerror(errno));
    	fprintf(stderr, "\n");
        return 1;
    }

    /* set register */
    if(smfi_register(mdfilter) == MI_FAILURE) {
        fprintf(stderr, ERR_MILTER_REGISTER, strerror(errno));
        fprintf(stderr, "\n");
        return 1;
    }

    /* run main */
    if(smfi_main() == MI_FAILURE) {
        fprintf(stderr, ERR_MILTER_START, strerror(errno));
        fprintf(stderr, "\n");
        return 1;
    }

    return 0;
}

/*
 * mdfi_connect
 * connect
 */
sfsistat
mdfi_connect(SMFICTX *ctx, char *hostname, _SOCK_ADDR *addr)
{
    struct session_data *session_list = NULL;

    /* prepare milter struct */
    session_list = session_data_init();
    if (session_list == NULL) {
        return SMFIS_ACCEPT;
    }

    /* set config data to "session_list" */
    session_list->sd_config = get_config();

    /* set private data to ctx */
    if (smfi_setpriv(ctx, session_list) != MI_SUCCESS) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}

/* 
 *  sfsistat mdfi_envfrom
 * 
 *  Envfrom will connect socket and send HELO and FROM.
 *  HELO may not be called.
 *  Please describe the operation you want to do when 
 *  connecting on envfrom.
 */
sfsistat
mdfi_envfrom(SMFICTX *ctx, char **mailfrom)
{
    struct session_data *session_list;
    int ret = 0;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_envfrom");
        return SMFIS_ACCEPT;
    }

    session_list->sd_socket = start_smtp(session_list);
    if (session_list->sd_socket == -1) {
        return SMFIS_ACCEPT;
    }

    ret = send_from(session_list);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    /* set private data to ctx */
    if (smfi_setpriv(ctx, session_list) != MI_SUCCESS) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}


/*
 * sfsistat mdfi_envrcpt
 * envrcpt
 */
sfsistat
mdfi_envrcpt(SMFICTX *ctx, char **rcptto)
{
    struct session_data *session_list;
    int ret = 0;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_envrcpt");
        return SMFIS_ACCEPT;
    }

    ret = send_rcpt(session_list, *rcptto);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}

/*
 * sfsistat mdfi_header
 * header
 */
sfsistat
mdfi_header(SMFICTX *ctx, char *headerf, char *headerv)
{
    int ret;
    struct session_data *session_list;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_header");
        return SMFIS_ACCEPT;
    }

    ret = send_header(session_list, headerf, headerv);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    /* continue processing */
    return SMFIS_CONTINUE;
}

/*
 * mdfi_eoh
 * end
 */
sfsistat
mdfi_eoh(SMFICTX *ctx)
{
    struct session_data *session_list;
    int ret;

    session_list = smfi_getpriv(ctx);
    if (session_list == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_eoh");
        return SMFIS_ACCEPT;
    }

    ret = send_eoh(session_list);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}


/*
 * mdfi_body
 * write body
 */
sfsistat
mdfi_body(SMFICTX *ctx, u_char *bodyp, size_t bodylen)
{
    struct session_data *session_list;
    char body[bodylen + 1];
    int ret;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_body");
        return SMFIS_ACCEPT;
    }

    memcpy(body, bodyp, bodylen);
    body[bodylen + 1] = '\0';

    ret = send_body(session_list, (char *)body, bodylen);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }
    
    return SMFIS_CONTINUE;
}

/*
 * mdfi_eom
 * end
 */
sfsistat
mdfi_eom(SMFICTX *ctx)
{
    struct session_data *session_list;
    int ret;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_eom");
        return SMFIS_ACCEPT;
    }

    ret = send_eom(session_list);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}

/*
 * mdfi_abort
 * abort
 */
sfsistat
mdfi_abort(SMFICTX *ctx)
{
    struct session_data *session_list;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_close");
        return SMFIS_ACCEPT;
    }

    close(session_list->sd_socket); 
    session_list->sd_socket = -1;
    smfi_setpriv(ctx, session_list);

    return SMFIS_CONTINUE;
}

/*
 * mdfi_close
 * close
 */
sfsistat
mdfi_close(SMFICTX *ctx)
{
    struct session_data *session_list;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_close");
        return SMFIS_ACCEPT;
    }

    if (session_list->sd_socket == -1) {
        close(session_list->sd_socket); 
    }

    free_session_data(session_list);

    smfi_setpriv(ctx, NULL);

    return SMFIS_CONTINUE;
}

/*
 * mdfi_data
 * data_header
 */
sfsistat
mdfi_data(SMFICTX *ctx)
{
    struct session_data *session_list;
    int ret;

    if ((session_list = MLFIPRIV(ctx)) == NULL) {
        LOG(ERR_GET_PRIV, "mdfi_close");
        return SMFIS_ACCEPT;
    }

    ret = send_data(session_list);
    if (ret == -1) {
        return SMFIS_ACCEPT;
    }

    return SMFIS_CONTINUE;
}

/*
 * free_session_data
 * memory free
 */
void
free_session_data(struct session_data *session_list)
{
    release_config(session_list->sd_config);
    free(session_list);
}

/*
 * session_data_init
 * struct session_data get memory
 */
struct session_data *
session_data_init()
{
    struct session_data *new_milter;

    /* allocate for session_list */
    new_milter = (struct session_data *)malloc(sizeof (struct session_data));
    if (new_milter == NULL) {
        LOG(ERR_MEMORY_ALLOCATE, FL); 
        return NULL;
    }

    /* clean up memory */
    memset(new_milter, 0, sizeof (struct session_data));

    /* set default values */
    new_milter->sd_config = NULL;
    new_milter->sd_socket = -1;

    return new_milter;
}

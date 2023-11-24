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

int
start_smtp(struct session_data *session_list) {
    int new_sockfd = 0;
    struct sockaddr_in client_addr;
    struct timeval tv;
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;

    /* socket already open */
    if (session_list->sd_socket != -1) {
        return session_list->sd_socket;
    }

    /* make socket */
    if ((new_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOG(ERR_CREATE_SOCKET);
        return -1;
    }

   /*  set connection IPaddr Port to client struct */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(cfg_data->cd_copyport);
    client_addr.sin_addr.s_addr = inet_addr(cfg_data->cd_copyserverip);

    /* set timeout */
    tv.tv_sec = session_list->sd_config->cs_config->cd_copymailtimeout;
    tv.tv_usec = 0;
    setsockopt(new_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(new_sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    /* connection socket */
    if (connect(new_sockfd, (struct sockaddr *)&client_addr, 
                                          sizeof(client_addr)) != 0) {
        LOG(ERR_CONNECT_SOCKET);
        close(new_sockfd);
        return -1;
    }

    if (read_msg(new_sockfd) < 0){
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        close(new_sockfd);
        return -1;
    }

    return new_sockfd;
}

int
send_from(struct session_data *session_list)
{
    int ret;
    char hostname[HOST_LEN];
    char buf[1024];
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;
    
    if (cfg_data->cd_helodomain[0] == '\0') {
        ret = gethostname(hostname, HOST_LEN);
        if (ret != 0) {
            LOG(ERR_FUNC_GETHOSTNAME, FL);
            return -1;
        }
    } else { 
        strcpy(hostname, cfg_data->cd_helodomain);
    }

    sprintf(buf, SEND_HELO, hostname);
    if (write_msg(session_list->sd_socket, buf, strlen(buf)) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (read_msg(session_list->sd_socket) < 0){
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (write_msg(session_list->sd_socket, "MAIL FROM: <>\r\n", 15) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (read_msg(session_list->sd_socket) < 0) {
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

int
send_rcpt(struct session_data *session_list, char *rcptto)
{
    char buf[1024];
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;

    sprintf(buf, SEND_RCPT, rcptto);
    if (write_msg(session_list->sd_socket, buf, strlen(buf)) < 0){
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (read_msg(session_list->sd_socket) < 0) {
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

/* 
 *  int send_header
 * 
 *  Milter replaces '\r\n' in Header with '\n'.
 *  This function need to replace '\n' with '\r\n'.
 */
    
int
send_header(struct session_data *session_list, char *headerf, char *headerv)
{
    char *l_start;
    char *l_end;
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;

    if (write_msg(session_list->sd_socket, headerf, strlen(headerf)) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (write_msg(session_list->sd_socket, ":", 1) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }
    
    l_start = headerv;
    l_end = strchr(headerv, '\n');  /* search CRLF */

    /* found CRLF */
    while (l_end != NULL) {
        if (write_msg(session_list->sd_socket, l_start, l_end - l_start) < 0) {
            LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
            return -1;
        }

        if (write_msg(session_list->sd_socket, "\r\n", 2) < 0) { 
            LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
            return -1;
        }

        l_start = l_end + 1;
        l_end = strchr(l_start, '\n');  /* search CRLF */
    }

    /* than headerv */
    if (write_msg(session_list->sd_socket, l_start, strlen(l_start)) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    /* write end CRLF */
    if (write_msg(session_list->sd_socket, "\r\n", 2) < 0) { 
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }
    
    return 0;
}

int
send_eoh(struct session_data *session_list)
{
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;
    if (write_msg(session_list->sd_socket, "\r\n", 2) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

int
send_body(struct session_data *session_list, char *bodyp, size_t bodylen)
{
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;
    if (write_msg(session_list->sd_socket, bodyp, bodylen) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

int
send_eom(struct session_data *session_list)
{
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;
    if (write_msg(session_list->sd_socket, ".\r\n", 3) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (read_msg(session_list->sd_socket) < 0) {
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

int
send_data(struct session_data *session_list)
{
    struct config_data *cfg_data;

    cfg_data = session_list->sd_config->cs_config;
    if (write_msg(session_list->sd_socket, "DATA\r\n", 6) < 0) {
        LOG(ERR_WRITE, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    if (read_msg(session_list->sd_socket) < 0) {
        LOG(ERR_READ, FL, cfg_data->cd_copyserverip, cfg_data->cd_copyport);
        return -1;
    }

    return 0;
}

int
write_msg(int sockfd, char *buf, int len)
{
    int  left_len = len;
    int  size = 0;

    while (left_len > 0) {
        size = write(sockfd, buf, left_len);
        if (size < 0) {
            return -1;
        }
        buf += size;
        left_len -= size;
    }

    return 0;
}

int
read_msg(int sockfd)
{
    int n;
    char buf[1024];
    
    n = read(sockfd, buf, 1024);
    if (n == -1) {
        return -1;
    }

    return 0;
}

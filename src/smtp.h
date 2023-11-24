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

#ifndef _SMTP_H
#define _SMTP_H

#define HOST_LEN   256

/* send mail macro */
#define SEND_HELO   "HELO %s\r\n"
#define SEND_FROM   "MAIL FROM:%s\r\n"
#define SEND_RCPT   "RCPT TO:%s\r\n"
#define SEND_DATA   "DATA\r\n"
#define SEND_HEADER "%s:%s\r\n"
#define SEND_BODY   "%s"
#define SEND_EOH    "\r\n"
#define SEND_EOM    ".\r\n"

int start_smtp(struct session_data *ml_config);
int send_from(struct session_data *ml_config);
int send_rcpt(struct session_data *ml_config, char *);
int send_header(struct session_data *ml_config, char *, char *);
int send_eoh(struct session_data *ml_config);
int send_body(struct session_data *ml_config, char *, size_t);
int send_eom(struct session_data *ml_config);
int send_data(struct session_data *ml_config);
int write_msg(int, char *, int);
int read_msg(int);

#endif /* smtp.h */

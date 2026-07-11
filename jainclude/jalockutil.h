/*
** Job Arranger for ZABBIX
** Copyright (C) 2012 FitechForce, Inc. All Rights Reserved.
** Copyright (C) 2013 Daiwa Institute of Research Business Innovation Ltd. All Rights Reserved.
** Copyright (C) 2021 Daiwa Institute of Research Ltd. All Rights Reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/
#ifndef JOBARG_JALOCKUTIL_H
#define JOBARG_JALOCKUTIL_H

extern int CONFIG_DB_CON_COUNT;
extern char	*CONFIG_LOG_FILE;

int init_session_dbc_locks();
void ja_session_dbc_lock(const char *session_id, int process_id);
void ja_session_dbc_unlock();
void get_jaz_folder_path(char *folder_path);
int ja_is_session_dbc_avaliable();

#endif

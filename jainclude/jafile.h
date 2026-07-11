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

#include <json-c/json.h>

#ifndef JOBARG_JAFILE_H
#define JOBARG_JAFILE_H

int ja_file_create(const char *filename, const int size);
int ja_file_remove(const char *filename);
long ja_file_getsize(const char *filename);
int ja_file_load(const char *filename, const int size, void *data);
int ja_file_is_regular(const char *path);
int ja_folders_prepare(char* folders[], const char *parent_folderpath);
int ja_folder_clean(const char *folderpath);
int ja_file_create_a(const char *filename);
#ifndef _WINDOWS
int ja_lock_file(int file_descriptor);
int ja_unlock_file(int file_descriptor);
#endif
int ja_write_jsonObj_to_file(json_object *json_data, char *data_file);
int ja_file_clean(const char *filename);
int ja_check_file_folder_exist(const char *entry);
int ja_lock_resource(const char *lock_file_path, int lock_mode, char * caller_function); 
void ja_unlock_resource(int file_descriptor);
#endif

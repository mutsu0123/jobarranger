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

#include "common.h"
#include "comms.h"
#include "log.h"
#include "jacommon.h"
#include "jafile.h"
#include "jalockutil.h"
#include <json-c/json.h>

static char locked_file[JA_FILE_PATH_LEN];
char locked_folder[JA_FILE_PATH_LEN];
json_object* json_data = NULL;
int file_descriptor = -1;

void get_jaz_folder_path(char *folder_path) {
    const char *lastSlash = strrchr(CONFIG_LOG_FILE, '/');

    if (lastSlash != NULL)
	{
		size_t directoryLength = lastSlash - CONFIG_LOG_FILE;
		zbx_strlcpy(folder_path, CONFIG_LOG_FILE, directoryLength + 1);
		folder_path[directoryLength] = '\0';
	}
}

int init_session_dbc_locks() {
    const char *__function_name = "init_session_dbc_locks";
    char* folders[] = { JA_SESSION_DBC_LOCK_FOLDER_NAME, NULL };
    char filepath[JA_FILE_PATH_LEN];
    get_jaz_folder_path(&locked_folder);

    if(ja_folders_prepare(folders, &locked_folder) != SUCCEED) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not prepare folder for ja_session.", __function_name);
        return FAIL;
    }

    strcat(locked_folder, "/");
    strcat(locked_folder, JA_SESSION_DBC_LOCK_FOLDER_NAME);
    zabbix_log(LOG_LEVEL_DEBUG, "In %s(), locked_folder: %s", __function_name, locked_folder);
    if(ja_folder_clean(locked_folder) != SUCCEED) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not clean folder for ja_session.", __function_name);
        return FAIL;
    }

    for (int i = 0; i < CONFIG_DB_CON_COUNT; i++) {
        zbx_snprintf(filepath, JA_FILE_PATH_LEN, "%s/lock_%d", locked_folder, i);
        // crate lock file
        if (ja_file_create_a(filepath) != SUCCEED) {
            zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not create lock file for ja_session.", __function_name);
            return FAIL;
        }
    }
}

void ja_session_dbc_lock_detail(int process_id, char *session_id, char *lock_file, char *lock_folder) {
    const char *__function_name = "ja_session_dbc_lock_detail";
    int start_no = process_id % CONFIG_DB_CON_COUNT;

    while(1) {
        zbx_snprintf(lock_file, sizeof(locked_file), "%s/lock_%d", lock_folder, start_no);

        if (ja_check_file_folder_exist(lock_folder) != SUCCEED) {
            zabbix_log(LOG_LEVEL_WARNING, "In %s(), %s does not exist.", __function_name, lock_folder);
            init_session_dbc_locks();
        }
        if (ja_check_file_folder_exist(lock_file) != SUCCEED) {
            zabbix_log(LOG_LEVEL_WARNING, "In %s(), %s does not exist.", __function_name, lock_file);
            ja_file_create_a(lock_file);
        }

        file_descriptor = open(lock_file, O_RDWR);
        if(file_descriptor == -1) {
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Can not open file: %s", __function_name, lock_file);
            goto next;
        }

        if (ja_lock_file(file_descriptor) == SUCCEED) {
            json_data = json_object_new_object(); //create new json object
            json_object_object_add(json_data, JA_SES_ID_NAME, json_object_new_string(session_id));
            json_object_object_add(json_data, JA_SES_PID_NAME, json_object_new_int(process_id));

            if(ja_write_jsonObj_to_file(json_data, lock_file) != SUCCEED) {
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not write file: %s", __function_name, locked_file);
                json_object_put(json_data);
                close(file_descriptor);
                goto next;
            }
            json_object_put(json_data);
            break;
        } else {
            close(file_descriptor);
        }
next:
        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Connection is not avalible yet, connection file: %s", __function_name, locked_file);
        start_no++;
        if(start_no >= CONFIG_DB_CON_COUNT) {
            start_no = 0;
        }
        sleep(10);
        continue;
    }
}

void ja_session_dbc_lock(const char *session_id, int process_id) {
    const char *__function_name = "ja_session_dbc_lock";

    get_jaz_folder_path(&locked_folder);
    strcat(locked_folder, "/");
    strcat(locked_folder, JA_SESSION_DBC_LOCK_FOLDER_NAME);

    if (ja_check_file_folder_exist(locked_folder) != SUCCEED) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), %s does not exist.", __function_name, locked_folder);
        init_session_dbc_locks();
    }

    ja_session_dbc_lock_detail(process_id, session_id, &locked_file, &locked_folder);
}

void ja_session_dbc_unlock() {
    const char *__function_name = "ja_session_dbc_unlock";

    if (ja_check_file_folder_exist(locked_file) == SUCCEED) {
        while(1) {
            if(ja_file_clean(locked_file) == SUCCEED) {
                if(ja_unlock_file(file_descriptor) == SUCCEED) {
                    close(file_descriptor);
                    break;
                }
            }
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Failed to unlock file: %s, retrying in 15s.", __function_name, locked_file);
            sleep(15);
        }
    } else {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not find locked file: %s", __function_name, locked_file);
        close(file_descriptor);
    }
}

// int ja_session_dbc_unlock_with_ssID(char *session_id) {
//     const char *__function_name = "ja_session_dbc_unlock_with_ssID";
//     int file_size = 0;
//     char lock_folder[JA_FILE_PATH_LEN];

//     get_jaz_folder_path(lock_folder);
//     strcat(lock_folder, "/");
//     strcat(lock_folder, JA_SESSION_DBC_LOCK_FOLDER_NAME);

//     if (ja_check_file_folder_exist(lock_folder) != SUCCEED) {
//         zabbix_log(LOG_LEVEL_WARNING, "In %s(), %s does not exist.", __function_name, JA_SESSION_DBC_LOCK_FOLDER_NAME);
//         init_session_dbc_locks();
//         return SUCCEED;
//     }

//     for(int i = 0; i < CONFIG_DB_CON_COUNT; i++) {
//         zbx_snprintf(locked_file, sizeof(locked_file), "%s/lock_%d", lock_folder, i);

//         if (ja_check_file_folder_exist(locked_file) != SUCCEED) {
//             zabbix_log(LOG_LEVEL_WARNING, "In %s(), %s does not exist.", __function_name, lock_folder);
//             ja_file_create_a(locked_file);
//             continue;
//         }

//         file_size = ja_file_getsize(locked_file);
//         if (file_size <= 0) {
//             continue;;
//         }

//         json_data = json_object_from_file(locked_file);
//         if (json_data == NULL || is_error(json_data) || json_object_get_type(json_data) != json_type_object) {
//             zabbix_log(LOG_LEVEL_WARNING, "Json data is not readable in %s()", __function_name);
//             ja_file_clean(locked_file);
//             continue;
//         }

//         json_object_object_foreach(json_data, key, val) {             
//             if(strcmp(key, JA_SES_ID_NAME) == 0 && strcmp(val, session_id) == 0) {
//                 return ja_file_clean(locked_file);
//             }
//         }
//     }
// }

int ja_is_session_dbc_avaliable() {
    char lock_folder[JA_FILE_PATH_LEN];
    char lock_file[JA_FILE_PATH_LEN];
    int lock_file_size = 1;

    get_jaz_folder_path(lock_folder);
    strcat(lock_folder, "/");
    strcat(lock_folder, JA_SESSION_DBC_LOCK_FOLDER_NAME);

    for(int i = 0; i < CONFIG_DB_CON_COUNT; i++) {
        zbx_snprintf(lock_file, sizeof(lock_file), "%s/lock_%d", lock_folder, i);
        lock_file_size = ja_file_getsize(lock_file);
        if(lock_file_size == 0) {
            return SUCCEED;
        }
    }
    return FAIL;
}
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

#if defined(ZABBIX_SERVICE)
#include "service.h"
#elif defined(ZABBIX_DAEMON)
#include "daemon.h"
#endif
#ifdef _WINDOWS
#include <dirent.h>
#endif
#include "jacommon.h"
#include "jajobobject.h"
#include "jatcp.h"
#include "jatelegram.h"
#include "jaagent.h"
#include "jafcopy.h"
#include "listener.h"
#include "jafile.h"
#include "jajobfile.h"
#include "errno.h" //for error number fwrite/fread
#include "time.h"  //get current local time
#include "jastr.h"
#include <json-c/json.h>

extern int CONFIG_JA_LISTEN_LOOP_TIMEOUT;
#ifndef _WINDOWS
int listen_mthread_pid;

static void ja_sigchild_handler(int sig)
{
    switch (sig)
    {
    case SIGCHLD:
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        break;
    default:
        break;
    }
}
#endif
typedef struct
{
    ja_job_object *job;
    zbx_sock_t *s;

} ja_jobrun_params;
char serverIPs_file[JA_FILE_PATH_LEN];
json_object *json_data = NULL;
char *json_server_ip;

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: Write Json_object to serverIPs.json file                         *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: 0 if succeed. -1 if failed to write                          *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
int write_IP_jsonfile(json_object *json_array, char *data_file)
{
    const char *__function_name = "write_IP_jsonfile";
    FILE *fp;
    char *data = NULL;
    int ret = SUCCEED, file_size, write_count;
    char folderpath[JA_FILE_PATH_LEN];
    data = zbx_strdup(data, (char *)json_object_to_json_string(json_array));
    int createFolder = 0;

    if (data == NULL)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(),Parsing to json string failed.[%s] ", __function_name, data_file);
        ret = FAIL;
        goto error;
    }
    zbx_snprintf(folderpath, sizeof(folderpath), "%s%cserverIPs", CONFIG_TMPDIR, JA_DLM);
#if defined _WINDOWS
    createFolder = _mkdir(folderpath);
#else
    createFolder = mkdir(folderpath, JA_PERMISSION);
#endif
    if (createFolder != 0 && errno != EEXIST)
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Directory cannot be created.Path: %s", __function_name, folderpath);
        ret = FAIL;
        goto error;
    }
    zabbix_log(LOG_LEVEL_DEBUG, "In %s(),json data :[%s] to be written in [%s] ", __function_name, data, data_file);
    fp = fopen(data_file, "w");
    if (fp == NULL)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(),failed to open data file: [%s] (%s)", __function_name, data_file, strerror(errno));
        ret = FAIL;
        goto error;
    }
    file_size = strlen(data);
    if (0 >= file_size)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), [data] is empty.", __function_name);
        fclose(fp);
        ret = FAIL;
        goto error;
    }
    fseek(fp, 0L, SEEK_SET);
    write_count = fwrite(data, file_size, 1, fp);
    if (write_count != 1)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), file write failed. data : [%s], to file : %s", __function_name, data, data_file);
        ret = FAIL;
        goto error;
    }
    fclose(fp);

error:
    zbx_free(data);
    return ret;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: write serverIps.json file                                         *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
void write_IP_json_data(json_object *json_data, char *data_file)
{
    const char *__function_name = "write_IP_json_data";

    while (1)
    {
        if (write_IP_jsonfile(json_data, data_file) == SUCCEED)
        {
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Data updated to %s", __function_name, data_file);
            break;
        }
        else
        {
            zabbix_log(LOG_LEVEL_WARNING, "In %s(), Failed to write data to %s", __function_name, data_file);
            zbx_sleep(30);
        }
    }
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: read serverIps.json file                                          *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: json_object if no error occurred. NUll if error occurred at  *
 *               reading file.                                                *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
json_object *read_IP_json_data(char *data_file)
{
    int ret = FAIL;
    const char *__function_name = "read_IP_json_data";
    json_object *json_data;

    json_data = json_object_from_file(data_file);

    if (json_data == NULL)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), Json data is NULL.", __function_name);
        return NULL;
    }
    else if (is_error(json_data))
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), Got error in reading file: %s, Error: %s.", __function_name, data_file, strerror(errno));
        return NULL;
    }
    else if (json_object_get_type(json_data) != json_type_object)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), Json data is not Object type.", __function_name);
        return NULL;
    }

    return json_data;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: Check serverID is in serverIPs.json file or not                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: server's IP if server's id exists. NUll if server's ID and   *
 *               server's IP doesn't exist                                    *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
char *check_serverID(char *serverID, json_object *json_data)
{
    const char *__function_name = "check_serverID";

    json_object_object_foreach(json_data, key, val)
    {
        if (strcmp(key, serverID) == 0 && val != NULL && json_object_is_type(val, json_type_string))
        {
            return json_object_get_string(val);
        }
        else if (strcmp(key, serverID) == 0 && (val == NULL || !json_object_is_type(val, json_type_string)))
        {
            json_object_object_del(json_data, key);
            write_IP_jsonfile(json_data, serverIPs_file);
        }
    }

    return NULL;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: Check serverID is in serverIPs.json file or not                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: server's IP if server's id exists. NUll if server's ID and   *
 *               server's IP doesn't exist                                    *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
void check_serverID_protoVersion_from_serverIDs(ja_job_object *job, json_object *json_data, char *serverIP)
{
    const char *__function_name = "check_serverID_protoVersion_from_serverIDs";
    json_object *serverIDs_json_array = NULL;
    char *found_server_id = NULL;
    int temp_server_json_object_corrupted, ip_key_found, version_key_found;

    serverIDs_json_array = json_object_object_get(json_data, JA_PROTO_TAG_SERVERIDS);

    if (serverIDs_json_array != NULL && json_object_is_type(serverIDs_json_array, json_type_array))
    {
        size_t serverIDs_json_array_length = json_object_array_length(serverIDs_json_array);

        for (size_t i = 0; i < serverIDs_json_array_length; i++)
        {
            json_object *temp_server_json_object = json_object_array_get_idx(serverIDs_json_array, i);

            if (!temp_server_json_object || !json_object_is_type(temp_server_json_object, json_type_object))
            {
                zabbix_log(LOG_LEVEL_WARNING, "In %s(), json object[%zu] data corrupted, skipping.", __function_name, i);
                continue;
            }

            found_server_id = NULL;
            ip_key_found = 0;
            version_key_found = 0;

            json_object_object_foreach(temp_server_json_object, key, val)
            {
                // Validate key and value
                if (key != NULL && val != NULL &&
                    strcmp(key, JA_PROTO_TAG_SERVERID) == 0 &&
                    json_object_is_type(val, json_type_string) &&
                    strcmp(json_object_get_string(val), job->serverid) == 0)
                {

                    found_server_id = json_object_get_string(val);
                }
                else if (key != NULL && val != NULL && found_server_id != NULL &&
                         strcmp(key, JA_PROTO_TAG_SERVERIP) == 0 &&
                         strcmp(found_server_id, job->serverid) == 0 &&
                         strcmp(serverIP, json_object_get_string(val)) != 0)
                {

                    json_object_object_del(temp_server_json_object, JA_PROTO_TAG_SERVERIP);
                    ip_key_found = 0;
                }
                else if (key != NULL && val != NULL && found_server_id != NULL &&
                         strcmp(key, JA_PROTO_TAG_VERSION) == 0 &&
                         strcmp(found_server_id, job->serverid) == 0 &&
                         job->version != json_object_get_int(val))
                {

                    json_object_object_del(temp_server_json_object, JA_PROTO_TAG_VERSION);
                    version_key_found = 0;
                }
                else if (key != NULL && val != NULL && found_server_id != NULL &&
                         strcmp(key, JA_PROTO_TAG_VERSION) == 0 &&
                         strcmp(found_server_id, job->serverid) == 0 &&
                         job->version == json_object_get_int(val))
                {

                    version_key_found = 1;
                }
                else if (key != NULL && val != NULL && found_server_id != NULL &&
                         strcmp(key, JA_PROTO_TAG_SERVERIP) == 0 &&
                         strcmp(found_server_id, job->serverid) == 0 &&
                         strcmp(serverIP, json_object_get_string(val)) == 0)
                {

                    ip_key_found = 1;
                }
            }

            if (found_server_id != NULL)
            {
                if (!ip_key_found)
                {
                    json_object_object_add(temp_server_json_object, JA_PROTO_TAG_SERVERIP, json_object_new_string(serverIP));
                }
                if (!version_key_found)
                {
                    json_object_object_add(temp_server_json_object, JA_PROTO_TAG_VERSION, json_object_new_int(job->version));
                }
                if (!ip_key_found || !version_key_found)
                {
                    write_IP_json_data(json_data, serverIPs_file);
                }
                break;
            }
        }
    }

    if (found_server_id == NULL)
    {
        if (serverIDs_json_array == NULL || !json_object_is_type(serverIDs_json_array, json_type_array))
        {
            serverIDs_json_array = json_object_new_array();
            json_object_object_add(json_data, JA_PROTO_TAG_SERVERIDS, serverIDs_json_array);
        }

        json_object *server_json_object = json_object_new_object();
        json_object_object_add(server_json_object, JA_PROTO_TAG_SERVERID, json_object_new_string(job->serverid));
        json_object_object_add(server_json_object, JA_PROTO_TAG_SERVERIP, json_object_new_string(serverIP));
        json_object_object_add(server_json_object, JA_PROTO_TAG_VERSION, json_object_new_int(job->version));
        json_object_array_add(serverIDs_json_array, server_json_object);

        write_IP_json_data(json_data, serverIPs_file);
        json_object_put(server_json_object);
    }

    json_object_put(serverIDs_json_array);

    return NULL;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: Gets server ips from jobarranger/tmp/serverIPs/serverIPs.json file*
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
int getServerIPs(char *serverID, char *serverIP)
{
    const char *__function_name = "getServerIPs";
    char file_data[JA_FILE_PATH_LEN];
    zbx_snprintf(file_data, sizeof(file_data), "%s%cserverIPs.json", JA_IPS_FOLDER, JA_DLM);
    json_object *json_data = NULL;
    json_object *serverIDs_json_array = NULL;
    char *found_server_id = NULL;

    json_data = read_IP_json_data(file_data);
    if (json_data != NULL)
    {

        serverIDs_json_array = json_object_object_get(json_data, JA_PROTO_TAG_SERVERIDS);

        if (serverIDs_json_array != NULL && json_object_is_type(serverIDs_json_array, json_type_array))
        {
            size_t serverIDs_json_array_length = json_object_array_length(serverIDs_json_array);

            for (size_t i = 0; i < serverIDs_json_array_length; i++)
            {
                json_object *temp_server_json_object = json_object_array_get_idx(serverIDs_json_array, i);
                if (!temp_server_json_object || !json_object_is_type(temp_server_json_object, json_type_object))
                {
                    zabbix_log(LOG_LEVEL_WARNING, "In %s(), json object[%zu] data corrupted, skipping.", __function_name, i);
                    continue;
                }

                found_server_id = NULL;

                json_object_object_foreach(temp_server_json_object, key, val)
                {
                    // Validate key and value
                    if (key != NULL && val != NULL &&
                        strcmp(key, JA_PROTO_TAG_SERVERID) == 0 &&
                        json_object_is_type(val, json_type_string) &&
                        strcmp(json_object_get_string(val), serverID) == 0)
                    {

                        found_server_id = json_object_get_string(val);
                    }
                    else if (key != NULL && val != NULL && found_server_id != NULL &&
                             strcmp(key, JA_PROTO_TAG_SERVERIP) == 0 &&
                             strcmp(found_server_id, serverID) == 0)
                    {

                        zbx_strlcpy(serverIP, json_object_get_string(val), JA_SERVERID_LEN + 1);
                        json_object_put(json_data);

                        return SUCCEED;
                    }
                }
            }
        }
        json_object_put(json_data);
    }
    else
    {
        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Json_data is NULL.", __function_name);
    }

    return FAIL;
}

int getProtoVersions(char *serverID)
{
    const char *__function_name = "getServerIPs";
    char file_data[JA_FILE_PATH_LEN];
    char *temp_server_ip = NULL;
    zbx_snprintf(file_data, sizeof(file_data), "%s%cserverIPs.json", JA_IPS_FOLDER, JA_DLM);
    json_object *json_data = NULL;
    json_object *serverIDs_json_array = NULL;
    char *found_server_id = NULL;

    json_data = read_IP_json_data(file_data);
    if (json_data != NULL)
    {

        serverIDs_json_array = json_object_object_get(json_data, JA_PROTO_TAG_SERVERIDS);

        if (serverIDs_json_array != NULL && json_object_is_type(serverIDs_json_array, json_type_array))
        {
            size_t serverIDs_json_array_length = json_object_array_length(serverIDs_json_array);

            for (size_t i = 0; i < serverIDs_json_array_length; i++)
            {
                json_object *temp_server_json_object = json_object_array_get_idx(serverIDs_json_array, i);
                if (!temp_server_json_object || !json_object_is_type(temp_server_json_object, json_type_object))
                {
                    zabbix_log(LOG_LEVEL_WARNING, "In %s(), json object[%zu] data corrupted, skipping.", __function_name, i);
                    continue;
                }

                found_server_id = NULL;

                json_object_object_foreach(temp_server_json_object, key, val)
                {
                    // Validate key and value
                    if (key != NULL && val != NULL &&
                        strcmp(key, JA_PROTO_TAG_SERVERID) == 0 &&
                        json_object_is_type(val, json_type_string) &&
                        strcmp(json_object_get_string(val), serverID) == 0)
                    {

                        found_server_id = json_object_get_string(val);
                    }
                    else if (key != NULL && val != NULL && found_server_id != NULL &&
                             strcmp(key, JA_PROTO_TAG_VERSION) == 0 &&
                             strcmp(found_server_id, serverID) == 0)
                    {

                        int temp_version_number = json_object_get_int(val);
                        json_object_put(json_data);

                        return temp_version_number;
                    }
                }
            }
        }

        json_object_put(json_data);
    }
    else
    {
        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Json_data is NULL.", __function_name);
    }

    return 0;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose: check serverID in serverIPs.json file and update file if it's     *
 *          necessary                                                         *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Htoo Zaw Aung                                                      *
 *                                                                            *
 ******************************************************************************/
void check_IP_jsonfile(char *serverIPs_file, ja_job_object *job, char *serverIP)
{
    const char *__function_name = "check_IP_jsonfile";
    json_object *serverIDs_json_array = NULL;
    json_object *serverID_json_object = NULL;

    json_data = read_IP_json_data(serverIPs_file);
    if (json_data != NULL)
    {
        json_server_ip = check_serverID(job->serverid, json_data);

        if (json_server_ip == NULL || strcmp(json_server_ip, serverIP) != 0)
        {
            json_object_object_add(json_data, job->serverid, json_object_new_string(serverIP));
            write_IP_json_data(json_data, serverIPs_file);
        }

        check_serverID_protoVersion_from_serverIDs(job, json_data, serverIP); // check new serverids format
    }
    else
    {
        // file is not readable state
        json_data = json_object_new_object(); // create new json object
        json_object_object_add(json_data, job->serverid, json_object_new_string(serverIP));

        // serverids:
        serverIDs_json_array = json_object_new_array();
        serverID_json_object = json_object_new_object();

        json_object_object_add(serverID_json_object, JA_PROTO_TAG_SERVERID, json_object_new_string(job->serverid));
        json_object_object_add(serverID_json_object, JA_PROTO_TAG_SERVERIP, json_object_new_string(serverIP));
        json_object_object_add(serverID_json_object, JA_PROTO_TAG_VERSION, json_object_new_int(job->version));

        json_object_array_add(serverIDs_json_array, serverID_json_object);
        json_object_object_add(json_data, JA_PROTO_TAG_SERVERIDS, serverIDs_json_array);

        write_IP_json_data(json_data, serverIPs_file); // overide old file with new obeject
    }

    json_object_put(json_data);
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/

ZBX_THREAD_ENTRY(job_run_thread, args)
{
    struct tm *tm;
    int proc_run_flag = 0;
    char current_time[20];
    char filename[JA_FILE_PATH_LEN];
    char data_file[JA_FILE_PATH_LEN];
    char data_filename[JA_FILE_PATH_LEN];
    char jobid_datetime[JA_FILE_PATH_LEN];
    char full_datafile_path[JA_FILE_PATH_LEN];
    const char *__function_name = "job_run_thread";
    int ret = FAIL;
    int same_job = FAIL;
    ja_job_object *job;
    zbx_sock_t *s;
    s = (zbx_sock_t *)malloc(sizeof(zbx_sock_t));
    memcpy(s, (zbx_sock_t *)((ja_jobrun_params *)args)->s, sizeof(zbx_sock_t));
    job = (ja_job_object *)malloc(sizeof(ja_job_object));
    memcpy(job, (ja_job_object *)((ja_jobrun_params *)args)->job, sizeof(ja_job_object));
#ifndef _WINDOWS
    zbx_sock_close(((zbx_sock_t *)((ja_jobrun_params *)args)->s)->sockets[0]);
    zbx_sock_close(s->sockets[0]);
#else
    zbx_free(((ja_jobrun_params *)args)->job);
    zbx_free(((ja_jobrun_params *)args)->s);
#endif

    zbx_free(args);
    ja_job_object *temp_job;
    temp_job = (ja_job_object *)zbx_malloc(NULL, sizeof(ja_job_object));
// Added by ThihaOo@DAT 11/01/2021
#ifdef _WINDOWS
    struct _timeb currentTime;
    _ftime(&currentTime);
    tm = localtime(&currentTime.time);
#else
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    tm = localtime(&currentTime.tv_sec);
#endif
    zbx_snprintf(current_time, 20, "%.4d%.2d%.2d%.2d%.2d%.2d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    zabbix_log(LOG_LEVEL_DEBUG, "In %s() current Datetime is %s.", __function_name, current_time);
    // Added by ThihaOo@DAT 11/01/2021
    if (job->method != JA_AGENT_METHOD_KILL)
    {
        // check for job ID and server ID.
        zbx_snprintf(jobid_datetime, sizeof(jobid_datetime), ZBX_FS_UI64 "-%s.job", job->jobid, current_time);

        proc_run_flag = ja_jobfile_check_processexist(filename, jobid_datetime, NULL); // 0: No running process; 1: Running process
        // if the same jobid from same server is running,
        if (proc_run_flag == 1)
        {
            // check for server
            if (FAIL == ja_job_object_init(temp_job))
            {
                zbx_snprintf(job->message, sizeof(job->message), "job object cannot be initialized. job is Null.");
                if (job->result == JA_RESPONSE_SUCCEED)
                    job->result = JA_RESPONSE_FAIL;
                goto job_resend;
            }
            int datafile_read_flg = SUCCEED;
            get_jobid_datetime(filename, data_filename);
            zbx_snprintf(full_datafile_path, sizeof(full_datafile_path), "%s%c%s.json", JA_DATA_FOLDER, JA_DLM, data_filename);
            datafile_read_flg = read_datafile(temp_job, full_datafile_path);
            int comp = strncmp(job->serverid, temp_job->serverid, sizeof(job->serverid));
            if (datafile_read_flg != FAIL && temp_job != NULL && strncmp(job->serverip, temp_job->serverip, sizeof(job->serverip)) == 0)
            {
                zabbix_log(LOG_LEVEL_WARNING, "In %s(), Job with same job id is already running. inner job id:" ZBX_FS_UI64, __function_name, job->jobid);
                zbx_snprintf(job->message, sizeof(job->message), "Job with same job id is already running.");
                job->result = JA_RESPONSE_SUCCEED;
                same_job = SUCCEED;
                goto job_resend;
            }
            else if (datafile_read_flg == FAIL || temp_job == NULL)
            {
                zabbix_log(LOG_LEVEL_DEBUG, "In %s(), cannot read data file to compare server ip.", __function_name);
            }
            else
            {
                zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), server ip does not match.", __function_name);
            }
        }
        // write temp file
        if (ja_jobstatus_file(job, &current_time) == FAIL)
        {
            zabbix_log(LOG_LEVEL_ERR, "In %s() Temp file write failed.", __function_name);
            if (job->result == JA_RESPONSE_SUCCEED)
                job->result = JA_RESPONSE_FAIL;
            goto job_resend;
        }
    }
    // End
    //  zbx_snprintf(data_file, sizeof(data_file), "%s%c" ZBX_FS_UI64 "-%s.json", JA_DATA_FOLDER, JA_DLM, job->jobid, current_time);
    //  zabbix_log(LOG_LEVEL_DEBUG, "In %s() job type is : %s and data file is : %s", __function_name,job->type,data_file);
    //  if (ja_agent_begin(job,&current_time, & data_file) != SUCCEED) {
    //      zabbix_log(LOG_LEVEL_ERR, "In %s() job id :"ZBX_FS_UI64" job agent begin failed.", __function_name, job->jobid);
    //          job->result = JA_RESPONSE_FAIL;
    //  }
    ret = SUCCEED;
job_resend:
    ret = ja_tcp_send_to(s, job, CONFIG_TIMEOUT);
    if (same_job == FAIL)
    {
        if (ret == SUCCEED)
        {
            // create data file
            if (job->method != JA_AGENT_METHOD_KILL)
            {
                if (strcmp(job->type, JA_PROTO_VALUE_REBOOT) == 0)
                {
                    job->signal = 1;
                }
                if (ja_write_file_data(job, &current_time, 0) == FAIL)
                {
                    zabbix_log(LOG_LEVEL_ERR, "In %s() Data file write failed.", __function_name);
                    if (job->result == JA_RESPONSE_SUCCEED)
                        job->result = JA_RESPONSE_FAIL;
                    goto job_resend;
                }
            }
            // move from temp to  begin
            zbx_snprintf(data_file, sizeof(data_file), "%s%c" ZBX_FS_UI64 "-%s.json", JA_DATA_FOLDER, JA_DLM, job->jobid, current_time);
            if (ja_agent_begin(job, &current_time, &data_file) != SUCCEED)
            {
                zabbix_log(LOG_LEVEL_ERR, "In %s() job id :" ZBX_FS_UI64 " job agent begin failed.", __function_name, job->jobid);
                job->result = JA_RESPONSE_FAIL;
            }
        }
        else
        {
            // delete temp file on ack failed
            ja_delete_file(job, &current_time, 2);
        }
    }
    same_job = FAIL;
    zbx_tcp_unaccept(s);
    zbx_free(temp_job);
    zbx_free(job);
    zbx_free(s);
    zbx_thread_exit(0);
}

void ja_job_clean_temp()
{
    int i = 0, fp_ret = -1, ext_cd = -1;
    DIR *dir;
    struct dirent *entry;
    const char *__function_name = "ja_job_clean_temp";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s(),", __function_name);

    dir = opendir(JA_TEMP_FOLDER);
    if (dir == NULL)
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s, [%s] cannot be opened.%s", __function_name, JA_TEMP_FOLDER, strerror(errno));
        return;
    }
    while (NULL != (entry = readdir(dir)))
    {
        char full_path[JA_FILE_PATH_LEN];
        zbx_snprintf(full_path, sizeof(full_path), "%s%c%s", JA_TEMP_FOLDER, JA_DLM, entry->d_name);
        if (strstr(entry->d_name, ".job") != NULL)
        {
            if (remove(full_path) != 0)
            {
                zabbix_log(LOG_LEVEL_ERR, "Failed to remove file: %s. Error: %s", full_path, strerror(errno));
            }
        }
    }
    closedir(dir);
}

void ja_chk_job_begin(ja_job_object *job)
{
    const char *__function_name = "ja_chk_job_begin";
    int pid, i = 0, is_exist = FAIL;
    int folder_type = 1;
    int chk;
    char jobid[JA_JOB_ID_LEN];
    char filename[JA_FILE_NAME_LEN];
    char load_filename[JA_FILE_PATH_LEN];
    int loop_cnt = 0;
    DIR *exec_dir;
    struct dirent *entry;
    int chk_lock_file_descriptor;

    zabbix_log(LOG_LEVEL_DEBUG, "In %s(),", __function_name);
    zbx_uint64_t *host_job_list = (zbx_uint64_t *)malloc(job->size_of_host_running_job * sizeof(zbx_uint64_t)); // added by NyanLinnSoe 28/07/2023
    if (host_job_list == NULL)
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s(),cannot allocate memory for host job list.", __function_name);
        goto err;
    }
    memcpy(host_job_list, job->host_running_job, job->size_of_host_running_job * sizeof(zbx_uint64_t)); // added by NyanLinnSoe 28/07/2023

#ifndef _WINDOWS
    signal(SIGCHLD, SIG_IGN);

    pid = fork();
    if (pid < 0)
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Error creating child process. jobid :" ZBX_FS_UI64, __function_name, job->jobid);
        return;
    }
    else if (pid > 0)
    {
        return;
    }
    signal(SIGUSR1, alarm_handler);
    jaalarm(CONFIG_JA_LISTEN_LOOP_TIMEOUT);
#endif
    char status_folder_path[JA_FILE_PATH_LEN];
    char data_folder_path[JA_FILE_PATH_LEN];
    // 1. check job by job flag folders.

    for (i = 0; i < job->size_of_host_running_job; i++)
    {

        zabbix_log(LOG_LEVEL_DEBUG, "Attempting to lock : %s", JA_CHK_LOCK_FILE);

        chk_lock_file_descriptor = ja_lock_resource(JA_CHK_LOCK_FILE, JA_LOCK_EX | JA_LOCK_NB, __function_name);
        folder_type = 1;
        chk = SUCCEED;
        if (host_job_list[i] == NULL)
        {
            ja_unlock_resource(chk_lock_file_descriptor);
            break;
        }
        job->jobid = host_job_list[i];
        zbx_snprintf(jobid, sizeof(jobid), ZBX_FS_UI64, host_job_list[i]);
        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), job status check starts. jobid : %s", __function_name, jobid);

        while (1)
        {
            switch (folder_type)
            {
            case JA_TYPE_STATUS_TEMP:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_TEMP_FOLDER);
                break;
            case JA_TYPE_STATUS_BEGIN:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_BEGIN_FOLDER);
                break;
            case JA_TYPE_STATUS_EXEC:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_EXEC_FOLDER);
                break;
            case JA_TYPE_STATUS_END:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_END_FOLDER);
                break;
            case JA_TYPE_STATUS_CLOSE:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_CLOSE_FOLDER);
                break;
            case JA_TYPE_STATUS_ERROR:
                zbx_snprintf(status_folder_path, JA_FILE_PATH_LEN, "%s", JA_ERROR_FOLDER);
                break;
            default:
                chk = FAIL;
                break;
            }
            if (chk == FAIL)
            {
                zabbix_log(LOG_LEVEL_DEBUG, "In %s(), status check failed. jobid :%s", __function_name, jobid);
                break;
            }

            // check for execution folder.
            exec_dir = opendir(status_folder_path);
            if (exec_dir == NULL)
            {
                zabbix_log(LOG_LEVEL_ERR, "In %s()  status folder [%s] cannot be opened.", __function_name, status_folder_path);
                break;
            }
            while (NULL != (entry = readdir(exec_dir)))
            {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                {
                    continue;
                }
                if (strncmp(entry->d_name, jobid, strlen(jobid)) == 0 && strstr(entry->d_name, ".job") != NULL)
                {
                    zabbix_log(LOG_LEVEL_DEBUG, "In %s(), directory:%s, file name:%s, jobid: %s", __function_name, status_folder_path, entry->d_name, jobid);
                    zbx_snprintf(filename, sizeof(entry->d_name), "%s", entry->d_name);
                    is_exist = SUCCEED;
                    break;
                }
            }
            closedir(exec_dir);
            if (is_exist == SUCCEED)
            {
                break;
            }
            folder_type++;
        }
        if (is_exist == SUCCEED)
        {
            is_exist = FAIL;
            // neglect if it is running job.
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), job information exists.jobid :%s, folder_type :%d", __function_name, jobid, folder_type);
            if (folder_type == JA_TYPE_STATUS_TEMP || folder_type == JA_TYPE_STATUS_BEGIN || folder_type == JA_TYPE_STATUS_EXEC || folder_type == JA_TYPE_STATUS_END)
            {
                zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), jobid :%s is currently executing. Skip check.", __function_name, jobid);
                ja_unlock_resource(chk_lock_file_descriptor);

                continue;
            }
            // Data files move.
            zbx_snprintf(load_filename, strlen(filename) - 3, "%s", filename);

            if (folder_type == JA_TYPE_STATUS_CLOSE)
            {
                zbx_snprintf(data_folder_path, JA_FILE_PATH_LEN, "%s%c%s", JA_CLOSE_FOLDER, JA_DLM, load_filename);
            }
            else if (folder_type == JA_TYPE_STATUS_ERROR)
            {
                zbx_snprintf(data_folder_path, JA_FILE_PATH_LEN, "%s%c%s", JA_ERROR_FOLDER, JA_DLM, load_filename);
            }
            if (create_check_res_files(job, data_folder_path, load_filename) == FAIL)
            {
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot create check result files. jobid : %s, [Error No: %d], [Error Message: %s]", __function_name, jobid, errno, strerror(errno));
                ja_unlock_resource(chk_lock_file_descriptor);

                continue;
            }
            if (FAIL == rmdir(data_folder_path))
            {
                zabbix_log(LOG_LEVEL_WARNING, "In %s() [%s] folder deletion failed.", __function_name, data_folder_path);
            }
        }
        else
        {
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), running job does not exists. jobid :%s", __function_name, jobid);
            if (create_check_res_files(job, NULL, load_filename) == FAIL)
            {
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot create check result files. jobid : %s, [Error No: %d], [Error Message: %s]", __function_name, jobid, errno, strerror(errno));
            }
        }
        ja_unlock_resource(chk_lock_file_descriptor);

        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(),job check finished. jobid: %s", __function_name, jobid);
    }
    zbx_free(host_job_list);
err:
#ifdef _WINDOWS
    return 0;
#else
    jaalarm(0);
    exit(0);
#endif
}
#ifdef _WINDOWS
int ja_chk_job_begin_thread(LPVOID lpParam)
{
    ja_job_object *job = (ja_job_object *)lpParam;
    ja_chk_job_begin(job);
    return 0;
}
int ja_chk_job_begin_timeout_thread(LPVOID lpParam)
{
    HANDLE hThread;
    DWORD threadId;
    int ret = SUCCEED;
    ja_job_object *job = (ja_job_object *)lpParam;
    // check_job->host_running_job = (zbx_uint64_t*)malloc((job->size_of_host_running_job + 1) * sizeof(zbx_uint64_t));

    do
    {
        hThread = CreateThread(NULL, 0, ja_chk_job_begin_thread, job, 0, &threadId);

        if (hThread == NULL)
        {
            zabbix_log(LOG_LEVEL_ERR, "Failed to create thread. Error code: %lu\n", GetLastError());
            return 1;
        }
        DWORD waitResult = WaitForSingleObject(hThread, CONFIG_JA_LISTEN_LOOP_TIMEOUT * 1000);
        // DWORD waitResult = WaitForSingleObject(hThread, 1 * 100);
        if (waitResult == WAIT_OBJECT_0)
        {
            DWORD threadExitCode;
            GetExitCodeThread(hThread, &threadExitCode); // Get the exit code of the thread
                                                         // Check the exit code against your error codes
            switch (threadExitCode)
            {
            case 0:
                zabbix_log(LOG_LEVEL_DEBUG, "Check begin job Thread finished successfully.\n");
                break;
            case 6:
                zabbix_log(LOG_LEVEL_ERR, "File creation thread timed out.\n");
                break;
            default:
                zabbix_log(LOG_LEVEL_ERR, "Executing job check process error occurred. exit code : %d\n", threadExitCode);
                break;
            }
        }
        else if (waitResult == WAIT_FAILED)
        {
            zabbix_log(LOG_LEVEL_ERR, "Executing job check thread failed with error code :[%s]", GetLastError());
        }
        else if (waitResult == WAIT_TIMEOUT)
        {
            zabbix_log(LOG_LEVEL_ERR, "Executing job check timed out.");
            TerminateThread(hThread, 0);
            ret = FAIL;
        }
        // Close the thread handle
        CloseHandle(hThread);
    } while (ret != SUCCEED);

    zbx_free(job);
    return 0;
}
#endif
/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int process_listener(zbx_sock_t *s, ja_job_object *job)
{
    int ret;
    ZBX_THREAD_HANDLE child_thread;
    char socket_ip[JA_SERVERID_LEN];
    const char *__function_name = "process_listener";
    zbx_snprintf(serverIPs_file, sizeof(serverIPs_file), "%s%cserverIPs.json", JA_IPS_FOLDER, JA_DLM);

    zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

    // End
    socket_ip[0] = '\0';
    if (ja_tcp_check_security(s, CONFIG_HOSTS_ALLOWED, 1, socket_ip) == FAIL)
    {
        zbx_tcp_unaccept(s);
        zabbix_log(LOG_LEVEL_ERR, "In %s() ja_tcp_check_security() check failed.", __function_name);
#ifdef _WINDOWS
        zbx_free(job);
#endif
        return FAIL;
    }
    zabbix_log(LOG_LEVEL_DEBUG, "In %s() socket_ip=[%s]", __function_name, socket_ip);
    if (FAIL == ja_job_object_init(job))
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s() job object cannot be initialized. job is Null.", __function_name);
        return FAIL;
    }
    else
    {
        ret = ja_tcp_recv_to_server(s, job, CONFIG_TIMEOUT);
        if(ret == FAIL || ret == RECEIVED_NULL){
            zbx_tcp_unaccept(s);
#ifdef _WINDOWS
            zbx_free(job);
#endif
            return ret;
        }
        zabbix_log(LOG_LEVEL_DEBUG, "In %s() job object accepted.", __function_name);
        job->result = JA_RESPONSE_SUCCEED;
    }

    zbx_snprintf(job->serverip, sizeof(job->serverip), "%s", socket_ip);

    if (job->method == JA_AGENT_METHOD_KILL)
    {
        if (check_job_abort_flag(job->jobid, 1) == JA_ABORT_FILE_EXIST)
        {
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s() job:" ZBX_FS_UI64 " is already under abort process.", __function_name, job->jobid);
            job->result = JA_RESPONSE_SUCCEED;
            ret = ja_tcp_send_to(s, job, CONFIG_TIMEOUT);
            zbx_tcp_unaccept(s);
#ifdef _WINDOWS
            zbx_free(job);
#endif
            return SUCCEED;
        }
    }

    if (strcmp(job->kind, JA_PROTO_VALUE_JOBRUN) == 0)
    {
        check_IP_jsonfile(serverIPs_file, job, socket_ip); // check for serverIPs.json, author:htoozawaung

        zbx_snprintf(job->kind, sizeof(job->kind), "%s",
                     JA_PROTO_VALUE_JOBRUN_RES);
        // ZBX_THREAD_HANDLE *threads = NULL;
        // threads =(ZBX_THREAD_HANDLE *) zbx_calloc(threads, 1,sizeof(ZBX_THREAD_HANDLE));
        ja_jobrun_params *param = malloc(sizeof(ja_jobrun_params));
        param->job = job;
#ifdef _WINDOWS
        param->s = malloc(sizeof(zbx_sock_t));
        memcpy(param->s, s, sizeof(zbx_sock_t));
#else
        param->s = s;
#endif
        child_thread = zbx_thread_start(job_run_thread, param);
#ifndef _WINDOWS
        zbx_sleep(0);
        zbx_free(param);
        zbx_sock_close(s->socket);
        s->socket = s->socket_orig; /* restore main socket */
        s->socket_orig = ZBX_SOCK_ERROR;
        s->accepted = 0;
        struct sigaction phan;
        phan.sa_sigaction = (void *)ja_sigchild_handler;
        sigaction(SIGCHLD, &phan, NULL);
#endif
        ret = SUCCEED;
    }
    else if (strcmp(job->kind, JA_PROTO_VALUE_FCOPY) == 0)
    {
        check_IP_jsonfile(serverIPs_file, job, socket_ip); // check for serverIPs.json, author:htoozawaung

        zbx_snprintf(job->kind, sizeof(job->kind), "%s",
                     JA_PROTO_VALUE_FCOPY_RES);
        if (ret == SUCCEED) {
            ja_fcopy_begin(job, s);
            return SUCCEED;
        }
        else
        {
            if (job->result == JA_RESPONSE_SUCCEED)
                job->result = JA_RESPONSE_FAIL;
            ret = ja_tcp_send_to(s, job, CONFIG_TIMEOUT);
            zbx_tcp_unaccept(s);
        }
    }
    else if (strcmp(job->kind, JA_PROTO_VALUE_CHKJOB) == 0)
    {
        check_IP_jsonfile(serverIPs_file, job, socket_ip); // check for serverIPs.json, author:htoozawaung

        zabbix_log(LOG_LEVEL_DEBUG, "In %s(), job kind is :%s", __function_name, job->kind);
        // For Windows :
//|----listener Thread----
//|    |----Non-blocking child thread----
//|    |   |----alarm timeout controller----
//|    |   |   |----ja_chk_job_begin()----
#ifdef _WINDOWS
        HANDLE hThread_chkjob;
        DWORD threadId_chkjob;
        // Create a thread
        ja_job_object *job_orig = (ja_job_object *)malloc(sizeof(ja_job_object));
        if (job_orig == NULL)
        {
            zabbix_log(LOG_LEVEL_ERR, "In %s(),Cannot create job pointer.", __function_name);
            zbx_snprintf(job->message, sizeof(job->message), "Cannot create job pointer.");
            job->result = JA_RESPONSE_FAIL;
            goto contd;
        }
        ja_job_object_init(job_orig);
        // zbx_snprintf(jobid, sizeof(jobid), ZBX_FS_UI64, host_job_list[i]);
        job_orig->size_of_host_running_job = job->size_of_host_running_job;
        zbx_snprintf(job_orig->kind, sizeof(job->kind), "%s", job->kind);
        job_orig->version = job->version;
        zbx_snprintf(job_orig->serverid, sizeof(job->serverid), "%s", job->serverid);
        zbx_snprintf(job_orig->hostname, sizeof(job->hostname), "%s", job->hostname);
        zbx_snprintf(job_orig->serverip, sizeof(job->serverip), "%s", job->serverip);
        job_orig->host_running_job = (zbx_uint64_t *)malloc(job->size_of_host_running_job * sizeof(zbx_uint64_t)); // added by NyanLinnSoe 28/07/2023
        if (job_orig->host_running_job == NULL)
        {
            zabbix_log(LOG_LEVEL_INFORMATION, "In %s(),cannot allocate memory for host job list.", __function_name);
            zbx_snprintf(job->message, sizeof(job->message), "Cannot allocate memory for host job list.");
            job->result = JA_RESPONSE_FAIL;
            goto contd;
        }
        memcpy(job_orig->host_running_job, job->host_running_job, job->size_of_host_running_job * sizeof(zbx_uint64_t)); // added by NyanLinnSoe 28/07/2023
        hThread_chkjob = CreateThread(NULL, 0, ja_chk_job_begin_timeout_thread, job_orig, 0, &threadId_chkjob);

        if (hThread_chkjob == NULL)
        {
            zabbix_log(LOG_LEVEL_ERR, "Failed to create thread. Error code: %lu\n", GetLastError());
            zbx_snprintf(job->message, sizeof(job->message), "Failed to create thread. Error code: %lu\n", GetLastError());
            job->result = JA_RESPONSE_FAIL;
            goto contd;
        }
        CloseHandle(hThread_chkjob);
#else
        ja_chk_job_begin(job);
#endif
    contd:
        ret = ja_tcp_send_to(s, job, CONFIG_TIMEOUT);
        if (job->host_running_job != NULL)
        {
            zbx_free(job->host_running_job);
            job->host_running_job = NULL;
        }
        zbx_tcp_unaccept(s);
#ifdef _WINDOWS
        zbx_free(job);
#endif
    }
    else if (strcmp(job->kind, JA_PROTO_VALUE_IPCHANGE) == 0)
    {
        ja_job_object job_res;
        ja_job_object_init(&job_res);
        job_res.version = 1.3;
        ret = FAIL;
        zbx_snprintf(job_res.kind, sizeof(job_res.kind), "%s", JA_PROTO_VALUE_IPCHANGE);

        json_data = read_IP_json_data(serverIPs_file);
        if (json_data != NULL)
        {
            json_server_ip = check_serverID(job->serverid, json_data);

            if (json_server_ip == NULL)
            {
                job_res.return_code = FAIL;
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Requsted serverId doesn't exist.", __function_name);
            }
            else if (strcmp(json_server_ip, socket_ip) != 0)
            {
                json_object_object_add(json_data, job->serverid, json_object_new_string(socket_ip));
                write_IP_json_data(json_data, serverIPs_file);
                job_res.return_code = SUCCEED;
                zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Server IP is changed to %s in serverIPs.json file.", __function_name, socket_ip);
            }
            else if (strcmp(json_server_ip, socket_ip) == 0)
            {
                zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Server IP is the same to %s in serverIPs.json file.", __function_name, socket_ip);
                job_res.return_code = SUCCEED;
            }

            int temp_job_version = job->version;
            job->version = 0;
            check_serverID_protoVersion_from_serverIDs(job, json_data, socket_ip); // check new serverids format
            job->version = temp_job_version;                                       // restore version number
        }
        else
        {
            zabbix_log(LOG_LEVEL_ERR, "In %s(), serverIPs.json file is not readable and recovering it.", __function_name);
            // file is not readable state
            json_data = json_object_new_object(); // create new json object
            json_object_object_add(json_data, job->serverid, json_object_new_string(socket_ip));

            // serverids:
            json_object *serverIDs_json_array = json_object_new_array();
            json_object *serverID_json_object = json_object_new_object();

            json_object_object_add(serverID_json_object, JA_PROTO_TAG_SERVERID, json_object_new_string(job->serverid));
            json_object_object_add(serverID_json_object, JA_PROTO_TAG_SERVERIP, json_object_new_string(socket_ip));
            json_object_object_add(serverID_json_object, JA_PROTO_TAG_VERSION, json_object_new_int(job->version));

            json_object_array_add(serverIDs_json_array, serverID_json_object);
            json_object_object_add(json_data, JA_PROTO_TAG_SERVERIDS, serverIDs_json_array);

            write_IP_json_data(json_data, serverIPs_file); // overide old file with new obeject
            job_res.return_code = SUCCEED;
        }
        ret = ja_tcp_send_to(s, &job_res, CONFIG_TIMEOUT);

        json_object_put(json_data);
#ifdef _WINDOWS
        zbx_free(job);
#endif
    }
    else
    {
        zbx_snprintf(job->kind, sizeof(job->kind), "%s",
                     JA_PROTO_VALUE_JOBRUN_RES);
        job->version = JA_PROTO_TELE_VERSION;
        job->result = JA_RESPONSE_FAIL;
        ret = ja_tcp_send_to(s, job, CONFIG_TIMEOUT);
        zbx_tcp_unaccept(s);
#ifdef _WINDOWS
        zbx_free(job);
#endif
    }
    return ret;
}

/******************************************************************************
 *                                                                            *
 * Function:                                                                  *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
#ifdef _WINDOWS
void process_listener_thread(LPVOID lpParam)
{
    zabbix_log(LOG_LEVEL_DEBUG, "Listener thread process start.");
    zbx_sock_t *sock;
    ja_job_object *job;
    ja_job_object *temp_job;
    zabbix_log(LOG_LEVEL_DEBUG, "Parameters assigned.");
    sock = malloc(sizeof(zbx_sock_t));
    memcpy(sock, (zbx_sock_t *)lpParam, sizeof(zbx_sock_t));
    job = (ja_job_object *)zbx_malloc(NULL, sizeof(ja_job_object));
    zabbix_log(LOG_LEVEL_DEBUG, "Calling process listener..");
    process_listener(sock, job);
    zabbix_log(LOG_LEVEL_DEBUG, "Listener thread process end.");
    zbx_free(sock);
    ExitThread(0);
}
#else
void catch_listen_sig_term()
{
    int status;
    if (listen_mthread_pid > 0)
    {
        kill(listen_mthread_pid, SIGTERM);
        waitpid(listen_mthread_pid, &status, WUNTRACED);
    }
    exit(0);
}
#endif
ZBX_THREAD_ENTRY(listener_thread, args)
{
    int ret, local_request_failed = 0, write_log = 1, thread_number = 0;
    zbx_sock_t sock;
    struct tm *tm;
    time_t now;
    ja_job_object *job;
    // thread check process
    int status;

#ifdef _WINDOWS
    zbx_sock_t tmp_sock;
#else
    while (listen_mthread_pid = fork())
    {
        signal(SIGTERM, catch_listen_sig_term);
        if (listen_mthread_pid < 0)
        {
            exit(-1);
        }
        if (listen_mthread_pid > 0)
        {
            zabbix_log(LOG_LEVEL_DEBUG, "Listen process execution thread : %d", listen_mthread_pid);
        }
        wait(&status);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) != 10)
            {
                exit(WEXITSTATUS(status));
            }
            else
            {
                zabbix_log(LOG_LEVEL_INFORMATION, "[Listener] Timeout error occurred. Restart.");
            }
        }
        if (WIFSIGNALED(status))
        {
            kill(getpid(), WTERMSIG(status));
        }
        sleep(1);
    }
#endif

    assert(args);
    assert(((zbx_thread_args_t *)args)->args);

    zabbix_log(LOG_LEVEL_INFORMATION,
               "jobarg_agentd #%d started [listener]",
               ((zbx_thread_args_t *)args)->thread_num);
    thread_number = ((zbx_thread_args_t *)args)->thread_num;
    memcpy(&sock, (zbx_sock_t *)((zbx_thread_args_t *)args)->args,
           sizeof(zbx_sock_t));
    zbx_free(args);
    ja_job_clean_temp();
#ifdef _WINDOWS
    HANDLE hThread;
    DWORD threadId;
#else
    job = (ja_job_object *)zbx_malloc(NULL, sizeof(ja_job_object));
    signal(SIGUSR1, alarm_handler);
#endif
    int count = 0;
    while (ZBX_IS_RUNNING())
    {
        // Harvesting process.
#ifndef _WINDOWS
        while (1)
        {
            // Check if any child process has terminated
            pid_t child_pid = waitpid(-1, NULL, WNOHANG);

            if (child_pid <= 0)
            {
                // No child process has terminated, break the loop
                break;
            }
            else
            {
                // A child process has terminated
                zabbix_log(LOG_LEVEL_DEBUG, "child process terminated.PID :%d ", child_pid);
            }
        }
#endif
        zbx_setproctitle("listener [waiting for connection]");
        time(&now);
        tm = localtime(&now);
        if (0 == tm->tm_hour || 4 == tm->tm_hour || 8 == tm->tm_hour || 12 == tm->tm_hour || 16 == tm->tm_hour || 20 == tm->tm_hour)
        {
            if (write_log == 0)
            {
                zabbix_log(LOG_LEVEL_INFORMATION, "jobarg_agentd[%s (revision %s)] #%d running [listener]", JOBARG_VERSION, JOBARG_REVISION, thread_number);
                write_log = 1;
            }
        }
        else
        {
            write_log = 0;
        }
        count++;
        if (SUCCEED == (ret = ja_tcp_accept_listener(&sock)) && sock.socket != ZBX_SOCK_ERROR)
        {
            local_request_failed = 0;
            zbx_setproctitle("listener [processing request]");
#ifdef _WINDOWS
            // Create a thread
            zabbix_log(LOG_LEVEL_DEBUG, "Listener : listen accepted.");
            zbx_sock_t *to_sock;
            to_sock = malloc(sizeof(zbx_sock_t));
            memcpy(to_sock, &sock, sizeof(zbx_sock_t));
            hThread = CreateThread(NULL, 0, process_listener_thread, to_sock, 0, &threadId);

            if (hThread == NULL)
            {
                zabbix_log(LOG_LEVEL_ERR, "Failed to create thread. Error code: %lu\n", GetLastError());
                zbx_free(to_sock);
                return 1;
            }

            // Wait for the thread to finish (in this case, it never finishes)
            DWORD waitResult = WaitForSingleObject(hThread, CONFIG_JA_LISTEN_LOOP_TIMEOUT * 1000);
            if (waitResult == WAIT_OBJECT_0)
            {
                DWORD threadExitCode;
                GetExitCodeThread(hThread, &threadExitCode); // Get the exit code of the thread

                // Check the exit code against your error codes
                switch (threadExitCode)
                {
                case 0:
                    zabbix_log(LOG_LEVEL_DEBUG, "Listener thread finished successfully.\n");
                    ret = SUCCEED;
                    zbx_free(to_sock);
                    break;
                default:
                    zabbix_log(LOG_LEVEL_ERR, "Exiting jobarg_agentd.Listener error occurred. exit code : %d\n", threadExitCode);
                    ret = FAIL;
                    break;
                }
            }
            else if (waitResult == WAIT_FAILED)
            {
                zabbix_log(LOG_LEVEL_ERR, "Listener execution  thread failed with error code :[%s]", GetLastError());
                // Handle the error here
                ret = FAIL;
            }
            else if (waitResult == WAIT_TIMEOUT)
            {
                zabbix_log(LOG_LEVEL_ERR, "Listener execution timed out.");
                TerminateThread(hThread, 0);
                ret = FAIL;
            }
            // Close the thread handle
            CloseHandle(hThread);
#else
            jaalarm(CONFIG_JA_LISTEN_LOOP_TIMEOUT);
            ret = process_listener(&sock, job);
            jaalarm(0);
#endif
        }
        if (SUCCEED == ret)
        {
            zabbix_log(LOG_LEVEL_DEBUG, "Listener thread successfully finished.");
            continue;
        }

        if(RECEIVED_NULL == ret){
            zabbix_log(LOG_LEVEL_DEBUG, "Listener thread received NULL data.");
            continue;
        }

        zabbix_log(LOG_LEVEL_DEBUG, "Listener error: %s",
                   zbx_tcp_strerror());

        if (local_request_failed++ > 1000)
        {
            zabbix_log(LOG_LEVEL_WARNING,
                       "Too many consecutive errors on accept() call.");
            local_request_failed = 0;
        }
    }

    zabbix_log(LOG_LEVEL_INFORMATION, "jobarg_agentd listener stopped");
    zbx_free(job);
    ZBX_DO_EXIT();
    zbx_thread_exit(0);
}
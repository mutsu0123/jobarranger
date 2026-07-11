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
#include "log.h"
#include <json-c/json.h>
#include "jacommon.h"
#ifdef _WINDOWS
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#endif

extern char *CONFIG_TMPDIR;
extern char *CONFIG_LOG_FILE;
#ifdef _WINDOWS
void get_nano_time(char* nano_time)
{
    int		gmtoff, ms;
    struct _timeb	tv;
    struct tm* tm;
    _ftime(&tv);
    tm = localtime(&tv.time);
    ms = tv.millitm;
   

    // Format the timestamp as YYYYMMDDHHMMSSNNNNNNNNN
    zbx_snprintf(nano_time, 20, "%.4d%.2d%.2d%.2d%.2d%.2d%.4d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, ms);
}
#else
void get_nano_time(char *nano_time)
{
	struct tm *tm;
	time_t now = time(NULL);
	tm = localtime(&now);

	struct timeval time_now;
	gettimeofday(&time_now, NULL);
	time_t time_str_tm = gmtime(&time_now.tv_sec);

	zbx_snprintf(nano_time, 20, "%.4d%.2d%.2d%.2d%.2d%.2d%.4d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, time_now.tv_usec);
}
#endif

void change_ip_format(char *ip_to_change){
	int i = 0;
	while(i < strlen(ip_to_change)){
		if(ip_to_change[i] == '.'){
			ip_to_change[i] = '-';
		}
		i++;
	}
}
void filePath_for_tmpjob_agent(const char*unique_id, char * jobfile_path){
    const char *__function_name = "filePath_for_tmpjob_agent";
    zbx_snprintf(jobfile_path, JA_FILE_PATH_LEN, "%s%cjobs%c%s.job", CONFIG_TMPDIR, JA_DLM, JA_DLM, unique_id);
    /*
    const char* lastSlash = strrchr(CONFIG_TMPDIR, JA_DLM);

    char var_file_name[JA_FILE_NAME_LEN];
    zbx_snprintf(var_file_name,sizeof(var_file_name), "%ctmp%cjobs%c%s.job",JA_DLM,JA_DLM,JA_DLM, unique_id);
    
    if (lastSlash != NULL) {
        size_t directoryLength = lastSlash - CONFIG_TMPDIR;
        zbx_strlcpy(jobfile_path, CONFIG_TMPDIR, directoryLength+1);
        strcat(jobfile_path, var_file_name);
        jobfile_path[directoryLength+ strlen(var_file_name)] = '\0';
    }
    */
}
void filePath_for_tmpjob_server(const char*unique_id, char * jobfile_path){
    const char *__function_name = "filePath_for_tmpjob_server";

    const char* lastSlash = strrchr(CONFIG_LOG_FILE, '/');

    char var_file_name[JA_FILE_NAME_LEN];
    zbx_snprintf(var_file_name,sizeof(var_file_name), "/job/%s.job", unique_id);
    
    if (lastSlash != NULL) {
        size_t directoryLength = lastSlash - CONFIG_LOG_FILE;
        zbx_strlcpy(jobfile_path, CONFIG_LOG_FILE, directoryLength+1);
        strcat(jobfile_path, var_file_name);
        jobfile_path[directoryLength+ strlen(var_file_name)] = '\0';
    }
}

int add_uid_to_job_file(char *jobfile_path,char *unique_id)
{
    const char *__function_name = "add_uid_to_job_file";
    zabbix_log(LOG_LEVEL_DEBUG, "In %s() adding uid to file. path: %s. uid [%s]", __function_name,jobfile_path,unique_id);
    FILE *f = fopen(jobfile_path, "a+");
    if (f != NULL)
    {
#ifdef _WINDOWS
        _lock_file(f);
#else
        flockfile(f);
#endif
        fprintf(f,"%s\n",unique_id); 
#ifdef _WINDOWS
            _unlock_file(f);
#else
        funlockfile(f);
#endif
        fclose(f);
        
        return SUCCEED;
    }
    else
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s() Error adding uid[%s] to file %s. Error : %s", __function_name,unique_id, jobfile_path,strerror(errno));
        return FAIL;
    }
}
int read_lastLine_from_file(char *jobfile_path,char* previous){
    const char *__function_name = "read_lastLine_from_file";
    zabbix_log(LOG_LEVEL_DEBUG, "In %s() Reading file. flg: %s", __function_name, jobfile_path);

    FILE *file;
    file = fopen(jobfile_path, "r");

    if (file == NULL) {
        zbx_snprintf(previous, JA_MAX_STRING_LEN, "new");
        zabbix_log(LOG_LEVEL_DEBUG, "In %s() No file found", __function_name);
        return FAIL;
    }

    char lastLine[50];
    char currentLine[50];
    lastLine[0] = '\0';
    currentLine[0] = '\0';
    while (fgets(currentLine, sizeof(currentLine), file) != NULL) {
        // Copy the current line to lastLine
        zbx_snprintf(lastLine, JA_MAX_STRING_LEN, "%s", currentLine);
    }

    if (lastLine[0] != '\0') {
        lastLine[strlen(lastLine)-1] = '\0';
        zabbix_log(LOG_LEVEL_DEBUG, "Last line from [%s] is [%s]",jobfile_path,lastLine);
        zbx_snprintf(previous, JA_MAX_STRING_LEN, "%s", lastLine);
    }

    fclose(file);
    return SUCCEED;
}

void extract_id(char *id, char *unique_id)
{
	int i = 0;
	while (i < strlen(unique_id))
	{
		if (unique_id[i] == '_')
		{
			break;
		}
		id[i] = unique_id[i];
		i++;
	}
	id[i] = '\0';
}

void get_cur_pre_filenames(char *unique_id, char *current, char *previous)
{
    const char *__function_name = "get_cur_pre_filenames";
    DIR *dirp;
    struct dirent *entry;
    char id[10];
    char idFromUid[10];
    char jobfile_path[JA_FILE_PATH_LEN];
    zabbix_log(LOG_LEVEL_DEBUG, "In %s() reading[%s]", __function_name,unique_id);

    extract_id(id, unique_id);

    const char* lastSlash = strrchr(CONFIG_TMPDIR, '/');

    char var_file_name[JA_FILE_NAME_LEN];
    zbx_snprintf(var_file_name,sizeof(var_file_name), "/tmp/jobs/");
    
    if (lastSlash != NULL) {
        size_t directoryLength = lastSlash - CONFIG_TMPDIR;
        zbx_strlcpy(jobfile_path, CONFIG_TMPDIR, directoryLength+1);
        strcat(jobfile_path, var_file_name);
        jobfile_path[directoryLength+ strlen(var_file_name)] = '\0';
    }

    dirp = opendir(jobfile_path);

    memset(previous, 0, sizeof(previous));
    while ((entry = readdir(dirp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }else{
            extract_id(idFromUid, entry->d_name);

            if (strcmp(id, idFromUid) == 0)
            {
                zbx_strlcpy(current,entry->d_name,strlen(entry->d_name)+1);
                current[strlen(current) - 4] = '\0';

                if (strcmp(current, unique_id) == 0)
                {
                    break;
                }
                // set previous data
                zbx_strlcpy(previous,current,strlen(current)+1);
            }
        }
    }
    closedir(dirp);
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
int ja_file_create(const char *filename, const int size)
{
    FILE *fp;
    const char *__function_name = "ja_file_create";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() filename: %s, size: %d",
               __function_name, filename, size);

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "Can not open the file: %s (%s)",
                   filename, strerror(errno));
        return FAIL;
    }
    if (fseek(fp, size, SEEK_SET) != 0) {
        zabbix_log(LOG_LEVEL_ERR, "Can not seek the file: %s (%s)",
                   filename, strerror(errno));
        fclose(fp);
        return FAIL;
    }
    if (fputc(0, fp) < 0) {
        zabbix_log(LOG_LEVEL_ERR, "Can not fputc the file: %s (%s)",
                   filename, strerror(errno));
        fclose(fp);
        return FAIL;
    }
    fclose(fp);
    return SUCCEED;
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
int ja_file_remove(const char *filename)
{
    int ret;
    const char *__function_name = "ja_file_remove";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() filename: %s", __function_name,
               filename);
    ret = SUCCEED;
    if (remove(filename) != 0) {
        zabbix_log(LOG_LEVEL_ERR, "Can not remove the file: %s (%s)",
                   filename, strerror(errno));
        ret = FAIL;
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
long ja_file_getsize(const char *filename)
{
    long size;
    FILE *fp;
    const char *__function_name = "ja_file_getsize";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() filename: %s", __function_name,
               filename);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);

    return size;
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
int ja_file_load(const char *filename, const int size, void *data)
{
    int ret, err, fsize,read_return;
    FILE *fp;
    const char *__function_name = "ja_file_load";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() filename: %s, size: %d",
               __function_name, filename, size);
    ret = SUCCEED;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        zabbix_log(LOG_LEVEL_ERR,
                   "Can not open the file: %s (%s)", filename,
                   strerror(errno));
        return FAIL;
    }

    err = 1;
    if (size > 0) {
        fseek(fp, 0L, SEEK_SET);
        read_return = fread(data,size, sizeof(char), fp);
        if (read_return == 0) {
            err = 0;
            zabbix_log(LOG_LEVEL_ERR,"In %s() ,fread failed for file: %s, file size is : %d, read return is : %d and error is :%s", __function_name,filename, size, read_return,strerror(errno));
        }
    } else {
        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        if (fsize >= JA_STD_OUT_LEN)
            fsize = JA_STD_OUT_LEN - 1;
        fseek(fp, 0L, SEEK_SET);
        if (fsize != 0) {
            if (fread(data, fsize, sizeof(char), fp) == 0) {
                zabbix_log(LOG_LEVEL_ERR, "In %s() ,fread failed for file: %s, file size is :%d,and error is :%s", __function_name, filename, fsize, strerror(errno));
                err = 0;
            }
        }

    }
    if (err != 1) {
        zabbix_log(LOG_LEVEL_ERR,
                   "Can not read result file: %s (%s)", filename,
                   strerror(errno));
        ret = FAIL;
    }
    fclose(fp);
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
#else
int ja_file_is_regular(const char *path)
{
    struct stat st;

    if (0 == lstat(path, &st) && 0 != S_ISREG(st.st_mode))
        return SUCCEED;

    return FAIL;
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
int ja_folders_prepare(char* folders[], const char *parent_folderpath) {
    const char *__function_name = "ja_folders_prepare";
    int tmp_cnt = 0;
    int folder_create_status;
    DIR* dir;
    FILE* fp;
    char tmp_file[JA_FILE_PATH_LEN];
    char folderpath[JA_FILE_PATH_LEN];

    while (folders[tmp_cnt] != NULL)
    {
        zbx_snprintf(folderpath, sizeof(folderpath), "%s%c%s", parent_folderpath, JA_DLM, folders[tmp_cnt]);
        folder_create_status = mkdir(folderpath,JA_PERMISSION);
        chmod(folderpath, JA_PERMISSION);

        if (folder_create_status != 0) {
            if (errno == EEXIST) {
                zabbix_log(LOG_LEVEL_DEBUG, "Directory already exist in %s", folderpath);
            }
            else {
                zabbix_log(LOG_LEVEL_ERR, "Directory cannot be created[%s].\nExiting jobarg_agentd (%s)", folderpath, strerror(errno));
                return FAIL;
            }
        }
        //check for read/write permission
        //check for read access.
        dir = opendir(folderpath);
        if (dir == NULL) {
            zabbix_log(LOG_LEVEL_ERR, "Directory cannot be opened.[%s].\n Exiting jobarg_agentd (%s)", folderpath, strerror(errno));
            return FAIL;
        }
        closedir(dir);

        //check for write access
        zbx_snprintf(tmp_file, sizeof(tmp_file), "%s%ctmp.txt", folderpath, JA_DLM);

        fp = fopen(tmp_file, "wb");
        if (fp == NULL) {
            zabbix_log(LOG_LEVEL_ERR, "Cannot create file under directory[%s].\nExiting jobarg_agentd (%s)", folderpath, strerror(errno));
            return FAIL;
        }
        fclose(fp);
        if (remove(tmp_file) != 0) {
            zabbix_log(LOG_LEVEL_ERR, "Cannot delete file under directory[%s].\nExiting jobarg_agentd (%s)", folderpath, strerror(errno));
            return FAIL;
        }
        
        tmp_cnt++;
    }
    return SUCCEED;
}

/******************************************************************************
 *                                                                            *
 * Function: ja_folder_clean                                                  *
 *                                                                            *
 * Purpose: To clean everthing under the given folderpath                     *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
int ja_folder_clean(const char *folderpath) {
    const char *__function_name = "ja_folder_clean";
    DIR* dir;
    struct dirent *entry;
    char entrypath[JA_FILE_PATH_LEN];
    struct stat info;

    zabbix_log(LOG_LEVEL_DEBUG, "%s() is called, folderpath: %s.", __function_name, folderpath);
    dir = opendir(folderpath);
    if (dir == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Directory cannot be opened.[%s].\n Exiting jobarg_agentd (%s)", __function_name,folderpath, strerror(errno));
        return FAIL;
    }

    while((entry = readdir(dir)) != NULL) {
        //ignore for current folder and parent folder
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        zbx_snprintf(entrypath, sizeof(entrypath), "%s/%s", folderpath, entry->d_name);

        if(stat(entrypath, &info) != 0) {
            zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not get information from: %s (%s)",
                   __function_name, entrypath, strerror(errno));
            return FAIL;
        }

        if(S_ISDIR(info.st_mode)) {
            if(ja_folder_clean(entrypath) != SUCCEED) {
                return FAIL;
            }

            if (rmdir(entrypath) != SUCCEED) {
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not remove the folder: %s (%s)",
                   __function_name, entrypath, strerror(errno));
                return FAIL;
            }
        } else {
            if(remove(entrypath) != SUCCEED) {
                zabbix_log(LOG_LEVEL_ERR, "In %s(), Can not remove the file: %s (%s)",
                    __function_name, entrypath, strerror(errno));
                return FAIL;
            }
        }
    }

    return SUCCEED;
}

int ja_file_clean(const char *filename) {
    const char *__function_name = "ja_file_clean";

    int fileDescriptor = open(filename, O_RDWR);
    if (fileDescriptor == -1) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), File cannot be opened.[%s]. Error:(%s)", __function_name, filename, strerror(errno));
        return FAIL;
    }

    // Set the file size to zero
    if (ftruncate(fileDescriptor, 0) == -1) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Error truncating file.[%s]. Error:(%s)", __function_name, filename, strerror(errno));
        close(fileDescriptor);
        return FAIL;
    }

    close(fileDescriptor);
    return SUCCEED; 
}

int ja_file_create_a(const char *filename) {
    const char *__function_name = "ja_file_create_a";
    FILE* fp;

    fp = fopen(filename, "a");
    if (fp == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "Cannot create file: [%s].\nExiting jobarg_agentd (%s)", filename, strerror(errno));
        return FAIL;
    }
    fclose(fp);
    return SUCCEED;
}

int ja_lock_file(int file_descriptor) {
    const char *__function_name = "ja_lock_file";
    
    return flock(file_descriptor, LOCK_EX|LOCK_NB);
}

int ja_unlock_file(int file_descriptor) {
    const char *__function_name = "ja_unlock_file";
    
    return flock(file_descriptor, LOCK_UN);
}

int write_jsonObj_to_file_detail(json_object *json_array, char *data_file){
    const char* __function_name = "write_jsonObj_to_file_detail";
    FILE* fp;
    char *data = NULL;
    int ret = SUCCEED, file_size, write_count;
    char folderpath[JA_FILE_PATH_LEN];
    data = zbx_strdup(data, (char*)json_object_to_json_string(json_array));

    if (data == NULL) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(),Parsing to json string failed.[%s] ", __function_name, data_file);
        ret = FAIL;
        goto error;
    }

    zabbix_log(LOG_LEVEL_DEBUG, "In %s(),json data :[%s] to be written in [%s] ", __function_name, data, data_file);
    fp = fopen(data_file, "w");
    if (fp == NULL)
    {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(),failed to open data file: [%s] (%s)", __function_name, data_file, strerror(errno));
        ret =  FAIL;
        goto error;
    }
    file_size = strlen(data);
    if (0 >= file_size) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), [data] is empty.", __function_name);
        fclose(fp);
        ret = FAIL;
        goto error;
    }
    fseek(fp, 0L, SEEK_SET);
    write_count = fwrite(data, file_size, 1, fp);
    if (write_count != 1) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), file write failed. data : [%s], to file : %s", __function_name, data,data_file);
        ret = FAIL;
        goto error;
    }
    fclose(fp);

error:
    zbx_free(data);
    return ret;
}

int ja_write_jsonObj_to_file(json_object *json_data, char *data_file) {
    const char* __function_name = "ja_write_jsonObj_to_file";

    if(write_jsonObj_to_file_detail(json_data, data_file) == SUCCEED) {
        zabbix_log(LOG_LEVEL_DEBUG, "In %s(), Data updated to %s", __function_name, data_file);
        return SUCCEED;
    } else {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), Failed to write data to %s", __function_name, data_file);
        return FAIL;
    }
}

int ja_check_file_folder_exist(const char *entry) {

    if(access(entry, F_OK) != SUCCEED) {
        return FAIL;
    }
    return SUCCEED;
}
#endif
 

int ja_lock_resource(const char *lock_file_path, int lock_mode, char * caller_function)
{
    const char *__function_name = "ja_lock_resource";
    
#ifdef _WINDOWS
    HANDLE hFile;
    OVERLAPPED overlapped = {0};

    DWORD flags = 0;    
    if (lock_mode == JA_LOCK_EX) {
        flags = LOCKFILE_EXCLUSIVE_LOCK;
    } else if (lock_mode == (JA_LOCK_EX | JA_LOCK_NB)) {
        flags = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
    } else if (lock_mode == (JA_LOCK_SH | JA_LOCK_NB)) {
        flags = 0 | LOCKFILE_FAIL_IMMEDIATELY;
    }

    while (1)
    {
        hFile = CreateFileA(lock_file_path,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            zabbix_log(LOG_LEVEL_ERR, "In %s(), Unable to open file: %s. Error: %lu, caller_function: %s",
                       __function_name, lock_file_path, GetLastError(), caller_function);
            Sleep(5000);
            continue;
        }


        if (LockFileEx(hFile, flags, 0, MAXDWORD, MAXDWORD, &overlapped))
        {
            zabbix_log(LOG_LEVEL_DEBUG, "In %s(), Locked mode:[%d], Locked file: %s, caller_function: %s", __function_name, lock_mode, lock_file_path, caller_function);
            return (int)hFile; // Cast HANDLE to int for uniformity
        }

        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Waiting to lock file: %s, caller_function: %s", __function_name, lock_file_path, caller_function);

        CloseHandle(hFile);
        Sleep(5000);
    }
#else
    int file_descriptor;
    while (1)
    {
        file_descriptor = open(lock_file_path, O_RDWR | O_CREAT, 0600);
        if (file_descriptor == -1)
        {
            zabbix_log(LOG_LEVEL_ERR, "In %s(), Unable to open lock file: %s. Error: %s, caller_function: %s",
                       __function_name, lock_file_path, strerror(errno), caller_function);
            sleep(5);
            continue;
        }
        if (flock(file_descriptor, lock_mode) == 0)
        {
            zabbix_log(LOG_LEVEL_DEBUG, "In %s(), Locked mode:[%d], Locked file: %s, caller_function: %s", __function_name, lock_mode, lock_file_path, caller_function);
            return file_descriptor;
        }

        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Waiting to lock file: %s, caller_function: %s", __function_name, lock_file_path, caller_function);

        close(file_descriptor);
        sleep(5);
    }
#endif
    return -1;
}

void ja_unlock_resource(int file_descriptor)
{
    const char *__function_name = "ja_unlock_resource";

#ifdef _WINDOWS
    HANDLE hFile = (HANDLE)file_descriptor;

    OVERLAPPED overlapped = {0};

    if (UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped))
    {
        zabbix_log(LOG_LEVEL_DEBUG, "In %s(), Resource unlocked successfully.", __function_name);
    }
    else
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Failed to unlock resource. Error: %lu", __function_name, GetLastError());
    }

    CloseHandle(hFile);
#else
    if (flock(file_descriptor, LOCK_UN) == 0)
    {
        zabbix_log(LOG_LEVEL_DEBUG, "In %s(), Resource unlocked successfully.", __function_name);
    }
    else
    {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Failed to unlock resource. Error: %s", __function_name, strerror(errno));
    }
    
    close(file_descriptor);
#endif
}

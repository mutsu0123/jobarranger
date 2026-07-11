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

int ja_is_valid_time(const char *time_str) {
    int hour, min;

    // Split the input string by ':'
    if (sscanf(time_str , "%d:%d" , &hour,&min) != 2)    return 0;
    if(hour >= 24)   return 0;
    if(min >= 60)    return 0;

    return 1;
}

// check range format: 00:00-00:00
int ja_is_valid_time_range_format(const char* time_str) {
    char time1[6], time2[6];
    char delimiter;

    // Split the input string by '-'
    if (sscanf(time_str, "%5[^-]-%5s", time1, time2) == 2) {
        return ja_is_valid_time(time1) && ja_is_valid_time(time2);
    }
    return 0;
}

// only accepts time as total second and returns if current time is in the range
int ja_is_time_in_range(int total_start_second, int total_end_second) {
    int current_hour, current_minute, current_second;
    struct tm *current_time;
    time_t raw_time;

    time(&raw_time);
    current_time = localtime(&raw_time); 
    current_hour = current_time->tm_hour;
    current_minute = current_time->tm_min;
    current_second = current_time->tm_sec;

    current_minute += current_hour * 60;
    current_second += current_minute * 60;

    return (current_second >= total_start_second && current_second < total_end_second);
}

int ja_is_japurge_time(const char* time_str) {
    const char	*__function_name = "ja_is_japurge_time";
    int start_hour, start_minute, start_second, end_hour, end_minute, end_second;
    struct tm *current_time;
    time_t raw_time;

    // Check CONFIG_JAPURGE_TIME, if it is not valid time, returned true in order to let japurge delete whenever.
    if (!ja_is_valid_time(time_str)) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), JaPurgeTime format from jobarg_server.conf is not right. CONFIG_JAPURGE_TIME: %s", __function_name, time_str);
        return 1;
    }
    // Check CONFIG_JAPURGE_TIME, if it is not valid time, returned true in order to let japurge delete whenever.
    if (sscanf(time_str, "%d:%d-%d:%d", &start_hour, &start_minute, &end_hour, &end_minute) != 4) {
        zabbix_log(LOG_LEVEL_WARNING, "In %s(), JaPurgeTime format from jobarg_server.conf is not right. CONFIG_JAPURGE_TIME: %s", __function_name, time_str);
        return 1;
    }
    // check default value and let it delete.
    if (start_hour == 00 && start_minute == 00 && end_hour == 00 && end_minute == 00) {
        return 1;
    }

    start_minute += start_hour * 60;
    start_second += start_minute * 60;
    end_minute += end_hour * 60;
    end_second += end_minute * 60;

    return ja_is_time_in_range(start_second, end_second);
}
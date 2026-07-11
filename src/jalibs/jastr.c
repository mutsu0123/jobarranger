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
#include "md5.h"
#include "log.h"

#include "jacommon.h"
#include "jastr.h"

#ifdef _WINDOWS
#include "gnuregex.h"
#include <windows.h>
#else
#include <locale.h>
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
char *ja_timestamp2str(const time_t time)
{
    static char buffer[15];
    struct tm *tmbuf;

    tmbuf = localtime(&time);
    zbx_snprintf(buffer, sizeof(buffer), "%.4d%.2d%.2d%.2d%.2d%.2d",
                 tmbuf->tm_year + 1900, tmbuf->tm_mon + 1, tmbuf->tm_mday,
                 tmbuf->tm_hour, tmbuf->tm_min, tmbuf->tm_sec);
    return buffer;
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
time_t ja_str2timestamp(const char *str)
{
    struct tm tmbuf;
    int year, mon, mday, hour, min, sec, maxday;
    static int day[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    char t_str[15], buf[5];
    const char *__function_name = "ja_str2timestamp";

    switch (strlen(str)) {
    case 1:
        return 0;
        break;
    case 8:
        zbx_snprintf(t_str, sizeof(t_str), "%s000000", str);
        break;
    case 10:
        zbx_snprintf(t_str, sizeof(t_str), "%s0000", str);
        break;
    case 12:
        zbx_snprintf(t_str, sizeof(t_str), "%s00", str);
        break;
    case 14:
        zbx_snprintf(t_str, sizeof(t_str), "%s", str);
        break;
    default:
        goto error;
        break;
    }

    zbx_snprintf(buf, 5, "%s", t_str);
    year = atoi(buf);
    zbx_snprintf(buf, 3, "%s", t_str + 4);
    mon = atoi(buf);
    zbx_snprintf(buf, 3, "%s", t_str + 6);
    mday = atoi(buf);
    zbx_snprintf(buf, 3, "%s", t_str + 8);
    hour = atoi(buf);
    zbx_snprintf(buf, 3, "%s", t_str + 10);
    min = atoi(buf);
    zbx_snprintf(buf, 3, "%s", t_str + 12);
    sec = atoi(buf);

    maxday = day[mon - 1] + (mon == 2 && year % 4 == 0
                             && (year % 100 != 0 || year % 400 == 0));
    if (year < 1900 || mon < 1 || mon > 12)
        goto error;
    if (mday < 1 || mday > maxday)
        goto error;
    if (hour < 0 || hour > 23)
        goto error;
    if (min < 0 || min > 59)
        goto error;
    if (sec < 0 || sec > 59)
        goto error;
    tmbuf.tm_year = year - 1900;
    tmbuf.tm_mon = mon - 1;
    tmbuf.tm_mday = mday;
    tmbuf.tm_hour = hour;
    tmbuf.tm_min = min;
    tmbuf.tm_sec = sec;
    tmbuf.tm_isdst = -1;
    return mktime(&tmbuf);

  error:
    zabbix_log(LOG_LEVEL_WARNING, "In %s() unknown the time format: %s",
               __function_name, str);
    return 0;
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
int ja_format_timestamp(const char *input, char *output)
{
    int i, len;
    char *p, *q;

    len = strlen(input);
    if (len != 8 && len != 10 && len != 12 && len != 14 && len != 17)
        return FAIL;

    p = (char *) input;
    q = output;
    for (i = 0; i < len; i++) {
        if (i == 4 || i == 6)
            *q++ = '/';
        if (i == 8)
            *q++ = ' ';
        if (i == 10 || i == 12)
            *q++ = ':';
        if (i == 14)
            *q++ = '.';
        *q++ = *p++;
    }
    *q = '\0';
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
int ja_match(const char *string, const char *pattern)
{
    switch (*pattern) {
    case '\0':
        return '\0' == *string;
    case '*':
        return ja_match(string, pattern + 1)
            || (('\0' != *string) && ja_match(string + 1, pattern));
    case '?':
        return ('\0' != *string)
            && ja_match(string + 1, pattern + 1);
    default:
        return ((unsigned char) *pattern == (unsigned char) *string)
            && ja_match(string + 1, pattern + 1);
    }
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
int ja_regexp(const char *string, const char *pattern)
{
    int ret;
    regex_t re;
    regmatch_t match;
    const char *__function_name = "ja_regexp";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() string: %s, pattern: %s",
               __function_name, string, pattern);
    if (string == NULL || pattern == NULL)
        return 1;

    if (regcomp(&re, pattern, REG_EXTENDED | REG_NEWLINE) != 0)
        return 0;

    if (0 == regexec(&re, string, (size_t) 1, &match, 0))
        ret = 1;
    else
        ret = 0;

    regfree(&re);
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
char * ja_fcopy_regex_pattern_format(char * string){
    size_t size = 0;
    char *s1,*s2,*s3,*s4,*s5;
    char * result_string;
    const char *__function_name = "ja_fcopy_regex_pattern_format";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() string: %s", __function_name, string);
    if (string == NULL)
        return 1;

    s1 = string_replace(string, "\\d", "[0-9]");
    s2 = string_replace(s1, "\\D", "[^0-9]");
    s3 = string_replace(s2, "\\w", "[A-Za-z0-9_]");
    s4 = string_replace(s3, "\\W", "[^A-Za-z0-9_]");
    s5 = string_replace(s4, "\\s", "\\");

    size = strlen(s5);

    result_string = (char*)zbx_malloc(NULL,size+1);
    zbx_snprintf(result_string, size+1, "%s", s5);

    zbx_free(s1);
    zbx_free(s2);
    zbx_free(s3);
    zbx_free(s4);
    zbx_free(s5);
    return result_string;
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
int ja_is_number(const char *str)
{
    if (NULL ==
        zbx_regexp_match(str, "^[-]{0,1}[0-9]+[.]{0,1}[0-9]*$", NULL)) {
        return 0;
    } else {
        return 1;
    }
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
int ja_number_comp(const double num, const char *range)
{
    int ret, flag;
    char *new_range, *p;
    double s, e;
    const char *__function_name = "ja_number_comp";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() num: %f, range: %s",
               __function_name, num, range);

    ret = -1;
    new_range = zbx_strdup(NULL, range);
    p = new_range + 1;
    flag = 0;

    while (*p != '\0') {
        if (*p == '-') {
            *p++ = '\0';
            flag = 1;
            break;
        }
        p++;
    }

    if (ja_is_number(new_range) != 1) {
        zabbix_log(LOG_LEVEL_WARNING, "%s is not a number", new_range);
        goto error;
    } else {
        s = atof(new_range);
    }
    if (flag == 1) {
        if (ja_is_number(p) != 1) {
            zabbix_log(LOG_LEVEL_WARNING, "%s is not a number", p);
            goto error;
        } else {
            e = atof(p);
            if (s > e) {
                zabbix_log(LOG_LEVEL_WARNING, "%f > %f is wrong", s, e);
                goto error;
            }
        }
        if (num >= s && num <= e) {
            ret = 1;
        } else {
            ret = 0;
        }
    } else {
        if (num == s)
            ret = 1;
        else
            ret = 0;
    }

  error:
    zbx_free(new_range);
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
int ja_number_match(const char *string, const char *pattern)
{
    int ret;
    char *str, *new_pattern;
    char *tp;
    double value;
    const char *__function_name = "ja_number_match";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

    if (string == NULL || pattern == NULL) {
        return 0;
    }

    if (strlen(string) == 0 || strlen(pattern) == 0) {
        return 0;
    }

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() string: %s, pattern: %s",
               __function_name, string, pattern);

    str = string_replace(string, " ", "");
    new_pattern = string_replace(pattern, " ", "");
    if (strlen(str) == 0 || strlen(new_pattern) == 0) {
        ret = 0;
        goto exit;
    }

    if (ja_is_number(str) != 1) {
        zabbix_log(LOG_LEVEL_WARNING, "string: %s is not a number", str);
        ret = -1;
        goto exit;
    }

    ret = -1;
    value = atof(str);
    tp = strtok(new_pattern, ",");
    while (tp != NULL) {
        ret = ja_number_comp(value, tp);
        if (ret != 0)
            break;
        tp = strtok(NULL, ",");
    }

  exit:
    zbx_free(str);
    zbx_free(new_pattern);
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
int ja_format_expr(char *input, char *output)
{
    char *p, *q;
    const char *__function_name = "ja_format_expr";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() input: [%s]",
               __function_name, input);

    zbx_snprintf(output, JA_MAX_STRING_LEN, "expr ");
    p = input;
    q = output + strlen(output);
    while (*p != '\0') {
        switch (*p) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '!':
        case '=':
        case '<':
        case '>':
        case '(':
        case ')':
        case '|':
        case '&':
        case ':':
        case ';':
        case '[':
        case ']':
        case '`':
            *q++ = '\\';
            break;
        default:
            break;
        }
        *q++ = *p++;
    }
    *q = '\0';

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() output: [%s]",
               __function_name, output);

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
int ja_format_date(char *input, char *output)
{
    int flag;
    char *p, *q;
    const char *__function_name = "ja_format_date";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() input: [%s]",
               __function_name, input);

    zbx_snprintf(output, JA_MAX_STRING_LEN, "date --date \"");
    p = input;
    q = output + strlen(output);

    flag = 0;
    while (*p != '\0') {
        if (flag == 0 && *p == ';') {
            flag = 1;
            *q++ = '"';
            *q++ = ' ';
            *q++ = '+';
            *q++ = '"';
            p++;
            continue;
        }
        switch (*p) {
        case '"':
        case '`':
        case '\'':
        case '>':
        case '<':
        case '&':
        case '\\':
            p++;
            break;
        default:
            *q++ = *p++;
            break;
        }
    }
    *q++ = '"';
    *q = '\0';

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() output: [%s]",
               __function_name, output);

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
int ja_format_extjob(char *input, char *output)
{
    char *p, *q;
    const char *__function_name = "ja_format_extjob";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() input: [%s]",
               __function_name, input);

    p = input;
    q = output;
    while (*p != '\0') {
        switch (*p) {
        case ';':
        case '`':
        case '>':
        case '<':
        case '&':
        case '|':
            p++;
            break;
        default:
            *q++ = *p++;
            break;
        }
    }
    *q = '\0';

    zabbix_log(LOG_LEVEL_DEBUG, "In %s() output: [%s]",
               __function_name, output);

    return SUCCEED;

}

#ifdef _WINDOWS
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
LPSTR ja_unicode_to_acp(LPCTSTR wide_string)
{
    LPSTR acp_string = NULL;
    int acp_size;

    acp_size =
        WideCharToMultiByte(CP_ACP, 0, wide_string, -1, NULL, 0, NULL,
                            NULL);
    acp_string = (LPSTR) zbx_malloc(acp_string, (size_t) acp_size);

    /* convert from wide_string to acp_string */
    WideCharToMultiByte(CP_ACP, 0, wide_string, -1, acp_string, acp_size,
                        NULL, NULL);

    return acp_string;
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
LPSTR ja_utf8_to_acp(LPCSTR utf8_string)
{
    LPSTR acp_string = NULL;
    wchar_t *wide_string;

    wide_string = zbx_utf8_to_unicode(utf8_string);
    acp_string = ja_unicode_to_acp(wide_string);
    if (wide_string != NULL)
        zbx_free(wide_string);

    return acp_string;
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
LPSTR ja_acp_to_utf8(LPCSTR acp_string)
{
    LPSTR utf8_string = NULL;
    wchar_t *wide_string;

    wide_string = zbx_acp_to_unicode(acp_string);
    utf8_string = zbx_unicode_to_utf8(wide_string);
    if (wide_string != NULL)
        zbx_free(wide_string);

    return utf8_string;
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
char *ja_checksum(const char *filename)
{
    char *hash_text = NULL;
    md5_state_t state;
    md5_byte_t hash[MD5_DIGEST_SIZE];
    u_char buf[16 * ZBX_KIBIBYTE];
    int i, nbytes, f = -1;
    size_t sz;

    if (-1 == (f = zbx_open(filename, O_RDONLY | O_BINARY)))
        goto error;

    md5_init(&state);
    while (0 < (nbytes = (int) read(f, buf, sizeof(buf)))) {
        md5_append(&state, (const md5_byte_t *) buf, nbytes);
    }
    md5_finish(&state, hash);

    if (0 > nbytes)
        goto error;

    sz = MD5_DIGEST_SIZE * 2 + 1;
    hash_text = (char *) zbx_malloc(hash_text, sz);
    for (i = 0; i < MD5_DIGEST_SIZE; i++) {
        zbx_snprintf(&hash_text[i << 1], sz - (i << 1), "%02x", hash[i]);
    }

  error:
    if (-1 != f)
        close(f);

    return hash_text;
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
char *ja_md5(const char *str)
{
    char *hash_text = NULL;
    md5_state_t state;
    md5_byte_t hash[MD5_DIGEST_SIZE];
    int i;
    size_t sz;

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *) str, strlen(str));
    md5_finish(&state, hash);

    sz = MD5_DIGEST_SIZE * 2 + 1;
    hash_text = (char *) zbx_malloc(hash_text, sz);
    for (i = 0; i < MD5_DIGEST_SIZE; i++) {
        zbx_snprintf(&hash_text[i << 1], sz - (i << 1), "%02x", hash[i]);
    }

    return hash_text;
}

/******************************************************************************
 *                                                                            *
 * Function: ja_check_number                                                  *
 *                                                                            *
 * Purpose: check that it is a number                                         *
 *                                                                            *
 * Parameters: data (in) - string to be checked                               *
 *                                                                            *
 * Return value:  SUCCEED - processed successfully                            *
 *                FAIL - detect incorrect data                                *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
int ja_check_number(char *data)
{
    const char *__function_name = "ja_check_number";

    zabbix_log(LOG_LEVEL_DEBUG, "In %s(%s)", __function_name, data);

    while ('\0' != *data) {
        if (0 == isdigit(*data)) {
            return FAIL;
        }
        data++;
    }
    return SUCCEED;
}
#ifdef _WINDOWS
char* ja_utf8_to_locale(const char* utf8_data) {
    const char* __function_name = "ja_utf8_to_locale";
    UINT codePage = GetACP(); // Get ANSI code page
    int utf8Length = strlen(utf8_data);
    char* out_data = NULL;
    // Determine the required buffer size for converted text
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8_data, utf8Length, NULL, 0);
    zabbix_log(LOG_LEVEL_DEBUG, "In %s(), original string :%s, original string length:%d, output string size:%d", __function_name, utf8_data, bufferSize, bufferSize);

    // Allocate memory for converted text
    wchar_t* convertedText = (wchar_t*)malloc((bufferSize + 1) * sizeof(wchar_t));

    // Convert UTF-8 to wide character string (UTF-16)
    MultiByteToWideChar(CP_UTF8, 0, utf8_data, utf8Length, convertedText, bufferSize);
    convertedText[bufferSize] = L'\0'; // Null-terminate the string

    // Convert wide character string to system code page
    bufferSize = WideCharToMultiByte(codePage, 0, convertedText, -1, NULL, 0, NULL, NULL);

    // Allocate memory for the final converted text
    out_data = (char*)malloc(bufferSize+1);
    if (out_data == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot allocate locale_data.error :%s", __function_name, strerror(errno));
        return NULL;
    }

    // Convert wide character string to system code page
    WideCharToMultiByte(codePage, 0, convertedText, -1, out_data, bufferSize, NULL, NULL);

    // Print the final converted text
    zabbix_log(LOG_LEVEL_DEBUG, "In %s(),Original Text :%s,Convered text:%s", __function_name, utf8_data, out_data);

    // Free allocated memory
    free(convertedText);
    return out_data;
}
#else
char* ja_utf8_to_locale(const char *utf8_data) {
    int ret = FAIL;
    char out_encode[16];
    char *outbuf = NULL;
    char *inbuf = NULL;
    size_t outbytesleft = 0, inbytesleft = 0;
    const char* __function_name = "ja_utf8_to_locale";

    // Set locale and get the current encoding
    setlocale(LC_CTYPE, "");
    char *locale = setlocale(LC_CTYPE, NULL);
    if (locale == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), setlocale failed to retrieve locale.", __function_name);
        return NULL;
    }

    // Manually map locale encoding to a standard iconv-compatible encoding name
    if (strcmp(locale, "ja_JP.SJIS") == 0) {
        zbx_snprintf(out_encode, sizeof(out_encode), "SHIFT_JIS");
    } else {
        // Fallback to the locale if no specific mapping is needed
        zbx_snprintf(out_encode, sizeof(out_encode), "%s", locale);
    }

    zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), Locale encoding: %s", __function_name, out_encode);

    // Allocate memory for buffers
    inbytesleft = strlen(utf8_data);
    outbytesleft = inbytesleft * 4;
    outbuf = (char *)malloc(outbytesleft + 1);
    if (outbuf == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot allocate outbuf. Error: %s", __function_name, strerror(errno));
        return NULL;
    }

    inbuf = (char *)malloc(inbytesleft + 1);
    if (inbuf == NULL) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot allocate inbuf. Error: %s", __function_name, strerror(errno));
        zbx_free(outbuf);
        return NULL;
    }
    zbx_strlcpy(inbuf, utf8_data, inbytesleft + 1);

    // Open iconv conversion descriptor
    iconv_t cd = iconv_open(out_encode, "UTF-8");

    if (cd == (iconv_t)-1) {
        zabbix_log(LOG_LEVEL_ERR, "In %s(), Cannot execute iconv_open. Error: %s", __function_name, strerror(errno));
        zbx_free(outbuf);
        zbx_free(inbuf);
        return NULL;
    }

    // Perform conversion
    char *inbuf_ptr = inbuf;
    char *outbuf_ptr = outbuf;
    if (iconv(cd, &inbuf_ptr, &inbytesleft, &outbuf_ptr, &outbytesleft) == (size_t)-1) {
        zabbix_log(LOG_LEVEL_INFORMATION, "In %s(), iconv() error: %s, Cannot execute command.", __function_name, strerror(errno));
        iconv_close(cd);
        zbx_free(outbuf);
        zbx_free(inbuf);
        return NULL;
    }

    *outbuf_ptr = '\0'; // Null-terminate the output buffer

    // Clean up
    iconv_close(cd);
    zbx_free(inbuf);

    return outbuf;
}
#endif
/******************************************************************************
 *                                                                            *
 * Function: ja_replace_invalid_bytes                                         *        
 *                                                                            *
 * Purpose: replace invalid utf8 characters                                   *      
 *                                                                            *
 * Parameters: const char* input - string to be checked                       *        
 *                                                                            *
 * Return value:  relplaced string                                            *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
char* ja_replace_invalid_bytes(const char* input) {
    size_t input_len = strlen(input);
    size_t output_len = input_len * 3; // Maximum possible length
    char* output = (char*)malloc(output_len + 1); 

    const unsigned char* p = (const unsigned char*)input;
    char* out = output;

    while (*p) {
        if (*p == 0xFF) {
            // Replace 0xFF with UTF-8 replacement character � (U+FFFD)
            *out++ = (char)0xEF;
            *out++ = (char)0xBF;
            *out++ = (char)0xBD;
        } else {
            *out++ = *p;
        }
        p++;
    }
    
    *out = '\0'; 
    return output;
}
#include "../../include/common.h"

int is_error(char *json)
{
    (void)json;   /* 未使用引数を黙らせる */

    /* ここでは JSON を解析せず、常に「エラーなし」とみなす */
    return SUCCEED;
}

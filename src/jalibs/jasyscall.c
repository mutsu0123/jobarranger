#include <stdlib.h>
int ja_system_call(const char *cmd)
{
    return system(cmd);
}

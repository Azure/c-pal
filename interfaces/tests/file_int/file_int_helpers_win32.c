//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifdef __cplusplus
#include <cstdlib>
#include <cstdio>
#include <cstring>
#else
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#endif
#include "file_int_helpers.h"

int delete_file(const char* filename)
{
    char* command = malloc(strlen(filename) + 5);
    if (command == NULL)
    {
        return -1;
    }
    (void)sprintf(command, "del %s", filename);
    (void)system(command);
    free(command);
    return 0;
}

bool check_file_exists(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return true;
    }
    return false;
}

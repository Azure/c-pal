//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifdef __cplusplus
#include <cstdlib>
#include <cstdio>
#else
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#endif
#include "file_int_helpers.h"

void delete_all_txt_files()
{
    system("del *.txt");
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

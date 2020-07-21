//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FILE_INT_HELPERS_H
#define FILE_INT_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

int delete_file(const char* filename);
bool check_file_exists(const char* filename);

#ifdef __cplusplus
}
#endif
#endif

//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.



#include <stdlib.h>

#include "file_int_helpers.h"

void delete_all_txt_files(void)
{
    system("rm *.txt");
}

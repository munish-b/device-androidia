/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            HwchUtils.cpp
*
* Description:          Utility function implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Check to see if a process is running on the Android system
// Pass in the binary name is a pattern e.g. "surfaceflinger"
bool processRunning(char *pattern)
{
    regex_t number;
    regex_t name;
    regcomp(&number, "^[0-9]+$", REG_EXTENDED);
    regcomp(&name, pattern, 0);

    if (chdir("/proc") == 0)
    {
        DIR* proc = opendir("/proc");

        // Look for all the directories in /proc
        struct dirent *dp;
        while ((dp = readdir(proc)))
        {

            // Match those which are numerical
            if ((regexec(&number, dp->d_name, 0, 0, 0)) == 0)
            {
                chdir(dp->d_name);
                char buf[4096];
                int fd = open("cmdline", O_RDONLY);
                buf[read(fd, buf, (sizeof buf)-1)] = '\0';
                if (regexec(&name, buf, 0, 0, 0) == 0)
                {
                    return true;
                }
                close(fd);
                chdir("..");
            }
        }
        closedir(proc);
    }

    return false;
}


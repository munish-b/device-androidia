
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#define LOG_NDEBUG 0
//#include <cutils/log.h>

int main(int /*argc*/, char** argv)
{
    char envstr[100];
    //ALOGD("DEBUG- call SF shim\n");
    strcpy(envstr, "LD_PRELOAD=/system/lib64/libdrm.so");
    putenv(envstr);

    execv ("/system/bin/surfaceflinger_real", argv);
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
    bool ftrace = false;
    uint32_t numFtraceLines = 0;

    if (argc > 1)
    {
        if (strcmp(argv[1], "-ftrace") == 0)
        {
            ftrace = true;
            printf("ftrace enabled\n");
        }
    }

    FILE* f = fopen("/dev/kmsg", "r");

    if (f == 0)
    {
        printf("Failed to open kmsg\n");
        exit(1);
    }

    ssize_t read;
    char* line = 0;
    size_t len = 0;

    while (true)
    {

        while ((read = getline(&line, &len, f)) != -1)
        {
            printf("%s", line);

            if (ftrace && (strstr(line, "vblank wait timed out") != 0))
            {
                // To avoid the output file getting too big, stop collecting Ftraces once
                // we have a million lines of FTRACE
                if (numFtraceLines < 1000000)
                {
                    // Cache the ftrace file
                    FILE* ftraceFile = fopen ("/sys/kernel/debug/tracing/trace", "r");

                    // Dump the ftrace inline into our kmsg file
                    if (ftraceFile)
                    {
                        printf("--------------- START OF FTRACE ------------------------\n");
                        while ((read = getline(&line, &len, ftraceFile)) != -1)
                        {
                            printf("FTRACE: %s", line);
                            ++numFtraceLines;
                        }

                        fclose(ftraceFile);
                        printf("--------------- END OF FTRACE ------------------------\n");
                    }
                    else
                    {
                        printf("FTRACE: file /sys/kernel/debug/tracing/trace not found\n");
                    }
                }
            }

        }

        usleep(100);
    }

    free(line);
    exit(0);
}

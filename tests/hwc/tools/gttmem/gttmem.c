#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <pciaccess.h>
#include "gttmem.h"

enum command
{
    Read32,
    Write32,
};

struct program_args
{
    enum command    cmd;
    int terse;
    union
    {
        struct
        {
            unsigned    addr;
            unsigned    offset;
            unsigned    value;
        };
    };
};

struct mem_map_region
{
    void *base_addr;
    int bytes;
    int fd_force_wake;
};

void read32(struct mem_map_region *gttmm, unsigned addr, unsigned offset);
void write32(struct mem_map_region *gttmm, unsigned addr, unsigned offset, unsigned value);
int ParseCommandLine(int argc, char **argv, struct program_args *params);
struct pci_device *GetPciGraphicsDevice(void);
struct mem_map_region *AcquireGTTAddress(struct pci_device *pci_dev);
void ReleaseGTTAddress(struct pci_device *pci_dev, struct mem_map_region *region);

int terse_output = 0;

int main(int argc, char**argv)
{
    struct program_args params;
    struct pci_device *pci_dev;
    struct mem_map_region *gttmm = NULL;
    int error;

    if (ParseCommandLine(argc, argv, &params))
    {
        return 1;
    }

    terse_output = params.terse;

    error = pci_system_init();
    if  (error)
    {
        printf("Failed to initialise PCI system, error %d\n", error);
        return 1;
    }

    pci_dev = GetPciGraphicsDevice();
    if (pci_dev == NULL)
    {
        goto end;
    }

    gttmm = AcquireGTTAddress(pci_dev);
    if (gttmm)
    {
        switch (params.cmd)
        {
        case Read32:
            read32(gttmm, params.addr, params.offset);
            break;

        case Write32:
            write32(gttmm, params.addr, params.offset, params.value);
            break;

        default:
            printf("Command %d ignored\n", params.cmd);
        }

        ReleaseGTTAddress(pci_dev, gttmm);
    }

end:
    pci_system_cleanup();
    return error;
}

void ConvertUnsignedToBinaryString(unsigned value, char binary_string[], int max_chars)
{
    int bit;
    int c = 0;

    --max_chars; // null terminated string

    for (bit = 31; bit >= 0; --bit)
    {
        if (c && ((bit+1) % 4) == 0)
        {
            if(((bit+1) % 8) == 0)
            {
                binary_string[c++] = ' ';
                binary_string[c++] = ' ';
            }
            else
            {
                binary_string[c++] = ':';
            }
        }

        if(c >= max_chars)
        {
            break;
        }

        binary_string[c++] = (value & (1 << bit)) ? '1' : '0';
    }
    binary_string[c] = '\0';
}

void read32(struct mem_map_region *gttmm, unsigned addr, unsigned offset)
{
    void *p = (unsigned char *)gttmm->base_addr + addr + offset;
    char binary_string[64];
    unsigned value;

    if (!terse_output)
    {
        if (offset)
        {
            printf("READ addr(%08X + %08X = %08X):\n", addr, offset, addr + offset);
        }
        else
        {
            printf("READ addr(%08X):\n", addr);
        }
    }

    value = *(volatile unsigned *)p;

    if (!terse_output)
    {
        ConvertUnsignedToBinaryString(value, binary_string, sizeof binary_string);
        printf("\t%08X = %s\n", value, binary_string);
    }
    else
    {
        printf("%08X\n", value);
    }
}

void write32(struct mem_map_region *gttmm, unsigned addr, unsigned offset, unsigned value)
{
    void *p = (unsigned char *)gttmm->base_addr + addr + offset;
    char binary_string[64];

    if (!terse_output)
    {
        ConvertUnsignedToBinaryString(value, binary_string, sizeof binary_string);
        if (offset)
        {
            printf("WRITE addr(%08X + %08X = %08X):\n", addr, offset, addr + offset);
            printf("\t%08X = %s\n", value, binary_string);
        }
        else
        {
            printf("WRITE addr(%08X):\n", addr);
            printf("\t%08X = %s\n", value, binary_string);
        }
    }

    *(volatile unsigned *)p = value;
}

int ParseCommandLine(int argc, char **argv, struct program_args *params)
{
    int show_syntax = (argc == 1); // always show the command syntax if no arguments passed in
    int n = 1;

    memset(params, 0, sizeof *params);

    while (!show_syntax && n < argc)
    {
        if (argv[n][0] != '-')
        {
            show_syntax = 1;
        }
        else if (strcmp(argv[n], "-h") == 0)
        {
            show_syntax = 1;
            break; // break immediately, this is not a syntax error
        }
        else if (strcmp(argv[n], "-r32") == 0)
        {
            params->cmd = Read32;
            if (n+1 > argc)
            {
                show_syntax = 1;
            }
            else
            {
                sscanf(argv[n+1], "%x", &params->addr);
                params->offset = 0;
                n += 2;
            }
        }
        else if (strcmp(argv[n], "-w32") == 0)
        {
            params->cmd = Write32;
            if (n+2 > argc)
            {
                show_syntax = 1;
            }
            else
            {
                sscanf(argv[n+1], "%x", &params->addr);
                sscanf(argv[n+2], "%x", &params->value);
                params->offset = 0;
                n += 3;
            }
        }
        else if (strcmp(argv[n], "-rd") == 0)
        {
            params->cmd = Read32;
            if (n+1 > argc)
            {
                show_syntax = 1;
            }
            else
            {
                sscanf(argv[n+1], "%x", &params->addr);
                params->offset = 0x180000;
                n += 2;
            }
        }
        else if (strcmp(argv[n], "-wd") == 0)
        {
            params->cmd = Write32;
            if (n+2 > argc)
            {
                show_syntax = 1;
            }
            else
            {
                sscanf(argv[n+1], "%x", &params->addr);
                sscanf(argv[n+2], "%x", &params->value);
                params->offset = 0x180000;
                n += 3;
            }
        }
        else if (strcmp(argv[n], "-t") == 0)
        {
            params->terse = 1;
            n += 1;
        }
        else
        {
            show_syntax = 1;
        }

        if (show_syntax)
        {
            printf("Syntax error arg[%d] = '%s': unknown command\n", n, argv[n]);
        }
    }

    if (show_syntax)
    {
        printf("\n");
        printf("Syntax - \n");
        printf("\t%s [<opts>] <cmd>\n", argv[0]);
        printf("\n");
        printf("\t\t<opts>::= <terse>\n");
        printf("\t\t\t<terse>   ::= -t                              # terse output\n");
        printf("\n");
        printf("\t\t<cmd>::= <Read32> | <Write32> |\n");
        printf("\t\t         <ReadDR> | <WriteDR> |\n");
        printf("\t\t         <Help>\n");
        printf("\n");
        printf("\t\t\t<Read32>  ::= -r32 <reg_addr>                 # read 32bits of memory\n");
        printf("\t\t\t<Write32> ::= -w32 <reg_addr> <reg_value>     # write to 32bits of memory\n");
        printf("\t\t\t<ReadDR>  ::= -rd <reg_addr>                  # read display register, <reg_addr> is offset by MMADR [180000H]\n");
        printf("\t\t\t<WriteDR> ::= -wd <reg_addr> <reg_value>      # write to display register, <reg_addr> is offset by MMADR [180000H]\n");
        printf("\t\t\t<Help>    ::= -h                              # display this message\n");
        printf("\n");
        printf("\t\t\t<reg_addr>  ::= <hexnumber>\n");
        printf("\t\t\t<reg_value> ::= <hexnumber>\n");
        printf("\n");
        printf("\t\t\tWhere <hexnumber> is an undecorated hexadecimal, e.g. 6f00d0\n");
        printf("\n");
        return 1;
    }
    return 0;
}

struct pci_device *GetPciGraphicsDevice(void)
{
    struct pci_device *pci_dev;
    int error = 0;

    pci_dev = pci_device_find_by_slot(0, 0, 2, 0);
    if (pci_dev == NULL || pci_dev->vendor_id != 0x8086)
    {
        struct pci_device_iterator *iter;
        struct pci_id_match match;

        match.vendor_id = 0x8086; /* Intel */
        match.device_id = PCI_MATCH_ANY;
        match.subvendor_id = PCI_MATCH_ANY;
        match.subdevice_id = PCI_MATCH_ANY;

        match.device_class = 0x3 << 16;
        match.device_class_mask = 0xff << 16;

        match.match_data = 0;

        iter = pci_id_match_iterator_create(&match);
        pci_dev = pci_device_next(iter);
        pci_iterator_destroy(iter);
    }

    if (pci_dev == NULL)
    {
        printf("Failed to find intel graphics device\n");
        return NULL;
    }

    error = pci_device_probe(pci_dev);
    if (error)
    {
        printf("Failed to probe the graphics device, error %d\n", error);
        return NULL;
    }

    if (pci_dev->vendor_id != 0x8086)
    {
        printf("Error, found non-Intel graphics device (Vendor ID = %X)\n", pci_dev->vendor_id);
        return NULL;
    }

    return pci_dev;
}

int intel_gen(uint32_t devid)
{
    if (IS_GEN2(devid))        return 2;
    if (IS_GEN3(devid))        return 3;
    if (IS_GEN4(devid))        return 4;
    if (IS_GEN5(devid))        return 5;
    if (IS_GEN6(devid))        return 6;
    if (IS_GEN7(devid))        return 7;
    if (IS_GEN8(devid))        return 8;

    return -1;
}

int AcquireForceWake()
{
    char root[128];
    char path[1024];
    struct stat st;
    int n;
    int fd = -1;

    memset(root, 0, sizeof root);
    memset(path, 0, sizeof path);

    if (stat("/d/dri", &st) == 0)
    {
        // debugfs mounted under /d
        strcpy(root, "/d");
    }
    else
    {
        if (stat("/sys/kernel/debug/dri", &st))
        {
            // debugfs isn't mounted
            if (stat("/sys/kernel/debug", &st))
            {
                printf("/sys/kernel/debug error %d\n", errno);
                return -1;
            }
            if (mount("debug", "/sys/kernel/debug", "debugfs", 0, 0))
            {
                printf("Can't mount /sys/kernel/debug error %d\n", errno);
                return -1;
            }
        }
        // debugfs mounted under /sys/kernel/debug
        strcpy(root, "/sys/kernel/debug");
    }

    for(n = 0; n < 16; ++n)
    {
        snprintf(path, sizeof path - 1, "%s/dri/%d/i915_forcewake_user", root, n);
        if (stat(path, &st) == 0)
        {
            fd = open(path, O_WRONLY);
        }
    }
    return fd;
}

void ReleaseForceWake(int fd)
{
    if (fd != -1)
    {
        close(fd);
    }
}

struct mem_map_region *AcquireGTTAddress(struct pci_device *pci_dev)
{
    uint32_t gen;
    int mmio_bar;
    struct mem_map_region *region;
    int error;

    region = malloc(sizeof *region);
    if (region == NULL)
    {
        printf("Failed to malloc memory map structure\n");
        return NULL;
    }

    // mmio_bar and region->bytes magic numbers are both taken from lib/intel_mmio.c, which is part
    // of the intel-gpu-tools package. On GMIN this is part of the Android repository, under
    // hardware/intel.
    //
    if (IS_GEN2(pci_dev->device_id))
        mmio_bar = 1;
    else
        mmio_bar = 0;

    gen = intel_gen(pci_dev->device_id);
    if (gen < 3)
        region->bytes = 512*1024;
    else if (gen < 5)
        region->bytes = 512*1024;
    else
        region->bytes = 2*1024*1024;

    region->base_addr = NULL;
    error = pci_device_map_range(pci_dev, pci_dev->regions[mmio_bar].base_addr, region->bytes,
                                 PCI_DEV_MAP_FLAG_WRITABLE, &region->base_addr);

    if (error)
    {
        printf("Failed to map the graphics aperture, error %d\n", error);
        free(region);
        return NULL;
    }

    region->fd_force_wake = AcquireForceWake();
    return region;
}

void ReleaseGTTAddress(struct pci_device *pci_dev, struct mem_map_region *region)
{
    ReleaseForceWake(region->fd_force_wake);
    pci_device_unmap_range(pci_dev, region->base_addr, region->bytes);
    free(region);
}


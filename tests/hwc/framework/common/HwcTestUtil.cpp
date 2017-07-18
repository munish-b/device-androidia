/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcTestUtil.cpp

Description:    miscellaneous functions

Environment:

****************************************************************************/

#include "HwcTestUtil.h"
#include "HwcTestState.h"
#include <stdlib.h>
#include "hardware/hwcomposer2.h"
#include "graphics.h"

void CloseFence(int fence)
{
    if (HwcTestState::getInstance()->IsLive())
    {
        if (fence)
        {
            HWCLOGD_COND(eLogFence, "Close fence %d", fence);
            close(fence);
        }
        else
        {
            HWCLOGW_COND(eLogFence, "Skipped closing zero fence");
        }
    }
}

// Misc string functions
const char* strafter(const char *str, const char* search)
{
    const char* p = strstr(str, search);
    if (p == 0)
    {
        return 0;
    }

    return p + strlen(search);
}

int strncmpinc(const char*& p, const char* search)
{
    int len = strlen(search);

    int cmp = strncmp(p, search, len);

    if (cmp == 0)
    {
        p += len;
    }

    return cmp;
}

int atoiinc(const char*& p)
{
    int ret = atoi(p);
    if ((*p == '-') || (*p == '+'))
    {
        ++p;
    }

    while (isdigit(*p))
    {
        ++p;
    }

    return ret;
}

// Expecting a pointer of form 0xabcd01234567
// Will also accept without the 0x, but an error will be generated.

uintptr_t atoptrinc(const char*& p)
{
    // Skip 0x if supplied.
    HWCCHECK(eCheckBadPointerFormat);
    if (strncmpinc(p, "0x") != 0)
    {
        HWCERROR(eCheckBadPointerFormat, "0x missing from value: pointer formatting should be used");
    }

    uintptr_t h = strtoll(p, 0, 16);

    while (isdigit(*p) || ('a' <= *p && *p <= 'f') || ('A' <= *p && *p <= 'F'))
    {
        ++p;
    }

    return h;
}

double atofinc(const char*& p)
{
    double ret = atof(p);
    while (isdigit(*p) || (*p == '.') || (*p == '-') || (*p == '+'))
    {
        ++p;
    }

    return ret;
}

void skipws(const char*& p)
{
    while (isblank(*p))
    {
        ++p;
    }
}

android::String8 getWord(const char*& p)
{
    uint32_t len = 0;
    while (p[len] && !isblank(p[len]) && (p[len] != '\n'))
    {
        ++len;
    }

    android::String8 ret(p, len);
    p+=len;

    return ret;
}

bool ExpectChar(const char*& p, char c)
{
    if (*p != c)
    {
        HWCLOGV_COND(eLogParse, "Expecting '%c': %s", c, p);
        return false;
    }
    else
    {
        p++;
        return true;
    }
}

const char* TriStateStr(TriState ts)
{
    switch(ts)
    {
        case eTrue:
            return "TRUE";
        case eFalse:
            return "FALSE";
        case eUndefined:
            return "UNDEFINED";
        default:
            return "INVALID";
    }
}

const char* FormatToStr(uint32_t fmt)
{
    #define PRINT_FMT(FMT)      \
        if (fmt == FMT)         \
        {                       \
            return #FMT;        \
        }

    PRINT_FMT(HAL_PIXEL_FORMAT_RGBA_8888)
    else PRINT_FMT(HAL_PIXEL_FORMAT_BGRA_8888)
    else PRINT_FMT(HAL_PIXEL_FORMAT_RGBX_8888)
    else PRINT_FMT(HAL_PIXEL_FORMAT_RGB_565)
    else PRINT_FMT(HAL_PIXEL_FORMAT_RGBA_4444_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL)

    else PRINT_FMT(HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL)

    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_422_I)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_420_H_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_411_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_422_V_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_422_H_INTEL)
    else PRINT_FMT(HAL_PIXEL_FORMAT_YCbCr_444_INTEL)
    else
    {
        return "UNKNOWN";
    }
    #undef PRINT_FMT
}

// Determines whether this buffer is a video format
bool IsNV12(uint32_t format)
{
    return ((format == HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL) ||
            (format == HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL) ||
            (format == HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL) ||
            (format == HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL) ||
            (format == HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL));
}

bool HasAlpha(uint32_t format)
{
    return ((format == HAL_PIXEL_FORMAT_RGBA_8888)
        || (format == HAL_PIXEL_FORMAT_BGRA_8888)
        || (format == HAL_PIXEL_FORMAT_A2R10G10B10_INTEL)
        || (format == HAL_PIXEL_FORMAT_A2B10G10R10_INTEL));
}

void* dll_open(const char* filename, int flag)
{
    void* st = dlopen(filename, flag);

    if (st == 0)
    {
        ALOGE("dlopen failed to open %s, errno=%d/%s", filename, errno, strerror(errno));
        ALOGE("%s", dlerror());
    }

    return st;
}

void DumpMemoryUsage()
{
// The following is taken from /linux/kernl/fs/proc/array.c
// and shows us how to read the data in /proc/self/state

//    seq_printf(m, "%d (%s) %c", pid_nr_ns(pid, ns), tcomm, state);
//    seq_put_decimal_ll(m, ' ', ppid);
//    seq_put_decimal_ll(m, ' ', pgid);
//    seq_put_decimal_ll(m, ' ', sid);
//    seq_put_decimal_ll(m, ' ', tty_nr);
//    seq_put_decimal_ll(m, ' ', tty_pgrp);
//    seq_put_decimal_ull(m, ' ', task->flags);
//    seq_put_decimal_ull(m, ' ', min_flt);
//    seq_put_decimal_ull(m, ' ', cmin_flt);
//    seq_put_decimal_ull(m, ' ', maj_flt);
//    seq_put_decimal_ull(m, ' ', cmaj_flt);
//    seq_put_decimal_ull(m, ' ', cputime_to_clock_t(utime));
//    seq_put_decimal_ull(m, ' ', cputime_to_clock_t(stime));
//    seq_put_decimal_ll(m, ' ', cputime_to_clock_t(cutime));
//    seq_put_decimal_ll(m, ' ', cputime_to_clock_t(cstime));
//    seq_put_decimal_ll(m, ' ', priority);
//    seq_put_decimal_ll(m, ' ', nice);
//    seq_put_decimal_ll(m, ' ', num_threads);
//    seq_put_decimal_ull(m, ' ', 0);
//    seq_put_decimal_ull(m, ' ', start_time);
//    seq_put_decimal_ull(m, ' ', vsize);
//    seq_put_decimal_ull(m, ' ', mm ? get_mm_rss(mm) : 0);
//    seq_put_decimal_ull(m, ' ', rsslim);
//    seq_put_decimal_ull(m, ' ', mm ? (permitted ? mm->start_code : 1) : 0);
//    seq_put_decimal_ull(m, ' ', mm ? (permitted ? mm->end_code : 1) : 0);
//    seq_put_decimal_ull(m, ' ', (permitted && mm) ? mm->start_stack : 0);
//    seq_put_decimal_ull(m, ' ', esp);
//    seq_put_decimal_ull(m, ' ', eip);
//    /* The signal information here is obsolete.
//     * It must be decimal for Linux 2.0 compatibility.
//     * Use /proc/#/status for real-time signals.
//     */
//    seq_put_decimal_ull(m, ' ', task->pending.signal.sig[0] & 0x7fffffffUL);
//    seq_put_decimal_ull(m, ' ', task->blocked.sig[0] & 0x7fffffffUL);
//    seq_put_decimal_ull(m, ' ', sigign.sig[0] & 0x7fffffffUL);
//    seq_put_decimal_ull(m, ' ', sigcatch.sig[0] & 0x7fffffffUL);
//    seq_put_decimal_ull(m, ' ', wchan);
//    seq_put_decimal_ull(m, ' ', 0);
//    seq_put_decimal_ull(m, ' ', 0);
//    seq_put_decimal_ll(m, ' ', task->exit_signal);
//    seq_put_decimal_ll(m, ' ', task_cpu(task));
//    seq_put_decimal_ull(m, ' ', task->rt_priority);
//    seq_put_decimal_ull(m, ' ', task->policy);
//    seq_put_decimal_ull(m, ' ', delayacct_blkio_ticks(task));
//    seq_put_decimal_ull(m, ' ', cputime_to_clock_t(gtime));
//    seq_put_decimal_ll(m, ' ', cputime_to_clock_t(cgtime));
//
//    if (mm && permitted) {
//        seq_put_decimal_ull(m, ' ', mm->start_data);
//        seq_put_decimal_ull(m, ' ', mm->end_data);
//        seq_put_decimal_ull(m, ' ', mm->start_brk);
//        seq_put_decimal_ull(m, ' ', mm->arg_start);
//        seq_put_decimal_ull(m, ' ', mm->arg_end);
//        seq_put_decimal_ull(m, ' ', mm->env_start);
//        seq_put_decimal_ull(m, ' ', mm->env_end);
//    } else
//        seq_printf(m, " 0 0 0 0 0 0 0");

    if (HwcTestState::getInstance()->IsOptionEnabled(eLogResources))
    {
        FILE* f = fopen("/proc/self/stat","r");
        if (f)
        {
            uint32_t d;
            uint32_t vm;
            char s[HWCVAL_DEFAULT_STRLEN];
            fscanf(f, "%s %s %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                s, s, s, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &vm,
                &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d, &d);
            fclose(f);

            HWCLOGA("VM USAGE: %4.1fMB", double(vm) / 1000000);
        }
        else
        {
            HWCLOGW("Can't open /proc/self/stat");
        }
    }
}

android::Vector<char*> splitString(char* str)
{
    android::Vector<char*> sv;
    char* s = str;
    while (*s)
    {
        sv.add(s);
        while (*s && (*s != ' '))
        {
            ++s;
        }

        if (*s == ' ')
        {
            *s='\0';
            ++s;
        }
    }

    return sv;
}

android::Vector<char*> splitString(android::String8 str)
{
    char* s = str.lockBuffer(str.size());
    return splitString(s);
}

Hwcval::FrameNums::operator android::String8() const
{
    android::String8 str = android::String8::format("frame:%d", mFN[0]);

    for (uint32_t i=1; i<HWCVAL_MAX_CRTCS; ++i)
    {
        str += android::String8::format(".%d", mFN[i]);
    }

    return str;
}




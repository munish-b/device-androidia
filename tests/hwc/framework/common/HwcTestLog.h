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

File Name:      HwcTestLog.h

Description:    Test logger function.

Environment:

Notes:

****************************************************************************/


#ifndef __HwcTestLog_h__
#define __HwcTestLog_h__

#if HWCVAL_SYSTRACE
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>
#undef ATRACE_CALL
#define ATRACE_CALL() ATRACE_NAME(__PRETTY_FUNCTION__)
#else
#ifndef ATRACE_CALL
#define ATRACE_CALL()
#define ATRACE_BEGIN(str)
#define ATRACE_END()
#endif
#endif

#include <cutils/log.h>
#include "HwcTestConfig.h"

HwcTestResult* HwcGetTestResult();
HwcTestConfig* HwcGetTestConfig();

int HwcValLogVerbose(int priority, const char* context, int line, const char* fmt, ...);
int HwcValLog(int priority, const char* fmt, ...);
int HwcValLogVA(int priority, const char* fmt, va_list& args);
int HwcValError(HwcTestCheckType check, HwcTestConfig* config, HwcTestResult* result, const char* fmt, ...);

#ifndef HWCVAL_LOG_VERBOSE
#define HWCLOG(LEVEL, ...) ALOGE(__VA_ARGS__)
#else
#define HWCLOG(LEVEL, ...)                                                     \
  HwcValLogVerbose(LEVEL, __FILE__, __LINE__, __VA_ARGS__)
#endif

#define HWCLOGV(...) HWCLOG(ANDROID_LOG_VERBOSE,__VA_ARGS__)
#define HWCLOGD(...) HWCLOG(ANDROID_LOG_DEBUG,__VA_ARGS__)
#define HWCLOGI(...) HWCLOG(ANDROID_LOG_INFO,__VA_ARGS__)
#define HWCLOGW(...) HWCLOG(ANDROID_LOG_WARN,__VA_ARGS__)
#define HWCLOGE(...) HWCLOG(ANDROID_LOG_ERROR,__VA_ARGS__)
#define HWCLOGF(...) HWCLOG(ANDROID_LOG_FATAL,__VA_ARGS__)
#define HWCLOGA(...) HWCLOG(ANDROID_LOG_UNKNOWN,__VA_ARGS__) // Log always

#define HWCLOGV_IF(COND,...) if (COND) HWCLOGV(__VA_ARGS__)
#define HWCLOGD_IF(COND,...) if (COND) HWCLOGD(__VA_ARGS__)
#define HWCLOGI_IF(COND,...) if (COND) HWCLOGI(__VA_ARGS__)
#define HWCLOGW_IF(COND,...) if (COND) HWCLOGW(__VA_ARGS__)
#define HWCLOGE_IF(COND,...) if (COND) HWCLOGE(__VA_ARGS__)
#define HWCLOGF_IF(COND,...) if (COND) HWCLOGF(__VA_ARGS__)

#define HWCCHECK(CHECKNUM) HwcGetTestResult()->IncEval(CHECKNUM)
#define HWCCHECK_ADD(CHECKNUM,ADD) HwcGetTestResult()->AddEval(CHECKNUM, ADD)
#define HWCERROR(CHECKNUM, ...) HwcValError(CHECKNUM, HwcGetTestConfig(), HwcGetTestResult(), __VA_ARGS__)
#define HWCCOND(CHECK) (HwcGetTestConfig()->mCheckConfigs[CHECK].enable)

#define HWCLOGV_COND(CHECK, ...) HWCLOGV(__VA_ARGS__)
#define HWCLOGD_COND(CHECK, ...) HWCLOGD(__VA_ARGS__)
#define HWCLOGI_COND(CHECK,...) if HWCCOND(CHECK) HWCLOGI(__VA_ARGS__)
#define HWCLOGW_COND(CHECK,...) if HWCCOND(CHECK) HWCLOGW(__VA_ARGS__)
#define HWCLOGE_COND(CHECK,...) if HWCCOND(CHECK) HWCLOGE(__VA_ARGS__)
#define HWCLOGF_COND(CHECK,...) if HWCCOND(CHECK) HWCLOGF(__VA_ARGS__)
#define HWCLOG_COND(LEVEL,CHECK,...) if HWCCOND(CHECK) HWCLOG(LEVEL,__VA_ARGS__)

#endif // __HwcTestLog_h__


/****************************************************************************
*
* Copyright (c) Intel Corporation (2015).
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
* File Name:            HwchInputGenerator.cpp
*
* Description:          Keypress generator class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchInputGenerator.h"
#include "HwcTestState.h"

#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Hwch::InputGenerator::InputGenerator()
  : mRunning(false),
    mActive(false),
    mKeypressFailed(false)
{
    run("HwchInputGenerator", android::PRIORITY_NORMAL);
    mRunning = true;

    Open();
}

Hwch::InputGenerator::~InputGenerator()
{
}

void Hwch::InputGenerator::Open()
{
    mFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    ALOG_ASSERT(mFd > 0);

    int ret = ioctl(mFd, UI_SET_EVBIT, EV_KEY);
    if (ret)
    {
        HWCERROR(eCheckInternalError, "Hwch::InputGenerator::Keypress ioctl UI_SET_EVBIT returned %d", ret);
        mKeypressFailed = true;
        return;
    }

    ret = ioctl(mFd, UI_SET_KEYBIT, KEY_A);
    if (ret)
    {
        HWCERROR(eCheckInternalError, "Hwch::InputGenerator::Keypress ioctl UI_SET_KEYBIT returned %d", ret);
        mKeypressFailed = true;
        return;
    }

    struct uinput_user_dev uidev;

    memset(&uidev, 0, sizeof(uidev));

    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;

    ret = write(mFd, &uidev, sizeof(uidev));
    if (ret < 0)
    {
        mKeypressFailed = true;
        HWCERROR(eCheckInternalError, "Hwch::InputGenerator::Keypress Failed to write uinput_user_dev (%d)", ret);
        return;
    }

    ret = ioctl(mFd, UI_DEV_CREATE);
    if (ret)
    {
        mKeypressFailed = true;
        HWCERROR(eCheckInternalError, "Hwch::InputGenerator::Keypress failed to create spoof keypress device (%d)", ret);
        return;
    }
}

void Hwch::InputGenerator::Keypress()
{
    if (mKeypressFailed)
    {
        return;
    }

    int ret =0;

    if (mFd == 0)
    {
        Open();
    }

    // Key down
    WriteEvent(EV_KEY, KEY_A, 1);

    // Synchronize
    WriteEvent(EV_SYN, 0, 0);

    // Key up
    WriteEvent(EV_KEY, KEY_A, 0);

    // Synchronize
    WriteEvent(EV_SYN, 0, 0);
}

int Hwch::InputGenerator::WriteEvent(int type, int code, int value)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));

    ev.type = type;
    ev.code = code;
    ev.value = value;

    int ret = write(mFd, &ev, sizeof(ev));

    if (ret < 0)
    {
        HWCERROR(eCheckInternalError, "Hwch::InputGenerator::Keypress failed to write type %d code %d value %d",
            type, code, value);
    }
    else
    {
        HWCLOGV_COND(eLogVideo, "Hwch::InputGenerator::Keypress wrote %d bytes (%d %d %d)", ret, type, code, value);
    }

    return ret;
}


// Set mode to input active,
void Hwch::InputGenerator::SetActive(bool active)
{
    if (active)
    {
        Keypress();

        if (!mRunning)
        {
            run("HwchInputGenerator", android::PRIORITY_NORMAL);
            mRunning = true;
        }
    }
    else
    {
        if (mActive)
        {
            mInactiveTime = systemTime(SYSTEM_TIME_MONOTONIC) + (HWCVAL_US_TO_NS * mTimeoutPeriodUs);
            HWCLOGD_COND(eLogVideo, "Stopping keypress generation. input timeout stability expected after %dus at %f",
                mTimeoutPeriodUs, double(mInactiveTime) / HWCVAL_SEC_TO_NS);
        }
    }

    mActive = active;
}

void Hwch::InputGenerator::Stabilize()
{
    if (!mActive)
    {
        if (mInactiveTime)
        {
            int64_t t = systemTime(SYSTEM_TIME_MONOTONIC);
            int us = int((mInactiveTime - t) / HWCVAL_US_TO_NS);

            if (us > 0)
            {
                HWCLOGD_COND(eLogVideo, "Waiting %dus until stability at %f",
                    us, double(mInactiveTime) / HWCVAL_SEC_TO_NS);
                usleep(us);
            }
        }
    }
}

void Hwch::InputGenerator::SetActiveAndWait(bool active)
{
    SetActive(active);
    Stabilize();
}

// Thread functions
bool Hwch::InputGenerator::threadLoop()
{
    while (!exitPending())
    {
        if (mActive)
        {
            Keypress();
        }

        usleep(mKeypressIntervalUs);
    }

    return false;
}

android::status_t Hwch::InputGenerator::readyToRun()
{
    return android::NO_ERROR;
}

const uint32_t Hwch::InputGenerator::mKeypressIntervalUs = 1 * HWCVAL_SEC_TO_US;
const uint32_t Hwch::InputGenerator::mTimeoutPeriodUs = 4 * HWCVAL_SEC_TO_US;

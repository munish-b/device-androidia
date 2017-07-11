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
* File Name:            HwchPavpSession.cpp
*
* Description:          Hwc harness Protected Content Session class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchPavpSession.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#ifdef HWCVAL_BUILD_PAVP
    #include "libpavp.h"
    #if ANDROID_VERSION >= 600
        #define HWCVAL_PAVP_REFACTORING
    #endif
#endif

Hwch::PavpSession::PavpSession()
  : mOldPavpInstance(0xffffffff)
  , mProtectedContentStarted(false)
#ifdef HWCVAL_BUILD_PAVP
  , mCoreu(0)
#endif
{
    for (uint32_t i=0; i<HWCVAL_MAX_PROT_SESSIONS; ++i)
    {
        mSessionInstance[i] = -1; // session not created
    }
}

Hwch::PavpSession::~PavpSession()
{
#ifdef HWCVAL_BUILD_PAVP
    CloseCoreU();
#endif
}

int32_t Hwch::PavpSession::StartPavpSession()
{
    ALOGI("Entered StartPavpSession");
#ifdef HWCVAL_BUILD_PAVP

    // Get a PAVP session handle
    pavp_lib_session *pLibInstance;
    int32_t rc = GetPavpLibHandle(&pLibInstance);

    HWCCHECK(eCheckPavpFail);
    if (rc != pavp_lib_session::status_ok || !(pLibInstance))
    {
        // HWCERROR will ultimately be logged by the caller
        HWCLOGE("Can't initialise PAVP library status=%d", rc);
        return -1;
    }

    // Create a PAVP session. Retry if necessary
    rc = CreatePavpSession(pLibInstance);
    uint32_t repeat=10;
    while ((rc == pavp_lib_session::operation_failed) && (--repeat > 0))
    {
        sleep(2);
        rc = CreatePavpSession(pLibInstance);
    }

    HWCCHECK(eCheckPavpFail);
    if (rc != pavp_lib_session::status_ok)
    {
        // HWCERROR will ultimately be logged by the caller
        HWCLOGE("Can't create PAVP session status=%d", rc);
        return -1;
    }

    // Get Session ID (a.k.a. app_id).
    uint32_t pavpSessionId;
    rc = GetPavpSession(pLibInstance, pavpSessionId);

    HWCCHECK(eCheckPavpFail);
    if (rc != pavp_lib_session::status_ok)
    {
        HWCLOGE("Can't get PAVP session id, status=%d", rc);
        return -1;
    }

    HWCLOGA("PAVP Session %d created", pavpSessionId);
    return pavpSessionId;
#else
    HWCVAL_UNUSED(mOldPavpInstance);
    return 0;
#endif
}

#ifdef HWCVAL_BUILD_PAVP
void Hwch::PavpSession::CloseCoreU()
{
    if (mCoreu)
    {
        CoreUDestroy(mCoreu);
        mCoreu = 0;
    }
}

int32_t Hwch::PavpSession::GetPavpLibHandle(pavp_lib_session **pLibInstance)
{
    return pavp_lib_session::pavp_lib_init(INTERFACE_VERSION, pLibInstance);
}

int32_t Hwch::PavpSession::CreatePavpSession(pavp_lib_session *pLibInstance)
{
    int32_t rc = pavp_lib_session::invalid_arg;

    if (!pLibInstance)
    {
        return rc;
    }

    // Create Heavy Mode Session
#ifndef HWCVAL_PAVP_REFACTORING
    bool bIsHeavyMode = true;
    bool bIsTranscode = false;
    rc = pLibInstance->pavp_create_session(bIsHeavyMode, bIsTranscode);
#else
    pavp_lib_session::pavp_mode session_mode = pavp_lib_session::pavp_mode_heavy;
    pavp_lib_session::pavp_type session_type = pavp_lib_session::pavp_type_decode;
    rc = pLibInstance->pavp_create_session(session_mode, session_type);
#endif

    return rc;
}

int32_t Hwch::PavpSession::GetPavpSession(pavp_lib_session *pLibInstance, uint32_t& id)
{
    return pLibInstance ? pLibInstance->pavp_get_app_id(id) :
           pavp_lib_session::invalid_arg;
}

int32_t Hwch::PavpSession::StartCoreUSession(int32_t pavpSessionId)
{
    // Create a CoreU session.
    // This will allow us to create protected buffers which can then be used by HWC.
    //
    // Note - I'm using ALOG rather than HWCLOG here because CoreU puts debug in the
    // Android log, and it's helpful to see which call generates which log entries.
    //
    // This code is adapted from core/test/coreu_test/coreu_test.c
    //
    ALOGI("HwchSystem: Starting CoreU Initialization");
    mCoreu = CoreUInit();

    HWCCHECK(eCheckPavpFail);
    if (mCoreu == 0)
    {
        HWCLOGE("Failed to initialize CoreU");
        return false;
    }

    bool bMultiPavpSessionSupport = true;
    HWCCHECK(eCheckPavpFail);
    if (!CoreUPavpSetMDRMMode(
        mCoreu,
        bMultiPavpSessionSupport))
    {
        HWCLOGE("Failure to set multi-PAVP session support");
    }

    pid_t pid = getpid();
    PAVP_PROTECTION_INFO PavpInfo;
    bool bRestoreCheckPowerState = false;

    // Temporarily turn off CoreU power state checking if it's currently on
    PavpInfo.Action = PAVP_ACTION_TEST;
    PavpInfo.ulProcessID = (ULONG)pid;
    PavpInfo.TestParams.TestOperation = PAVP_TEST_GET_POWERCHECK;
    if (CoreUDoPavpAction(mCoreu, &PavpInfo, sizeof(PavpInfo)) &&
        PavpInfo.bReturnValue)
    {
        bool checkPowerState = (PavpInfo.TestParams.Param[0] == 1);
        ALOGI("    Current CoreU power checking is %s", (checkPowerState) ? "on" : "off");

        if (checkPowerState)
        {
            PavpInfo.TestParams.TestOperation = PAVP_TEST_SET_POWERCHECK;
            PavpInfo.TestParams.Param[0] = 0;
            if (CoreUDoPavpAction(mCoreu, &PavpInfo, sizeof(PavpInfo)) &&
                PavpInfo.bReturnValue)
            {
                ALOGI("    Temporarily turned off");
                bRestoreCheckPowerState = TRUE;
            }
            else
            {
                ALOGE("    Error turning off power state checking");
            }
        }
    }
    else
    {
        ALOGE("    Error getting current power state checking parameter\n");
    }

    ALOGI("HwchPavpSession::GetSessionInstance:");
    int32_t pavpInstance = GetSessionInstance(pavpSessionId);
    if (pavpInstance == 0)
    {
        HWCLOGI("No PAVP session allocated");
        return -1;
    }

    HWCLOGA("HwchSystem: Finished CoreU Initialization");
    return pavpInstance;
}

// Transitions a session to "ready" state and then returns its instance Id.
uint32_t Hwch::PavpSession::GetSessionInstance(int32_t session)
{
    PAVP_SESSION_STATUS sessionStatus;
    PAVP_SESSION_ID sessionId;
    sessionId.SessionIdValue = session;

    if (GetPavpSessionStatus(session) < PAVP_SESSION_POWERUP)
    {
        SetPavpSession(session, PAVP_SESSION_POWERUP, "POWERUP");
    }

    if (GetPavpSessionStatus(session) < PAVP_SESSION_RECOVER)
    {
        SetPavpSession(session, PAVP_SESSION_RECOVER, "RECOVER");
    }

    if (GetPavpSessionStatus(session) < PAVP_SESSION_IN_INIT)
    {
        SetPavpSession(session, PAVP_SESSION_IN_INIT,"IN_INIT");
    }

    if (GetPavpSessionStatus(session) < PAVP_SESSION_INITIALIZED)
    {
        SetPavpSession(session, PAVP_SESSION_INITIALIZED, "INITIALIZED");
    }

    if (GetPavpSessionStatus(session) < PAVP_SESSION_READY)
    {
        SetPavpSession(session, PAVP_SESSION_READY, "READY");
    }

#ifndef HWCVAL_PAVP_REFACTORING
    GMM_PAVP_MODE pavpMode;
#else
    KM_PAVP_SESSION_MODE pavpMode;
#endif

    uint32_t instanceId = 0;
    CoreUPavpQuerySessionStatus(mCoreu, sessionId, &sessionStatus, &pavpMode, &instanceId);

    HWCCHECK(eCheckPavpFail);
    if (instanceId == 0)
    {
        HWCLOGE("Failure to create PAVP instance, please see logcat");
        return 0;
    }
    else if (sessionStatus != PAVP_SESSION_READY)
    {
        HWCLOGE("PAVP session is not READY.");
        return 0;
    }
    else
    {
        if (session < HWCVAL_MAX_PROT_SESSIONS)
        {
            mSessionInstance[session] = instanceId;
        }

        HWCLOGA("PAVP Instance %d created", instanceId);
    }

    return instanceId;
}

PAVP_SESSION_STATUS Hwch::PavpSession::GetPavpSessionStatus(int32_t pavpSessionId)
{
    PAVP_SESSION_STATUS status;
    PAVP_SESSION_ID sessionId;
#ifndef HWCVAL_PAVP_REFACTORING
    GMM_PAVP_MODE pavpMode;
#else
    KM_PAVP_SESSION_MODE pavpMode;
#endif

    uint32_t instanceId;
    sessionId.SessionIdValue = pavpSessionId;

    if (!CoreUPavpQuerySessionStatus(mCoreu, sessionId, &status, &pavpMode, &instanceId))
    {
        return PAVP_SESSION_UNINITIALIZED;
    }
    else
    {
        //mPreviousStatus = status;
        ALOGI("CoreUPavpGetSession id=%d status=%d", pavpSessionId, status);
        return status;
    }
}

void Hwch::PavpSession::SetPavpSession(int session, PAVP_SESSION_STATUS status, const char* str)
{
    ALOGI("CoreUPavpChangeSession(sessionId=0x%x, %s):",mPavpSessionId, str);
    PAVP_SESSION_STATUS previousStatus;
    PAVP_SESSION_ID sessionId;
    sessionId.SessionIdValue = session;

    if (!CoreUPavpChangeSession(mCoreu,
                                KM_PAVP_SESSION_TYPE_DECODE,
#ifdef HWCVAL_PAVP_REFACTORING
                                KM_PAVP_SESSION_MODE_HEAVY,
#endif
                                &sessionId,
                                &status,
                                &previousStatus))
    {
        HWCLOGE("Test failed to transition CoreU session %d to %s", session, str);
    }
}
#endif // HWCVAL_BUILD_PAVP

bool Hwch::PavpSession::StartProtectedContent()
{
#ifdef HWCVAL_BUILD_PAVP
    mOldPavpInstance = mPavpInstance;
    ALOGI("Starting PAVPSession thread");
    run("Hwch::PavpSession", android::PRIORITY_NORMAL);
    return true;
#else
    HWCERROR(eCheckFeatureNotInBuild, "PAVP not supported, so this test can't be run.");
    return false;
#endif
}

// Do NOT call this immediately after StartProtectedContent().
// Send some frames through first.
bool Hwch::PavpSession::ProtectedContentStarted()
{
#ifdef HWCVAL_BUILD_PAVP
    // Wait for initialization, if still running.
    join();

    if (!mProtectedContentStarted)
    {
        HWCERROR(eCheckPavpFail, "PAVP/CoreU failed to start, see log.");
    }
    else if (mPavpInstance == mOldPavpInstance)
    {
        HWCERROR(eCheckPavpFail, "New PAVP instance has not been created. Still instance %d", mPavpInstance);
    }
#else
    HWCERROR(eCheckFeatureNotInBuild, "PAVP not supported, so this test can't be run.");
#endif
    return mProtectedContentStarted;
}

bool Hwch::PavpSession::threadLoop()
{
#ifdef HWCVAL_BUILD_PAVP
    uint32_t sessionToKill;
    if (mPavpSessionId >= 3)
    {
        sessionToKill = mPavpSessionId - 3;
    }
    else
    {
        // Sessions are in a cycle of 15, i.e. in the range 0-14
        sessionToKill = mPavpSessionId + (HWCVAL_MAX_PROT_SESSIONS - 3);
    }

    for (uint32_t tries=1; tries <= 5; ++tries)
    {
        // Destroy an old session, so we never run out
        // (if the session has been created)
        if (mSessionInstance[sessionToKill] >= 0)
        {
            HWCLOGI("Terminating PAVP session %d", sessionToKill);
            SetPavpSession(sessionToKill, PAVP_SESSION_TERMINATE_IN_PROGRESS, "TERMINATE_IN_PROGRESS");
            SetPavpSession(sessionToKill, PAVP_SESSION_UNINITIALIZED, "UNINITIALIZED");
            mSessionInstance[sessionToKill] = -1;
        }

        if (tries > 1)
        {
            HWCLOGI("Creating PAVP session (try %d)", tries);
        }

        int32_t pavpSessionId = StartPavpSession();
        if (pavpSessionId >= 0)
        {
            int32_t pavpInstanceId =  StartCoreUSession(pavpSessionId);

            if (pavpInstanceId > 0)
            {
                mPavpSessionId = pavpSessionId;
                mPavpInstance = pavpInstanceId;
                mProtectedContentStarted = true;

                if (!mQuiet)
                {
                    printf("Protected Content Session %d Instance %d\n", mPavpSessionId, mPavpInstance);
                }

                HWCLOGI("Protected Content Session %d Instance %d", mPavpSessionId, mPavpInstance);
                return false;
            }
        }

        // Try deleting a different session
        sessionToKill = (sessionToKill + 1) % HWCVAL_MAX_PROT_SESSIONS;
    }

    // After 5 tries, we still don't have a session
    HWCERROR(eCheckPavpFail, "No PAVP session allocated");

#endif

    return false;
}

android::status_t Hwch::PavpSession::readyToRun()
{
    return android::NO_ERROR;
}



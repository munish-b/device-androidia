/****************************************************************************

 Copyright (c) Intel Corporation (2015).

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

 File Name:     rotation.java

 Description:   general screen rotation tests. It's probably better to put
                app specific rotation tests in a separate app class for
                all but the simplest apps.

 Environment:

 Notes:         UiAutomator rotation doesn't work as well as physically
                rotating the device, e.g. the camera app will rotate its
                UI but still record portrait or landscape according to its
                physical orientation.

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.app.UiAutomation;
import android.graphics.Rect;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


public class rotation extends uiautomatorHelper
{
    // ETM X-Component test: CheckResolution_Rotation (single res)
    public void testRotation() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testRotation";
        UiAutomation uia = getInstrumentation().getUiAutomation();
        UiObject2 uiobj;
        Rect bounds;

        Log.d(TAG, "testRotation: testing 270...");
        uia.setRotation(UiAutomation.ROTATION_FREEZE_270);
        mDevice.pressHome();
        SystemClock.sleep(mUpdateTimeout);
        if (!launchApp(CALCULATOR_PACKAGE))
        {
            Log.e(TAG, "testRotation: failed to open Calculator - trying the 'all apps' panel");
            if(!launchApp(CALCULATOR_PACKAGE, "Calculator"))
            {
                Log.e(TAG, "testRotation: can't find the calculator");
            }
        }
        else
        {
            keyInNumberString("2468");
            SystemClock.sleep(mShortDelayForObserver);
            keyInArithmeticOperator('/');
            SystemClock.sleep(mShortDelayForObserver);
            keyInNumberString("2");
            SystemClock.sleep(mShortDelayForObserver);
            keyInArithmeticOperator('=');
            SystemClock.sleep(mLongDelayForObserver);
        }
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testRotation: testing 90...");
        uia.setRotation(UiAutomation.ROTATION_FREEZE_90);
        mDevice.pressHome();
        SystemClock.sleep(mUpdateTimeout);
        if(!launchApp(CALENDAR_PACKAGE))
        {
            Log.e(TAG, "testRotation: failed to open calendar - trying the 'all apps' panel");
            if(!launchApp(CALENDAR_PACKAGE, "Calendar"))
            {
                Log.e(TAG, "testRotation: can't find the calendar");
            }
        }
        else
        {
            uiobj = findUiObject2(By.res("com.android.calendar:id/swipe_refresh_layout"));
            if(uiobj == null)
            {
                Log.w(TAG, "testRotation: can't swipe the Calendar");
                SystemClock.sleep(mLongDelayForObserver);
            }
            else
            {
                bounds = uiobj.getVisibleBounds();
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
            }
        }
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testRotation: testing 180...");
        uia.setRotation(UiAutomation.ROTATION_FREEZE_180);
        mDevice.pressHome();
        SystemClock.sleep(mUpdateTimeout);
        openAlarmPanel();
        addAlarm(12);
        SystemClock.sleep(5000);
        deleteAlarm();

        Log.d(TAG, "testRotation: natural orientation...");
        mDevice.setOrientationNatural();

        Log.d(TAG, "testRotation: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: CheckResolution_Rotation (multi res)
    public void testRotationMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testRotationMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testRotationMultiRes: looking for mode %s", mode));
            for(String hdmiMode : availableModes)
            {
                if(mode.equals(hdmiMode))
                {
                    modeFound = true;
                    break;
                }
            }
            if(modeFound)
            {
                Log.d(TAG, String.format("testRotationMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testRotationMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testRotationMultiRes: selected mode %s, running testRotation()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testRotation();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testRotationMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testRotationMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testRotationMultiRes: finished");
        SendSuccessStatus(testName, true);
    }
}

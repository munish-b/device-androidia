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

 File Name:     notificationShade.java

 Description:   tests for the notification shade (the pull-down panel) on
                Android's desktop.

 Environment:

 Notes:

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.graphics.Rect;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.util.Log;
import android.os.RemoteException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


public class notificationShade extends uiautomatorHelper
{
    // ETM X-Component test: Basic display, Notification layer_external display (single res)
    public void testNotification() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testNotification";
        int             alarmMinutes = 2;
        UiObject2       uiobj;
        Rect            bounds;

        Log.d(TAG, "testNotification: setting alarm to go off in " + alarmMinutes + " minutes");
        openAlarmPanel();
        addAlarm(alarmMinutes);
        homeScreen();

        // let the user see the alarm in the notification panel
        mDevice.openNotification();
        SystemClock.sleep(mShortDelayForObserver); // used to know what's going on

        Log.d(TAG, "testNotification: waiting for the alarm...");
        waitAlarm(alarmMinutes);

        Log.d(TAG, "testNotification: snoozing the alarm...");
        checkForNotificationButtonText("Snooze", true);

        closeNotification();

        Log.d(TAG, "testNotification: opening Apps");
        mDevice.pressHome();
        try
        {
            // give the UI longer time = "mUpdateTimeout time" to respond
            uiobj = waitUiObject2(By.desc("Apps"), mUpdateTimeout);
            uiobj.click();
        }
        catch(NullPointerException npe)
        {
            Log.e(TAG, "testNotification: failed to open the all apps panel");
        }

        if(!launchApp(CALENDAR_PACKAGE))
        {
            Log.e(TAG, "testNotification: failed to open Calendar - trying the 'all apps' panel");
            if(!launchApp(CALENDAR_PACKAGE, "Calendar"))
            {
                // the purpose of the test is to check the alarm notification feature - don't fail it
                // just because we can't play with another app while the alarm is pending
                Log.e(TAG, "testNotification: can't find the calendar");
            }
        }
        else
        {
            uiobj = waitUiObject2(By.res("com.android.calendar:id/swipe_refresh_layout"), mUpdateTimeout);
            if(uiobj == null)
            {
                Log.w(TAG, "testNotification: can't swipe the calendar");
                SystemClock.sleep(mLongDelayForObserver);
            }
            else
            {
                // simulate the finger swiping across the display
                bounds = uiobj.getVisibleBounds();
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.right - 100, bounds.centerY(), bounds.left + 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
                mDevice.swipe(bounds.left + 100, bounds.centerY(), bounds.right - 100, bounds.centerY(), 200);
            }
        }

        Log.d(TAG, "testNotification: opening Apps");
        mDevice.pressHome();
        try
        {
            uiobj = waitUiObject2(By.desc("Apps"), mUpdateTimeout);
            uiobj.click();
        }
        catch(NullPointerException npe)
        {
            // in this specific test the click failing doesn't effect the validation result
            Log.e(TAG, "testNotification: failed to open the all apps panel (2)");
        }

        if(!launchApp(CALCULATOR_PACKAGE))
        {
            Log.e(TAG, "testNotification: failed to open Calendar - trying the 'all apps' panel");
            if(!launchApp(CALENDAR_PACKAGE, "Calendar"))
            {
                // the purpose of the test is to check the alarm notification feature - don't fail it
                // just because we can't play with another app while the alarm is pending
                Log.e(TAG, "testNotification: can't find the calculator");
            }
        }
        else
        {
            keyInNumberString("1234");
            SystemClock.sleep(mShortDelayForObserver);
            keyInArithmeticOperator('*');
            SystemClock.sleep(mShortDelayForObserver);
            keyInNumberString("2");
            SystemClock.sleep(mShortDelayForObserver);
            keyInArithmeticOperator('=');
            SystemClock.sleep(mLongDelayForObserver);
        }

        homeScreen();
        SystemClock.sleep(mShortDelayForObserver);
        openAndroidSettings("About tablet");
        SystemClock.sleep(mShortDelayForObserver);
        switchToApp("Calendar");
        SystemClock.sleep(mShortDelayForObserver);
        homeScreen();

        Log.d(TAG, "testNotification: dismissing the snoozing alarm...");
        SystemClock.sleep(mShortDelayForObserver);
        checkForNotificationButtonText("Dismiss", true);
        closeNotification();

        Log.d(TAG, "testNotification: deleting the alarm...");
        SystemClock.sleep(mShortDelayForObserver);
        openAlarmPanel();
        SystemClock.sleep(mShortDelayForObserver);
        deleteAlarm();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testNotification: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Basic display, Notification layer_external display (multi res)
    public void testNotificationMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testNotificationMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<String>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testNotificationMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testNotificationMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testNotificationMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testNotificationMultiRes: selected mode %s, running testNotification()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testNotification();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testNotificationMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testNotificationMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testNotificationMultiRes: finished");
        SendSuccessStatus(testName, true);
    }
}

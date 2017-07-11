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

File Name:      notificationShade.java

Description:    UiAutomator tests for the system Notification Shade.

Environment:

Notes:

****************************************************************************/

package com.intel.hwcval;

import java.util.List;
import java.util.ListIterator;

import android.os.RemoteException;
import android.util.Log;

import com.android.uiautomator.core.UiDevice;
import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;
import com.android.uiautomator.core.UiScrollable;
import com.intel.hwcval.uiautomatorHelper;

public class notificationShade extends UiAutomatorTestCase
{
    private static final String TAG = uiautomatorHelper.TAG;

    public void testNotification() throws RemoteException, UiObjectNotFoundException
    {
        final String testName = "testNotification";
        UiDevice    uidev = getUiDevice();
        int         alarmMinutes = 2;
        UiSelector  uisel;
        UiObject    uiobj;

        Log.d(TAG, "testNotification: locking device");
        uiautomatorHelper.lockDevice(this);

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testNotification: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        Log.d(TAG, "testNotification: setting alarm to go off in " + alarmMinutes + " minutes");
        uiautomatorHelper.setAlarm(this, alarmMinutes);

        uidev.pressHome();
        sleep(1000);
        uidev.openNotification();
        sleep(1000);

        Log.d(TAG, "testNotification: waiting for alarm...");
        uiautomatorHelper.waitAlarm(this, alarmMinutes);

        Log.d(TAG, "testNotification: snoozing the alarm...");
        uiautomatorHelper.checkForNotificationButtonText(this, "Snooze", true);
        sleep(1000);

        uidev.openNotification();
        sleep(1000);

        Log.d(TAG, "testNotification: opening the calendar...");
        uiautomatorHelper.gotoApps(this);
        if(!uiautomatorHelper.launchApp(this, "Calendar"))
        {
            Log.e(TAG, "testNotification: error, couldn't find calendar");
        }
        else
        {
            sleep(2000);
            try
            {
                uisel = new UiSelector().resourceIdMatches("(.*)(main_pane)");
                uiobj = new UiObject(uisel);
                uiobj.swipeLeft(100);
                sleep(2000);
                uiobj.swipeRight(100);
                sleep(2000);
                uiobj.swipeUp(100);
                sleep(2000);
                uiobj.swipeDown(100);
                sleep(2000);
            }
            catch(UiObjectNotFoundException nfex)
            {
                Log.w(TAG, "testNotification: can't swipe the calendar");
                sleep(10000);
            }
        }

        Log.d(TAG, "testNotification: opening the calculator...");
        uiautomatorHelper.gotoApps(this);
        if(!uiautomatorHelper.launchApp(this, "Calculator"))
        {
            Log.e(TAG, "testNotification: error, couldn't find calendar");
        }
        else
        {
            uiautomatorHelper.keyInNumberString(uidev, "1234");
            sleep(2000);
            uiautomatorHelper.keyInArithmeticOperator(uidev, '*');
            sleep(2000);
            uiautomatorHelper.keyInNumberString(uidev, "2");
            sleep(2000);
            uiautomatorHelper.keyInArithmeticOperator(uidev, '=');
            sleep(4000);
        }

        uidev.pressHome();
        sleep(1000);

        Log.d(TAG, "testNotification: dismissing the snoozing alarm...");
        uiautomatorHelper.checkForNotificationButtonText(this, "Dismiss", true);

        Log.d(TAG, "testNotification: deleting the alarm...");
        uiautomatorHelper.deleteAlarm(this);

        Log.d(TAG, "testNotification: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, true);
    }

    public void testNotificationMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String            testName = "testNotificationMultiRes";
        UiSelector              uisel;
        UiObject                uiobj;
        String                  originalModeString;
        String                  modeString;
        List<String>            modeList;
        ListIterator<String>    modeListIter;
        int                     modesTested = 0;
        boolean                 success = true;

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testNotificationMultiRes: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        Log.d(TAG, "testNotificationMultiRes: opening HDMI settings");
        uiautomatorHelper.openSettings(this, "HDMI");

        uisel = new UiSelector().text("HDMI Connected");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "testNotificationMultiRes: HDMI not connected - aborting test");
            uiautomatorHelper.SendNotRunStatus(this, testName);
            return;
        }
        else
        {
            Log.d(TAG, "testNotificationMultiRes: HDMI connected");

            originalModeString = uiautomatorHelper.getSelectedHDMIMode(this);
            modeList = uiautomatorHelper.getHDMIModeList(this);
            modeListIter = modeList.listIterator();

            while(modeListIter.hasNext())
            {
                modeString = modeListIter.next();
                Log.d(TAG, "testNotificationMultiRes: mode " + modeString);
                if(uiautomatorHelper.selectHDMIMode(this, modeString))
                {
                    Log.d(TAG, "testNotificationMultiRes: selected mode successfully, running testNotification...");
                    sleep(3000);
                    testNotification();
                    sleep(3000);

                    if(++modesTested == 3)
                    {
                        Log.d(TAG, "testNotificationMultiRes: tested " + modesTested + " modes - that's quite enough");
                        break;
                    }

                    if(modeListIter.hasNext())
                    {
                        // return to the HDMI settings panel for the next mode
                        uiautomatorHelper.openSettings(this, "HDMI");
                        uisel = new UiSelector().text("HDMI Connected");
                        uiobj = new UiObject(uisel);
                        if(!uiobj.exists())
                        {
                            Log.d(TAG, "testNotificationMultiRes: HDMI unplugged - aborting test");
                            success = false;
                            break;
                        }
                    }
                }
                else
                {
                    Log.d(TAG, "testNotificationMultiRes: failed to select mode " + modeString);
                }
            }
            uiautomatorHelper.openSettings(this, "HDMI");
            uiautomatorHelper.selectHDMIMode(this, originalModeString);
        }

        Log.d(TAG, "testNotificationMultiRes: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }
}

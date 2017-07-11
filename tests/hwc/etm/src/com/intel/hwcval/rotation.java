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

File Name:      rotation.java

Description:    UiAutomator tests for rotated displays.

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
import com.intel.hwcval.uiautomatorHelper;

public class rotation extends UiAutomatorTestCase
{
    private static final String TAG = uiautomatorHelper.TAG;

    public void testRotation() throws RemoteException
    {
        final String testName = "testRotation";
        UiDevice    uidev = getUiDevice();
        UiSelector  uisel;
        UiObject    uiobj;

        Log.d(TAG, "testRotation: locking device");
        uiautomatorHelper.lockDevice(this);

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testRotation: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        // set the orientation and then use the device for a bit
        uidev.setOrientationLeft();
        try
        {
            uiautomatorHelper.gotoApps(this);
            if(!uiautomatorHelper.launchApp(this, "Calculator"))
            {
                Log.d(TAG, "testRotation: failed to open the calculator");
            }
            uiautomatorHelper.keyInNumberString(uidev, "2468");
            sleep(2000);
            uiautomatorHelper.keyInArithmeticOperator(uidev, '/');
            sleep(2000);
            uiautomatorHelper.keyInNumberString(uidev, "2");
            sleep(2000);
            uiautomatorHelper.keyInArithmeticOperator(uidev, '=');
            sleep(4000);
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "testRotation: calculator test failed (continuing)");
        }

        // set the orientation and then use the device for a bit
        uidev.setOrientationRight();
        try
        {
            uiautomatorHelper.gotoApps(this);
            if(!uiautomatorHelper.launchApp(this, "Calendar"))
            {
                // it's not fatal
                Log.d(TAG, "testRotation: failed to open the Calendar");
            }
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
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "testRotation: Calendar test failed (continuing)");
        }

        // set the orientation and then use the device for a bit
        uidev.setOrientationNatural();
        try
        {
            uiautomatorHelper.gotoApps(this);
            if(!uiautomatorHelper.launchApp(this, "Clock"))
            {
                // it's not fatal
                Log.d(TAG, "testRotation: failed to open the Clock");
            }
            Log.d(TAG, "testRotation: setting an alarm...");
            uiautomatorHelper.setAlarm(this, 12);
            sleep(5000);
            Log.d(TAG, "testRotation: deleting the alarm...");
            uiautomatorHelper.deleteAlarm(this);
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "testRotation: Clock test failed (continuing)");
        }

        Log.d(TAG, "testRotation: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, true);
    }

    public void testRotationMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String            testName = "testRotationMultiRes";
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
            Log.d(TAG, "testRotationMultiRes: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        Log.d(TAG, "testRotationMultiRes: opening HDMI settings");
        uiautomatorHelper.openSettings(this, "HDMI");

        uisel = new UiSelector().text("HDMI Connected");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "testRotationMultiRes: HDMI not connected - aborting test");
            uiautomatorHelper.SendNotRunStatus(this, testName);
            return;
        }
        else
        {
            Log.d(TAG, "testRotationMultiRes: HDMI connected");

            originalModeString = uiautomatorHelper.getSelectedHDMIMode(this);
            modeList = uiautomatorHelper.getHDMIModeList(this);
            modeListIter = modeList.listIterator();

            while(modeListIter.hasNext())
            {
                modeString = modeListIter.next();
                Log.d(TAG, "testRotationMultiRes: mode " + modeString);
                if(uiautomatorHelper.selectHDMIMode(this, modeString))
                {
                    Log.d(TAG, "testRotationMultiRes: selected mode successfully, running testRotation...");
                    sleep(3000);
                    testRotation();
                    sleep(3000);

                    if(modeListIter.hasNext())
                    {
                        // return to the HDMI settings panel for the next mode
                        uiautomatorHelper.openSettings(this, "HDMI");
                        uisel = new UiSelector().text("HDMI Connected");
                        uiobj = new UiObject(uisel);
                        if(!uiobj.exists())
                        {
                            Log.d(TAG, "testRotationMultiRes: HDMI unplugged - aborting test");
                            success = false;
                            break;
                        }
                    }
                }
                else
                {
                    Log.d(TAG, "testRotationMultiRes: failed to select mode " + modeString);
                }
            }
            uiautomatorHelper.openSettings(this, "HDMI");
            uiautomatorHelper.selectHDMIMode(this, originalModeString);
        }

        Log.d(TAG, "testRotationMultiRes: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }
}

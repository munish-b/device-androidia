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

File Name:      uiautomatorHelper.java

Description:    helper functions for the UiAutomator tests.

Environment:

Notes:

****************************************************************************/

package com.intel.hwcval;

import java.util.List;
import java.util.ArrayList;
import java.util.Calendar;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.util.Pair;
import android.view.KeyEvent;

import com.android.uiautomator.core.UiDevice;
import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.core.UiScrollable;
import com.android.uiautomator.core.UiCollection;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;
import android.graphics.Rect;


//******************************************************************************
//******************************************************************************
// CLASS: uiautomatorHelper
//******************************************************************************
//******************************************************************************

public class uiautomatorHelper
{
    // ************************************************
    // *************** public interface ***************
    // ************************************************

    // The log tag is public as it is expected to be shared by all our test cases
    public static final String TAG = "com.intel.hwcval.uiautomator";

    public static void SendSuccessStatus(UiAutomatorTestCase tc, String testName, boolean success)
    {
        Bundle status = new Bundle();
        int result;

        if(success)
        {
            status.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                             String.format("ETM Test(%s) completed successfully", testName));
            result = Activity.RESULT_OK;
        }
        else
        {
            status.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                             String.format("ETM Test(%s) failed", testName));
            result = Activity.RESULT_CANCELED;
        }
        tc.getAutomationSupport().sendStatus(result, status);
    }

    public static void SendNotRunStatus(UiAutomatorTestCase tc, String testName)
    {
        Bundle status = new Bundle();
        int result;

        status.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                         String.format("ETM Test(%s) not run", testName));
        result = Activity.RESULT_OK;
        tc.getAutomationSupport().sendStatus(result, status);
    }

    // FUNC  : unlockDevice
    // PREREQ: isDeviceLocked() == true
    // DESC  : unlocks the device to the Home screen
    // THROWS: RemoteException - a device error occurred
    public static void unlockDevice(UiAutomatorTestCase tc) throws RemoteException
    {
        UiObject    uiobj;
        Rect        bounds;

        Log.d(TAG, "unlockDevice: called");
        try
        {
            // the device may be asleep, prod it
            tc.getUiDevice().wakeUp();

            uiobj = GetDeviceLock(tc);
            bounds = uiobj.getBounds();
            //Log.d(TAG, String.format("unlockDevice: lock icon TLBR(%d,%d..%d,%d)",
            //           bounds.top, bounds.left, bounds.bottom, bounds.right));

            if(!tc.getUiDevice().swipe(bounds.centerX(), bounds.centerY(), bounds.centerX(), 0, 100))
            {
                Log.d(TAG, "unlockDevice: swipe failed");
            }
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "unlockDevice: failed, is device locked?");
        }

        // allow a little time for the login to complete
        tc.sleep(200);
    }

    // FUNC  : lockDevice
    // PREREQ: none
    // DESC  : locks the device (turns off the screen)
    // THROWS: RemoteException - a device error occurred
    public static void lockDevice(UiAutomatorTestCase tc) throws RemoteException
    {
        Log.d(TAG, "lockDevice: called");
        tc.getUiDevice().sleep();
    }

    // FUNC  : isDeviceLocked
    // PREREQ: none
    // DESC  : checks the device lock state
    // RETURN: true - the device is on the lock screen, or sleeping
    // THROWS: RemoteException - a device error occurred
    public static boolean isDeviceLocked(UiAutomatorTestCase tc) throws RemoteException
    {
        UiObject    uiobj;
        boolean     isLocked = true;

        try
        {
            if(tc.getUiDevice().isScreenOn())
            {
                uiobj = GetDeviceLock(tc);
                isLocked = uiobj.isEnabled();
            }
        }
        catch(UiObjectNotFoundException nfex)
        {
            isLocked = false;
        }
        Log.d(TAG, String.format("isDeviceLocked? %c", isLocked ? 'y' : 'n'));
        return isLocked;
    }

    // FUNC  : gotoApps
    // PREREQ: none
    // DESC  : goto the Apps screen
    // THROWS: UiObjectNotFoundException - the apps button wasn't found
    public static void gotoApps(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;
        boolean     tryAgain = true;

        Log.d(TAG, "gotoApps: called");
        for(;;)
        {
            try
            {
                tc.getUiDevice().pressHome();
                tc.sleep(2000);
                uisel = new UiSelector().description("Apps");
                uiobj = new UiObject(uisel);
                uiobj.waitForExists(1000);
                uiobj.clickAndWaitForNewWindow();
                return;
            }
            catch(UiObjectNotFoundException nfex)
            {
                if(tryAgain)
                {
                    // give it one more go
                    Log.d(TAG, "gotoApps: didn't find Apps, trying the home button again");
                    tryAgain = false;
                    tc.sleep(500);
                }
                else
                {
                    Log.e(TAG, "gotoApps: ERROR - never found the Apps button");
                    throw(nfex);
                }
            }
        }
    }

    // FUNC  : launchApp
    // PREREQ: must be on the Apps screen
    // DESC  : click on the specified app
    // THROWS: UiObjectNotFoundException - the specified app wasn't found
    public static boolean launchApp(UiAutomatorTestCase tc, String appName) throws UiObjectNotFoundException
    {
        UiSelector      uisel, uiselApps, uiSelTabs;
        UiScrollable    uiscroller;
        UiObject        uiobj;

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
        {
            // Earlier Versions of Android have a tab bar, make sure APPS is selected
            uiSelTabs = new UiSelector().className(android.widget.TabWidget.class.getName());
            uiselApps = new UiSelector().description("Apps");
            uisel = new UiSelector().packageName("com.android.launcher").childSelector(uiSelTabs).childSelector(uiselApps);
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }

        try
        {
            // don't select anything specific with the uisel, just get the most
            // senior scrollable thing - this will allow us to scroll through all
            // the apps
            uisel = new UiSelector().scrollable(true);
            uiscroller = new UiScrollable(uisel).setAsHorizontalList();

            uisel = new UiSelector().className(android.widget.TextView.class.getName());
            uiobj = uiscroller.getChildByText(uisel, appName);
            if(uiobj.exists())
            {
                Log.e(TAG, "launchApp - found(" + uiobj.getText() + ") content desc(" + uiobj.getContentDescription() + ")");
                uiobj.clickAndWaitForNewWindow();
            }
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "launchApp: didn't find '" + appName + "'");
            return false;
        }
        return true;
    }

    // FUNC  : openSettings
    // PREREQ: none
    // DESC  : opens the specified settings
    // THROWS: UiObjectNotFoundException - the settings string wasn't found
    public static boolean openSettings(UiAutomatorTestCase tc, String settings) throws UiObjectNotFoundException
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiScrollable    uiscroller;
        UiObject        uiobj;

        gotoApps(tc);
        if(!launchApp(tc, "Settings"))
        {
            UiObjectNotFoundException e = new UiObjectNotFoundException("Settings app not found!");
            throw(e);
        }

        try
        {
            uisel = new UiSelector().resourceId("android:id/list").scrollable(true);
            uiscroller = new UiScrollable(uisel).setAsVerticalList();
            uisel = new UiSelector().className(android.widget.TextView.class.getName());
            uiobj = uiscroller.getChildByText(uisel, settings);
            uiobj.clickAndWaitForNewWindow();
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "openSettings: didn't find '" + settings + "'");
            return false;
        }
        return true;
    }

    // FUNC  : getSelectedHDMIMode
    // PREREQ: openSettings(tc, "HDMI")
    // DESC  : gets the current HDMI mode
    // THROWS: UiObjectNotFoundException - the modes panel wasn't found
    public static String getSelectedHDMIMode(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        UiCollection    uicoll;
        String          modeString;

        // open the mode list panel
        uisel = new UiSelector().text("Modes");
        uiobj = new UiObject(uisel);
        uiobj.clickAndWaitForNewWindow();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview");
        uicoll = new UiCollection(uisel);

        uisel = new UiSelector().checked(true);
        uiobj = uicoll.getChildByDescription(uisel, "");
        modeString = uiobj.getText();
        Log.d(TAG, "getSelectedHDMIMode: mode = " + modeString);

        // return to the main HDMI panel
        uidev.pressBack();
        return modeString;
    }

    // FUNC  : getHDMIModeList
    // PREREQ: openSettings(tc, "HDMI")
    // DESC  : gets the HDMI mode list
    // THROWS: UiObjectNotFoundException - the modes panel wasn't found
    public static List<String> getHDMIModeList(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        UiScrollable    uiscroller;
        int             i;
        boolean         atEndOfList = false;
        String          modeString = new String();
        List<String>    modeList = new ArrayList<String>();

        // open the mode list panel
        uisel = new UiSelector().text("Modes");
        uiobj = new UiObject(uisel);
        uiobj.clickAndWaitForNewWindow();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview").scrollable(true);
        uiscroller = new UiScrollable(uisel).setAsVerticalList();
        uiscroller.flingToBeginning(1);
        uisel = new UiSelector().className(android.widget.CheckedTextView.class.getName());
        for(i=0;;)
        {
            try
            {
                uiobj = uiscroller.getChildByInstance(uisel, i);
                modeString = uiobj.getText();
                Log.d(TAG, "getHDMIModeList - mode: " + modeString);
                modeList.add(modeString);
                ++i;
            }
            catch(UiObjectNotFoundException e1)
            {
                // see if we can scroll it into view
                if(atEndOfList)
                {
                    break;
                }

                atEndOfList = !uiscroller.scrollForward(10);

                // reset the index
                Log.d(TAG, "getHDMIModeList - resetting the index...");
                uiscroller.scrollTextIntoView(modeString);
                for(i = 0; ; ++i)
                {
                    String modeAtIndex;
                    uiobj = uiscroller.getChildByInstance(uisel, i);
                    modeAtIndex = uiobj.getText();
                    Log.d(TAG, String.format("getHDMIModeList - index[%d]:mode(%s) == mode(%s)?", i, modeAtIndex, modeString));
                    if(modeAtIndex.equals(modeString))
                    {
                        // got it, but it's already been added, continue from the next entry
                        ++i;
                        break;
                    }
                }
            }
        }

        // return to the main HDMI panel
        uidev.pressBack();
        return modeList;
    }

    // FUNC  : selectHDMIMode
    // PREREQ: openSettings(tc, "HDMI")
    // DESC  : selects the new HDMI mode
    // THROWS: UiObjectNotFoundException - the modes panel wasn't found
    public static boolean selectHDMIMode(UiAutomatorTestCase tc, String modeString) throws UiObjectNotFoundException
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        UiScrollable    uiscroller;
        String          newModeString;

        Log.d(TAG, "selectHDMIMode - setting mode: " + modeString);

        // open the mode list panel
        uisel = new UiSelector().text("Modes");
        uiobj = new UiObject(uisel);
        uiobj.clickAndWaitForNewWindow();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview").scrollable(true);
        uiscroller = new UiScrollable(uisel).setAsVerticalList();
        uiscroller.flingToBeginning(1);

        uisel = new UiSelector().className(android.widget.CheckedTextView.class.getName());
        uiobj = uiscroller.getChildByText(uisel, modeString, true);
        uiobj.clickAndWaitForNewWindow();

        newModeString = getSelectedHDMIMode(tc);
        Log.d(TAG, "selectHDMIMode - new mode: " + newModeString);

        return newModeString.equals(modeString);
    }

    // FUNC  : setAlarm
    // PREREQ: none
    // DESC  : sets an alarm to go off in the specified number of minutes
    // THROWS: UiObjectNotFoundException - the clock app wasn't found, or its
    //              design has changed
    public static void setAlarm(UiAutomatorTestCase tc, int minutes) throws UiObjectNotFoundException
    {
        alarmHelper.openAlarmPanel(tc);
        alarmHelper.addAlarm(tc, minutes);
    }

    // FUNC  : deleteAlarm
    // PREREQ: none
    // DESC  : deletes a previously set alarm
    // THROWS: UiObjectNotFoundException - the clock app wasn't found, or its
    //              design has changed
    public static void deleteAlarm(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiSelector      uisel, uiselChild;
        UiObject        uiobj;

        alarmHelper.openAlarmPanel(tc);
        uiselChild = new UiSelector().description("Delete alarm");
        uiobj = new UiObject(uiselChild);
        uiobj.click();
    }

    // FUNC  : waitAlarm
    // PREREQ: none
    // DESC  : waits the specified number of minutes for an alarm to appear
    // RETN  : true if the alarm has gone off
    // THROWS: UiObjectNotFoundException
    public static boolean waitAlarm(UiAutomatorTestCase tc, int minutes) throws UiObjectNotFoundException
    {
        UiDevice    uidev = tc.getUiDevice();
        int         seconds = minutes * 60 + 4; // allow a little extra

        while(seconds > 0)
        {
            if(!uidev.openNotification())
            {
                Log.d(TAG, "waitAlarm - can't open notification");
            }
            else
            {
                if(checkForNotificationButtonText(tc, "Dismiss Now"))
                {
                    Log.d(TAG, "waitAlarm - sleeping...");
                    tc.sleep(3000);
                    seconds -= 3;
                }
                else
                {
                    Log.d(TAG, "waitAlarm - 'Dismiss Now' button not found. Has the alarm gone off?");

                    tc.sleep(1000);
                    if(!checkForNotificationButtonText(tc, "Snooze"))
                    {
                        Log.d(TAG, "waitAlarm - can't see it");
                        continue;
                    }
                    Log.d(TAG, "waitAlarm - detected alarm");
                    return true;
                }
            }
        }
        Log.d(TAG, "waitAlarm - returning without discovering alarm");
        return false;
    }

    // FUNC  : checkForNotificationButtonText
    // PREREQ: none
    // DESC  : opens the notification panel and searches for a button with
    //         the text string
    public static boolean checkForNotificationButtonText(UiAutomatorTestCase tc, String text)
    {
        return checkForNotificationButtonText(tc, text, false);
    }

    // FUNC  : checkForNotificationButtonText
    // PREREQ: none
    // DESC  : opens the notification panel and searches for a button with
    //         the text string. Optionally clicks on the button
    public static boolean checkForNotificationButtonText(UiAutomatorTestCase tc, String text, boolean andClickIt)
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        UiScrollable    uiscroller;
        boolean         found = false;

        if(!uidev.openNotification())
        {
            Log.d(TAG, "checkForNotificationButtonText - can't open notification");
        }
        else
        {
            if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
            {
                uisel = new UiSelector().resourceIdMatches("(.*)(notification_panel)");
            }
            else
            {
                uisel = new UiSelector().resourceIdMatches("(.*)(notification_stack_scroller)");
            }
            uisel.scrollable(true);
            uiscroller = new UiScrollable(uisel).setAsVerticalList();

            try
            {
                Log.d(TAG, "checkForNotificationButtonText - looking for '" + text + "' button...");
                uisel = new UiSelector().className(android.widget.Button.class.getName());
                uiobj = uiscroller.getChildByText(uisel, text);
                Log.d(TAG, "checkForNotificationButtonText - found '" + text + "' button...");
                found = true;

                if(andClickIt)
                {
                    uiobj.clickAndWaitForNewWindow();
                }
            }
            catch(UiObjectNotFoundException e1)
            {
                Log.d(TAG, "checkForNotificationButtonText - didn't find '" + text + "' button...");
            }
        }
        return found;
    }

    public static void keyInNumberString(UiDevice uidev, String numString)
    {
        for(int i = 0; i < numString.length(); ++i)
        {
            int code;
            int numCodes[] = {KeyEvent.KEYCODE_0,
                              KeyEvent.KEYCODE_1,
                              KeyEvent.KEYCODE_2,
                              KeyEvent.KEYCODE_3,
                              KeyEvent.KEYCODE_4,
                              KeyEvent.KEYCODE_5,
                              KeyEvent.KEYCODE_6,
                              KeyEvent.KEYCODE_7,
                              KeyEvent.KEYCODE_8,
                              KeyEvent.KEYCODE_9
                              };

            code = numString.charAt(i) - '0';
            uidev.pressKeyCode(numCodes[code]);
        }
    }

    public static void keyInArithmeticOperator(UiDevice uidev, char operator)
    {
        switch(operator)
        {
        case '+':
            uidev.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_ADD);
            break;

        case '-':
            uidev.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_SUBTRACT);
            break;

        case '*':
            uidev.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_MULTIPLY);
            break;

        case '/':
            uidev.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_DIVIDE);
            break;

        case '=':
            uidev.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_EQUALS);
            break;
        }
    }

    // ***********************************************
    // *************** private methods ***************
    // ***********************************************

    private static UiObject Kitkat_GetDeviceLock(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiSelector uisel = new UiSelector().resourceIdMatches("com.android.keyguard:id/keyguard_selector_view_frame");
        return new UiObject(uisel);
    }

    private static UiObject Lollipop_GetDeviceLock(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiSelector uisel = new UiSelector().resourceIdMatches("com.android.systemui:id/lock_icon");
        return new UiObject(uisel);
    }

    private static UiObject GetDeviceLock(UiAutomatorTestCase tc) throws UiObjectNotFoundException
    {
        UiObject uiobj;

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
        {
            uiobj = Kitkat_GetDeviceLock(tc);
            Log.d(TAG, "GetDeviceLock: got Kitkat lock");
        }
        else
        {
            uiobj = Lollipop_GetDeviceLock(tc);
            Log.d(TAG, "GetDeviceLock: got Lollipop lock");
        }
        return uiobj;
    }
}


//******************************************************************************
//******************************************************************************
//CLASS: alarmHelper
//******************************************************************************
//******************************************************************************

class alarmHelper
{
    private static final String TAG = uiautomatorHelper.TAG;


    // ***********************************************
    // ************** public interface ***************
    // ***********************************************

    // FUNC  : openAlarmPanel
    // PREREQ: none
    // DESC  : opens the alarm panel of the clock app
    // THROWS: UiObjectNotFoundException - the alarm panel wasn't found
    public static void openAlarmPanel(UiAutomatorTestCase tc)  throws UiObjectNotFoundException
    {
        UiDevice        uidev = tc.getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        boolean         goneBack = false;

        // Open the clock app
        //
        uiautomatorHelper.gotoApps(tc);
        if(!uiautomatorHelper.launchApp(tc, "Clock"))
        {
            UiObjectNotFoundException e = new UiObjectNotFoundException("Clock app not found!");
            throw(e);
        }

        // go to the alarm panel
        //
        try
        {
            uisel = new UiSelector().description("Alarm");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }
        catch(UiObjectNotFoundException nfex)
        {
            // the action bar isn't displayed because the Clock app is already running an
            // activity which doesn't show it - try hitting the back button to get to the
            // main activity
            if(goneBack)
            {
                // going back should have returned us to the main activity - give up
                throw nfex;
            }
            else
            {
                uidev.pressBack();
                goneBack = true;
            }
        }
    }

    // FUNC  : addAlarm
    // PREREQ: in the alarm panel, see openAlarmPanel()
    // DESC  : sets an alarm to go off in the specified number of minutes
    // THROWS: UiObjectNotFoundException - the function needs updating to the
    //                                     latest alarm interface
    public static void addAlarm(UiAutomatorTestCase tc, int alarmMinutesFromNow)
                       throws UiObjectNotFoundException
    {
        UiDevice            uidev = tc.getUiDevice();
        UiSelector          uisel;
        UiObject            uiobj;
        Calendar            calendar = Calendar.getInstance();
        int                 minutes, newMinutes;
        int                 hours, newHours;
        int                 ispm;
        String              ampm;
        String              time;

        // Create a UiSelector to find the Add Alarm button and simulate a user click to open the
        // add alarm activity
        //
        uisel = new UiSelector().description("Add alarm");
        uiobj = new UiObject(uisel);
        uiobj.clickAndWaitForNewWindow();

        Log.d(TAG, "addAlarm: calendar: " + calendar.toString());
        ispm = calendar.get(Calendar.AM_PM);
        if(ispm == 1)
        {
            Log.d(TAG, "addAlarm: currently PM");
            ampm = "PM";
        }
        else
        {
            Log.d(TAG, "addAlarm: currently AM");
            ampm = "AM";
        }

        hours = calendar.get(Calendar.HOUR);
        if (hours == 0)
        {
            // http://developer.android.com/reference/java/util/Calendar.html#HOUR
            // claims this is the 12-hour clock, but I've seen 00 hours!
            hours = 12;
        }
        newHours = hours;

        minutes = calendar.get(Calendar.MINUTE);
        newMinutes = (minutes + alarmMinutesFromNow) % 60;

        Log.d(TAG, "addAlarm: minutes(" + minutes + ") newMinutes(" + newMinutes + ")");

        if(newMinutes < minutes)
        {
            // add an hour
            //
            newHours = hours + 1;
            if(newHours == 13)
            {
                Log.d(TAG, "addAlarm: rolling over hours");
                newHours = 1;
            }

            if(newHours == 12)
            {
                Log.d(TAG, "addAlarm: rolling over AM/PM");
                if(ampm.equals("AM"))
                {
                    ampm = "PM";
                }
                else
                {
                    ampm = "AM";
                }
            }
        }

        Log.d(TAG, String.format("addAlarm: time: %02d:%02d%s", newHours, newMinutes, ampm));

        time = String.format("%02d%02d", newHours, newMinutes);
        uiautomatorHelper.keyInNumberString(uidev, time);
        if(ampm.equals("AM"))
        {
            uidev.pressKeyCode(KeyEvent.KEYCODE_A);
        }
        else
        {
            uidev.pressKeyCode(KeyEvent.KEYCODE_P);
        }

        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
        {
            uisel = new UiSelector().text("Done");
        }
        else
        {
            uisel = new UiSelector().text("OK");
        }
        uiobj = new UiObject(uisel);
        uiobj.clickAndWaitForNewWindow();
    }
}


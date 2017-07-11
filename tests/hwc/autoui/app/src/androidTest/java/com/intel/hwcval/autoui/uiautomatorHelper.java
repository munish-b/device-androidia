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

 File Name:     uiautomatorHelper.java

 Description:   the base class for our automation classes, the helper class
                also contains a lot of helper functions for common chores.

 Environment:

 Notes:

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiCollection;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.By;
import android.test.InstrumentationTestCase;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.util.Log;
import android.view.KeyEvent;

import java.io.BufferedInputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.NullPointerException;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.regex.Pattern;

public class uiautomatorHelper extends InstrumentationTestCase
{
    public final String TAG = "com.intel.hwcval.autoui";
    public final String SYSTEMUI_PACKAGE = "com.android.systemui";
    public final String CLOCK_PACKAGE = "com.google.android.deskclock";
    public final String CALENDAR_PACKAGE = "com.google.android.calendar";
    public final String CALCULATOR_PACKAGE = "com.android.calculator2";
    public final String MXPLAYER_PACKAGE = "com.mxtech.videoplayer.ad";
    public final String CAMERA_PACKAGE = "com.google.android.GoogleCamera";
    public final String PHOTOS_PACKAGE = "com.google.android.apps.plus";
    public final String ANDROID_SETTINGS_PACKAGE = "com.android.settings";
    public final String GFXBENCH_PACKAGE = "net.kishonti.gfxbench.gl";
    public final int mInitTimeout = 5000;
    public final int mUpdateTimeout = 2000;
    public final int mShortUpdateTimeout = 500;
    public final int mShortDelayForObserver = 1000;
    public final int mLongDelayForObserver = 5000;
    public UiDevice mDevice;

    public void setUp()
    {
        // Initialize UiDevice instance
        mDevice = UiDevice.getInstance(getInstrumentation());

        // Start from the home screen
        homeScreen();
    }

    public void SendSuccessStatus(String testName, boolean success)
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
        getInstrumentation().sendStatus(result, status);
    }

    public void SendNotRunStatus(String testName)
    {
        Bundle status = new Bundle();
        int result;

        status.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                String.format("ETM Test(%s) not run", testName));
        result = Activity.RESULT_OK;
        getInstrumentation().sendStatus(result, status);
    }

    public String shellCommand(String str)
    {
        UiAutomation uia = getInstrumentation().getUiAutomation();
        ParcelFileDescriptor pfd;
        FileDescriptor fd;
        InputStream is;
        int length;
        byte[] buf = new byte[1024];
        String outputString = new String();

        try
        {
            pfd = uia.executeShellCommand(str);
            fd = pfd.getFileDescriptor();
            is = new BufferedInputStream(new FileInputStream(fd));
            while((length = is.read(buf)) > 0)
            {
                String inputString = new String(buf);
                Log.d(TAG, String.format("shellCommand: buf[%d] '%s'", length, inputString));
                outputString += inputString;
            }

            Log.d(TAG, String.format("shellCommand: buf '%s'", outputString));
            is.close();
        }
        catch(IOException ioe)
        {
            Log.d(TAG, "shellCommand: failed to close fd");
        }

        return outputString;
    }

    public UiObject2 findUiObject2(BySelector bysel)
    {
        UiObject2 uiobj;
        try
        {
            // this should be a safe way to check for the existence of an object, but I've seen
            // NullPointerException raised from ByMatches.findMatches()
            uiobj = mDevice.findObject(bysel);
        }
        catch (NullPointerException npe)
        {
            uiobj = null;
        }
        return uiobj;
    }

    public UiObject2 waitUiObject2(BySelector bysel, int timeout)
    {
        UiObject2 uiobj;
        try
        {
            // this should be a safe way to check for the existence of an object, but I've seen
            // NullPointerException raised from ByMatches.findMatches()
            uiobj = mDevice.wait(Until.findObject(bysel), timeout);
        }
        catch (NullPointerException npe)
        {
            uiobj = null;
        }
        return uiobj;
    }

    public void homeScreen()
    {
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        PackageManager pm;
        ResolveInfo resolveInfo;
        String launcherPackage;
        boolean success = true;

        do
        {
            Log.d(TAG, "homeScreen: pressing home button...");
            mDevice.pressHome();

            // Use PackageManager to get the launcher package name
            Log.d(TAG, "homeScreen: waiting for home screen...");
            intent.addCategory(Intent.CATEGORY_HOME);
            pm = InstrumentationRegistry.getContext().getPackageManager();
            resolveInfo = pm.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
            launcherPackage = resolveInfo.activityInfo.packageName;

            try
            {
                mDevice.wait(Until.hasObject(By.pkg(launcherPackage).depth(0)), mInitTimeout);
                success = true;
            }
            catch (NullPointerException npe)
            {
                if (!success)
                {
                    Log.d(TAG, "homeScreen: failed");
                    break; // give up
                }

                success = false;
            }
        }
        while (!success);
        Log.d(TAG, "homeScreen: returning");
    }

    public void closeNotification()
    {
        UiObject2       uiobj;
        uiobj = findUiObject2(By.res(SYSTEMUI_PACKAGE, "notification_stack_scroller"));
        if(uiobj != null)
        {
            Rect bounds;

            // click outside the notification shade in order to close it
            Log.d(TAG, "closeNotification: notification is open");
            bounds = uiobj.getVisibleBounds();
            mDevice.click(bounds.left - 10, bounds.centerY());
        }
    }

    public boolean checkForNotificationButtonText(String text)
    {
        return checkForNotificationButtonText(text, false);
    }

    public boolean checkForNotificationButtonText(String text, boolean andClickIt)
    {
        UiObject        uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroller;
        boolean         found = false;

        if(!mDevice.openNotification())
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

    public boolean launchApp(String packageName) throws RemoteException
    {
        Context context = InstrumentationRegistry.getContext();
        final Intent intent = context.getPackageManager().getLaunchIntentForPackage(packageName);
        UiObject2 uiobj;
        String pkg;
        boolean success = true;

        try
        {
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK); // Clear out any previous instances
            context.startActivity(intent);

            // Wait for the app to appear
            mDevice.wait(Until.hasObject(By.pkg(packageName).depth(0)), mInitTimeout);
        }
        catch(NullPointerException npe)
        {
            Log.e(TAG, String.format("launchApp: didn't find package '%s'", packageName));
            success = false;
        }

        if(success)
        {
            // get rid of any dialogs which aren't part of normal usage
            if(packageName.equals(MXPLAYER_PACKAGE))
            {
                uiobj = waitUiObject2(By.desc("What's New?"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: MX Player(checkForWhatsNewInfoBox) - found it - clicking OK");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.text("OK"));
                    uiobj.click();
                }

                uiobj = waitUiObject2(By.textContains("Hardware acceleration"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: MX Player(checkForHwAccelerationInfoBox) - found it - clicking OK");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.text("OK"));
                    uiobj.click();
                }
            }
            else if(packageName.equals(CALENDAR_PACKAGE))
            {
                uiobj = waitUiObject2(By.desc("All views in one place"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: Calendar(allViewsInOnePlace) - found it - clicking OK");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.desc("Got it"));
                    uiobj.click();
                }
                uiobj = waitUiObject2(By.text("Couldn't sign in"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: Calendar(Couldn'tSignIn) - found it - clicking NEXT >");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.res(Pattern.compile("(.*)(auth_setup_wizard_navbar_next)")));
                    uiobj.click();
                    Log.d(TAG, "launchApp: waiting for Calendar...");
                    mDevice.wait(Until.hasObject(By.pkg(CALENDAR_PACKAGE).depth(0)), mInitTimeout);
                }
            }
            else if(packageName.equals(PHOTOS_PACKAGE))
            {
                boolean switchApps = false;

                // handle Google+
                uiobj = waitUiObject2(By.text("Signing in…"), mUpdateTimeout);
                if(uiobj != null)
                {
                    // wait for sign in to complete
                    do
                    {
                        SystemClock.sleep(mUpdateTimeout);
                        Log.d(TAG, "launchApp: Photos - Google+ trying to sign in...");
                        uiobj = findUiObject2(By.text("Signing in…"));
                    }
                    while(uiobj != null);
                }

                uiobj = waitUiObject2(By.text("No connection"), mShortUpdateTimeout);
                if(uiobj != null)
                {
                    // sign in failed - we're in the wrong activity
                    Log.d(TAG, "launchApp: Photos - Google+ failed to connect");
                    switchApps = true;
                }
                else
                {
                    uiobj = waitUiObject2(By.res("com.google.android.apps.plus:id/actionbar_avatar"), mShortUpdateTimeout);
                    if(uiobj != null)
                    {
                        // sign in succeeded - we're in the wrong activity
                        Log.d(TAG, "launchApp: Photos - Google+ connected");
                        switchApps = true;
                    }
                }

                if(switchApps)
                {
                    if(!switchToApp("Photos"))
                        return false;
                }

                uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(photos_intro_buttons)")), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: Photos(photos_intro_buttons) - found - clicking 'No thanks'");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.text("No thanks"));
                    uiobj.click();
                }

                uiobj = waitUiObject2(By.text("Setting up..."), mShortUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: Photos(Setting up...) - found - waiting for it to complete");
                    do
                    {
                        SystemClock.sleep(mUpdateTimeout);
                        Log.d(TAG, "launchApp: Photos - Photos setting up...");
                        uiobj = findUiObject2(By.text("Setting up…"));
                    }
                    while(uiobj != null);
                    uiobj = waitUiObject2(By.text("No connection"), mShortUpdateTimeout);
                    if(uiobj != null)
                    {
                        Log.d(TAG, "launchApp: Photos - Photos setting up...connection failed");
                        SystemClock.sleep(mShortDelayForObserver);
                        uiobj = findUiObject2(By.text("Cancel"));
                        uiobj.click();
                    }
                }
            }
            else if(packageName.equals(CAMERA_PACKAGE))
            {
                // look for the remember photos screen - it should only appear on the first run
                uiobj = findUiObject2(By.textStartsWith("Remember photo location"));
                if (uiobj != null)
                {
                    uiobj = findUiObject2(By.checked(true));
                    if (uiobj != null)
                    {
                        Log.d(TAG, "launchApp: Camera - unchecking the tag photos option");
                        SystemClock.sleep(mShortDelayForObserver);
                        uiobj.click();
                    }
                    uiobj = findUiObject2(By.text("NEXT"));
                    uiobj.click();
                }
            }
            else if(packageName.equals(GFXBENCH_PACKAGE))
            {
                Log.d(TAG, "launchApp: GFXBench - clicking through preamble...");
                SystemClock.sleep(mUpdateTimeout);

                uiobj = waitUiObject2(By.text("General License"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: GFXBench - closing general license");
                    uiobj = findUiObject2(By.text("Accept"));
                    uiobj.click();
                }
                uiobj = waitUiObject2(By.text("Information"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: GFXBench - closing information");
                    uiobj = findUiObject2(By.text("OK"));
                    uiobj.click();
                }

                uiobj = waitUiObject2(By.res("net.kishonti.gfxbench.gl:id/main_circleControl"), mUpdateTimeout);
                if(uiobj != null)
                {
                    Log.d(TAG, "launchApp: GFXBench - on home screen");
                }
                else
                {
                    uiobj = waitUiObject2(By.res("net.kishonti.gfxbench.gl:id/results_navbar"), mUpdateTimeout);
                    if(uiobj != null)
                    {
                        Log.e(TAG, "launchApp: GFXBench - on results screen");
                        mDevice.pressBack();
                    }
                    else
                    {
                        Log.d(TAG, "launchApp: GFXBench - not on home screen or results screen, checking for data synchronisation");

                        // this is the big one - the benchmark can take quite a while setting up
                        // before it's even ready to ask the user to agree to data synchronisation -
                        // allow 1 minute
                        Log.d(TAG, "launchApp: GFXBench - waiting for the data synchronisation dialog...");
                        uiobj = waitUiObject2(By.text("Data synchronization"), 60 * 1000);
                        if(uiobj != null)
                        {
                            Log.d(TAG, "launchApp: GFXBench - beginning data synchronisation");
                            uiobj = findUiObject2(By.text("OK"));
                            uiobj.click();

                            // this can take a long time - how long should I wait?
                            uiobj = waitUiObject2(
                                    By.res("net.kishonti.gfxbench.gl:id/main_circleControl"),
                                    4 * 60 * 1000);
                            if(uiobj == null)
                            {
                                Log.e(TAG, "launchApp: GFXBench - can't find home screen");
                            }
                        }
                    }
                }
            }
        }

        pkg = mDevice.getCurrentPackageName();
        if(pkg.equals(packageName))
        {
            Log.d(TAG, String.format("launchApp: current package '%s' - success", pkg));
        }
        else
        {
            Log.e(TAG, String.format("launchApp: current package '%s' not '%s' - failed", pkg, packageName));
            success = false;
        }

        return success;
    }

    public boolean launchApp(String packageName, String appName) throws RemoteException, UiObjectNotFoundException
    {
        UiObject2 uiobj;
        UiObject uiobj1;
        UiSelector uisel;
        UiScrollable uiscroll;

        Log.d(TAG, "launchApp: opening Apps");
        mDevice.pressHome();
        try
        {
            uiobj = waitUiObject2(By.desc("Apps"), mUpdateTimeout);
            uiobj.click();

            uisel = new UiSelector().resourceIdMatches("(.*)(apps_customize_pane_content)");
            uiscroll = new UiScrollable(uisel).setAsHorizontalList();
            uisel = new UiSelector().className(android.widget.TextView.class.getName());

            Log.d(TAG, String.format("launchApp: scrolling '%s' into view", appName));
            uiscroll.scrollTextIntoView(appName);
            SystemClock.sleep(mShortUpdateTimeout);

            Log.d(TAG, String.format("launchApp: clicking on '%s'", appName));
            uiobj1 = uiscroll.getChildByText(uisel, appName, true);
            uiobj1.clickAndWaitForNewWindow();
        }
        catch(UiObjectNotFoundException | NullPointerException ex)
        {
            Log.d(TAG, "launchApp: looking in recent apps list...");
            switchToApp(appName);
        }
        return true;
    }

    public boolean switchToApp(String appName) throws RemoteException
    {
        UiSelector      uisel;
        UiScrollable    uiscroll;
        UiObject        uiobj;

        Log.d(TAG, String.format("switchToApp(%s) - called", appName));
        mDevice.pressRecentApps();

        try
        {
            uisel = new UiSelector();
            uisel.scrollable(true);
            uiscroll = new UiScrollable(uisel);
            uiscroll.flingToBeginning(100);
            uisel = new UiSelector().resourceId("com.android.systemui:id/activity_description");
            uisel.childSelector(new UiSelector().className(android.widget.TextView.class.getName()));
            uiobj = uiscroll.getChildByText(uisel, appName, true);
            uiobj.click();
        }
        catch(UiObjectNotFoundException nfe)
        {
            Log.e(TAG, String.format("switchToApp(%s) - not found", appName));
            return false;
        }
        return true;
    }

    public boolean openAndroidSettings(String setting) throws RemoteException, UiObjectNotFoundException
    {
        BySelector bySettingsSelector = By.res("android:id/action_bar").hasChild(By.text("Settings"));
        UiObject2 uiobj;
        UiSelector uisel;
        UiScrollable uiscroller;
        UiObject uiobj1;

        if(!launchApp(ANDROID_SETTINGS_PACKAGE))
        {
            Log.d(TAG, "openAndroidSettings: failed to open the settings panel, trying the 'all apps' panel");
            if(!launchApp(ANDROID_SETTINGS_PACKAGE, "Settings"))
            {
                Log.d(TAG, "openAndroidSettings: failed to open the settings panel");
                return false;
            }
        }

        uiobj = waitUiObject2(bySettingsSelector, mShortUpdateTimeout);
        if(uiobj == null)
        {
            Log.d(TAG, "openAndroidSettings: didn't find Settings - hitting back");
            uiobj = waitUiObject2(bySettingsSelector, mShortUpdateTimeout);
            if(uiobj == null)
            {
                Log.d(TAG, "openAndroidSettings: didn't find Settings - hitting back again");
                uiobj = waitUiObject2(bySettingsSelector, mShortUpdateTimeout);
                if(uiobj == null)
                {
                    Log.e(TAG, "openAndroidSettings: can't find main settings panel, giving up");
                    return false;
                }
            }
        }

        try
        {
            uisel = new UiSelector().resourceId("com.android.settings:id/dashboard").scrollable(true);
            uiscroller = new UiScrollable(uisel).setAsVerticalList();
            uisel = new UiSelector().className(android.widget.TextView.class.getName());
            uiobj1 = uiscroller.getChildByText(uisel, setting);
            uiobj1.clickAndWaitForNewWindow();
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "openAndroidSettings: didn't find '" + setting + "'");
            return false;
        }
        return true;
    }

    public boolean IsHDMIConnected()
    {
        UiObject2 uiobj;

        uiobj = waitUiObject2(By.text("HDMI Connected"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "IsHDMIConnected: true");
            return true;
        }
        Log.d(TAG, "IsHDMIConnected: false");
        return false;
    }

    public String getSelectedHDMIMode() throws UiObjectNotFoundException, RemoteException
    {
        UiObject2       uiobj;
        UiSelector      uisel;
        UiObject        uiobj1;
        UiCollection    uicoll;
        String          modeString;

        if(!openAndroidSettings("HDMI") || !IsHDMIConnected())
            return null;

        // open the mode list panel
        uiobj = findUiObject2(By.text("Modes"));
        uiobj.click();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview");
        uicoll = new UiCollection(uisel);

        uisel = new UiSelector().checked(true);
        uiobj1 = uicoll.getChildByDescription(uisel, "");
        modeString = uiobj1.getText();
        Log.d(TAG, "getSelectedHDMIMode: mode = " + modeString);

        // return to the main HDMI panel
        mDevice.pressBack();
        return modeString;
    }

    public boolean selectHDMIMode(String modeString) throws UiObjectNotFoundException, RemoteException
    {
        UiObject2       uiobj;
        UiSelector      uisel;
        UiObject        uiobj1;
        UiScrollable    uiscroller;
        String          newModeString;

        Log.d(TAG, "selectHDMIMode - setting mode: " + modeString);

        if(!openAndroidSettings("HDMI") || !IsHDMIConnected())
            return false;

        // open the mode list panel
        uiobj = findUiObject2(By.text("Modes"));
        uiobj.click();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview").scrollable(true);
        uiscroller = new UiScrollable(uisel).setAsVerticalList();
        uiscroller.flingToBeginning(1);

        uisel = new UiSelector().className(android.widget.CheckedTextView.class.getName());
        uiobj1 = uiscroller.getChildByText(uisel, modeString, true);
        uiobj1.clickAndWaitForNewWindow();

        newModeString = getSelectedHDMIMode();
        Log.d(TAG, "selectHDMIMode - new mode: " + newModeString);

        return newModeString.equals(modeString);
    }

    public List<String> getHDMIModeList() throws UiObjectNotFoundException, RemoteException
    {
        UiObject2       uiobj;
        UiSelector      uisel;
        UiObject        uiobj1;
        UiScrollable    uiscroller;
        int             i;
        boolean         atEndOfList = false;
        String          modeString = "";
        List<String>    modeList = new ArrayList<String>();

        if(!openAndroidSettings("HDMI") || !IsHDMIConnected())
            return modeList;

        // open the mode list panel
        uiobj = findUiObject2(By.text("Modes"));
        uiobj.click();

        uisel = new UiSelector().resourceId("android:id/select_dialog_listview").scrollable(true);
        uiscroller = new UiScrollable(uisel).setAsVerticalList();
        uiscroller.flingToBeginning(1);
        uisel = new UiSelector().className(android.widget.CheckedTextView.class.getName());
        for(i=0;;)
        {
            try
            {
                uiobj1 = uiscroller.getChildByInstance(uisel, i);
                modeString = uiobj1.getText();
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
                    uiobj1 = uiscroller.getChildByInstance(uisel, i);
                    modeAtIndex = uiobj1.getText();
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
        mDevice.pressBack();
        return modeList;
    }

    public void openAlarmPanel() throws UiObjectNotFoundException, RemoteException
    {
        UiObject2 uiobj;
        boolean retrying = false;
        boolean success = true;

        do
        {
            // Open the clock app
            if(!launchApp(CLOCK_PACKAGE))
            {
                Log.e(TAG, "testRotation: failed to open Clock - trying the 'all apps' panel");
                if(!launchApp(CLOCK_PACKAGE, "Clock"))
                {
                    throw (new UiObjectNotFoundException("Clock app not found!"));
                }
            }

            try
            {
                uiobj = findUiObject2(By.desc("Alarm"));
                uiobj.click();
                if(!mDevice.waitForWindowUpdate("", 2 * mInitTimeout)) // this can take a while
                {
                    Log.e(TAG, "openAlarmPanel - timed out waiting for window update");
                    success = false;
                    retrying = !retrying;
                    if(retrying)
                    {
                        Log.d(TAG, "openAlarmPanel - trying the back button");
                        mDevice.pressBack();
                    }
                }
            }
            catch(NullPointerException npe)
            {
                Log.e(TAG,"openAlarmPanel: didn't find Alarm panel");
                throw (new UiObjectNotFoundException("Alarm panel not found!"));
            }
        }
        while(!success && retrying);
    }

    public void addAlarm(int alarmMinutesFromNow) throws UiObjectNotFoundException
    {
        UiObject2           uiobj;
        Calendar            calendar = Calendar.getInstance();
        int                 minutes, newMinutes;
        int                 hours, newHours;
        int                 ispm;
        String              ampm;
        String              time;

        // Create a UiSelector to find the Add Alarm button and simulate a user click to open the
        // add alarm activity
        //
        try
        {
            uiobj = findUiObject2(By.desc("Add alarm")); // field in the UI Automator viewer
            uiobj.click();
            if(!mDevice.waitForWindowUpdate("", mUpdateTimeout))
                Log.e(TAG, "addAlarm - timed out waiting for window update");
        }
        catch(NullPointerException npe)
        {
            // in this case alarm working is a requirement
            Log.e(TAG, "addAlarm: didn't find Add alarm button");
            throw(new UiObjectNotFoundException("Add alarm button not found!"));
        }

        Log.d(TAG, "addAlarm: calendar: " + calendar.toString());

        // using calendar app instead than Androind interface because much easier
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

        // http://developer.android.com/reference/java/util/Calendar.html#HOUR
        // claims this is the 12-hour clock, but I've seen 00 hours!
        hours = calendar.get(Calendar.HOUR);
        if (hours == 0)
            hours = 12;
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
                    ampm = "PM";
                else
                    ampm = "AM";
            }
        }

        Log.d(TAG, String.format("addAlarm: time: %02d:%02d%s", newHours, newMinutes, ampm));

        time = String.format("%02d%02d", newHours, newMinutes);
        keyInNumberString(time);
        if(ampm.equals("AM"))
            mDevice.pressKeyCode(KeyEvent.KEYCODE_A); // types in the new time
        else
            mDevice.pressKeyCode(KeyEvent.KEYCODE_P);

        try
        {
            if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
                uiobj = findUiObject2(By.text("Done"));
            else
                uiobj = findUiObject2(By.text("OK"));

            uiobj.click();
            if(!mDevice.waitForWindowUpdate("", mUpdateTimeout))
                Log.e(TAG, "addAlarm - timed out waiting for window update");
        }
        catch(NullPointerException npe)
        {
            Log.e(TAG, "addAlarm: didn't find set alarm button");
            throw(new UiObjectNotFoundException("Set alarm button not found!"));
        }

        // turn off the annoying vibrate
        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(vibrate_onoff)")), mShortUpdateTimeout);
        if(uiobj != null)
        {
            if(uiobj.isChecked())
                uiobj.click();
        }
    }

    public boolean waitAlarm(int minutes)
    {
        int milliseconds = (minutes * 60 + 4) * 1000;
        long startTimeMilliseconds;
        long currentTimeMilliseconds;

        currentTimeMilliseconds = startTimeMilliseconds = SystemClock.elapsedRealtime();
        while((currentTimeMilliseconds - startTimeMilliseconds) <  milliseconds)
        {
            if(!mDevice.openNotification())
            {
                Log.d(TAG, "waitAlarm - can't open notification");
            }
            else
            {
                if(checkForNotificationButtonText("Dismiss Now"))
                {
                    Log.d(TAG, "waitAlarm - alarm not gone off yet sleeping...");
                    SystemClock.sleep(3000);
                }
                else
                {
                    Log.d(TAG, "waitAlarm - 'Dismiss Now' button not found. Has the alarm gone off?");

                    SystemClock.sleep(1000);
                    if(checkForNotificationButtonText("Snooze"))
                    {
                        Log.d(TAG, "waitAlarm - detected alarm");
                        return true;
                    }
                    else
                    {
                        // if the alarm notification disappears when it goes off, all we're left with
                        // is the toast notification which we can't interact with. Wait for the waitAlarm
                        // to time-out
                        Log.w(TAG, "waitAlarm - looks like the alarm is sounding, but we can't snooze it. Waiting for time-out");
                    }
                }
            }
            currentTimeMilliseconds = SystemClock.elapsedRealtime();
        }
        Log.d(TAG, "waitAlarm - returning without discovering alarm");
        return false;
    }

    public void deleteAlarm()
    {
        BySelector bysel = By.res(Pattern.compile("(.*)(delete)"));
        UiObject2 uiobj;

        uiobj = findUiObject2(bysel);
        if(uiobj == null)
        {
            Log.d(TAG, "waitAlarm - didn't see the delete buttton, clicking the down arrow");
            uiobj = findUiObject2(By.res("com.android.deskclock:id/arrow"));
            uiobj.click();
            Log.d(TAG, "waitAlarm - looking for the delete button again");
            uiobj = waitUiObject2(bysel, mShortUpdateTimeout);
        }
        uiobj.click();
    }

    public void keyInNumberString(String numString)
    {
        for(int i = 0; i < numString.length(); ++i)
        {
            int code;
            int numCodes[] = {
                KeyEvent.KEYCODE_0,
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
            mDevice.pressKeyCode(numCodes[code]);
        }
    }

    public void keyInArithmeticOperator(char operator)
    {
        switch(operator)
        {
        case '+':
            mDevice.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_ADD);
            break;

        case '-':
            mDevice.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_SUBTRACT);
            break;

        case '*':
            mDevice.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_MULTIPLY);
            break;

        case '/':
            mDevice.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_DIVIDE);
            break;

        case '=':
            mDevice.pressKeyCode(KeyEvent.KEYCODE_NUMPAD_EQUALS);
            break;
        }
    }

    public void tapScreen()
    {
        Log.v(TAG, "tapScreen: tapping the centre of the display");
        mDevice.click(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2);
    }

    public void HandleResolverDialog(String option, String action) throws UiObjectNotFoundException
    {
        UiObject2    uiobj;

        Log.d(TAG, "HandleResolverDialog: checking for resolver dialog");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = waitUiObject2(By.clazz(Pattern.compile("(.*)(ResolverDrawerLayout)")), mInitTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "HandleResolverDialog: found it - selecting '" + option + "'");
            uiobj = findUiObject2(By.text(option));
            uiobj.click();
            SystemClock.sleep(mShortUpdateTimeout);

            Log.d(TAG, "HandleResolverDialog: selecting '" + action + "'");
            uiobj = findUiObject2(By.text(action));
            uiobj.click();
        }
    }

    public boolean launchVideoMxPlayer(String video) throws UiObjectNotFoundException, NullPointerException
    {
        BySelector      byselFolders = By.text("Folders");
        BySelector      byselNavigateUp = By.desc("Navigate up");
        UiObject2       uiobj;
        UiSelector      uisel;
        UiObject        uiCollObj = null;
        UiCollection uicoll;

        uiobj = waitUiObject2(byselFolders, mShortUpdateTimeout);
        if(uiobj == null)
        {
            Log.d(TAG, "launchVideoMxPlayer: looking for the Folders directory...");
            uiobj = findUiObject2(byselNavigateUp);
            while(uiobj != null)
            {
                uiobj.click();
                uiobj = waitUiObject2(byselFolders, mShortUpdateTimeout);
                if(uiobj == null)
                    uiobj = waitUiObject2(byselNavigateUp, mShortUpdateTimeout);
            }
        }
        if(uiobj == null)
        {
            Log.d(TAG, "launchVideoMxPlayer: failed to find the Download folder");
            return false;
        }

        Log.d(TAG, "launchVideoMxPlayer: rescanning...");
        uiobj = findUiObject2(By.desc("Folder scan"));
        uiobj.click();

        // use textContains, because MX Player can append 'New' to the folder name
        Log.d(TAG, "launchVideoMxPlayer: opening Download folder...");
        uiobj = waitUiObject2(By.textContains("Download"), mInitTimeout);
        if(uiobj == null)
        {
            // should have adb pushed a video to /sdcard/Download/
            Log.e(TAG, "launchVideoMxPlayer: error, there's no download folder - valetm error");
            return false;
        }
        uiobj.click();
        mDevice.waitForWindowUpdate(MXPLAYER_PACKAGE, mShortUpdateTimeout);

        uiobj = waitUiObject2(By.textContains(video), mShortUpdateTimeout);
        if(uiobj != null)
        {
            // play it
            Log.d(TAG, "launchVideoMxPlayer: playing the video");
            uiobj.click();
        }
        else
        {
            Log.e(TAG, "launchVideoMxPlayer: error, can't find video '" + video + "'");

            uisel = new UiSelector().resourceId("android:id/list");
            uicoll = new UiCollection(uisel);

            // just pick the first item
            uisel = new UiSelector().className(android.widget.RelativeLayout.class.getName());
            uiCollObj = uicoll.getChildByInstance(uisel, 0);
            uisel = new UiSelector().className(android.widget.ImageView.class.getName());
            uiCollObj = uiCollObj.getChild(uisel);
            if(uiCollObj != null && !uiCollObj.waitForExists(5000))
            {
                // should have adb pushed a video to /sdcard/download/
                Log.e(TAG, "launchVideoMxPlayer: error, are no videos listed - valetm error");
                return false;
            }
            // play it
            Log.d(TAG, "launchVideoMxPlayer: playing the video");
            uiCollObj.clickAndWaitForNewWindow();
        }

        // if the video has been run previously but didn't complete, we'll get a dialog asking us
        // whether to start over or resume - always restart the video
        uiobj = waitUiObject2(By.text("Start over"), mUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "launchVideoMxPlayer: found 'resume play' dialog - clicking START OVER");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj.click();
        }
        return true;
    }

    public void IsAccelerometerRotationEnabled()
    {
        UiAutomation uia = getInstrumentation().getUiAutomation();
        ParcelFileDescriptor pfd;
        FileDescriptor fd;
        InputStream is;
        byte[] buf = new byte[256];

        // TODO: this isn't working yet. Once it is, return the enabled state as a boolean
        try
        {
            pfd = uia.executeShellCommand("content query --uri content://settings/system --projection value --where \"name='accelerometer_rotation'\"");
            fd = pfd.getFileDescriptor();
            is = new BufferedInputStream(new FileInputStream(fd));
            is.read(buf, 0, buf.length);
            Log.d(TAG, String.format("IsAccelerometerRotationEnabled: buf '%s'", new String(buf)));
            is.close();
        }
        catch(IOException ioe)
        {
            Log.d(TAG, "IsAccelerometerRotationEnabled: failed to close fd");
        }
    }

    public void EnableAccelerometerRotation()
    {
        UiAutomation uia = getInstrumentation().getUiAutomation();
        ParcelFileDescriptor pfd;
        int fd;
        FileInputStream results;

        Log.d(TAG, "EnableAccelerometerRotation: called");
        try
        {
            pfd = uia.executeShellCommand("content insert --uri content://settings/system --bind name:s:accelerometer_rotation --bind value:i:1");
            pfd.close();
        }
        catch(IOException ioe)
        {
            Log.d(TAG, "EnableAccelerometerRotation: failed to close fd");
        }
    }

    public void DisableAccelerometerRotation()
    {
        UiAutomation uia = getInstrumentation().getUiAutomation();
        ParcelFileDescriptor pfd;
        int fd;
        FileInputStream results;

        Log.d(TAG, "DisableAccelerometerRotation: called");
        try
        {
            pfd = uia.executeShellCommand("content insert --uri content://settings/system --bind name:s:accelerometer_rotation --bind value:i:0");
            pfd.close();
        }
        catch(IOException ioe)
        {
            Log.d(TAG, "DisableAccelerometerRotation: failed to close fd");
        }
    }
}

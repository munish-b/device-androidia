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

File Name:      mxplayerApp.java

Description:    UiAutomator tests for the MX Player app.

Environment:

Notes:

****************************************************************************/

package com.intel.hwcval;

import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;

import android.os.RemoteException;
import android.util.Log;
import android.view.KeyEvent;

import android.widget.RelativeLayout;

import com.android.uiautomator.core.UiDevice;
import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.core.UiCollection;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;
import com.intel.hwcval.uiautomatorHelper;
import android.graphics.Rect;


public class mxplayerApp extends UiAutomatorTestCase
{
    private static final String TAG = uiautomatorHelper.TAG;

    public void testMXPlayer() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testMXPlayer";
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel, uiselChild;
        UiObject        uiobj;
        Rect            bounds;
        boolean         success;

        success = launchVideo();
        if(success)
        {
            // check for resume play dialog
            sleep(1000);
            checkForResumePlay();

            // wait for it to go into extended mode (if there's a second display connected)
            sleep(7500);

            // tap the screen to display the controls, then pause the video,
            // then tap the screen again
            Log.d(TAG, "testMXPlayer: pausing the video");
            uidev.click(uidev.getDisplayWidth() / 2, uidev.getDisplayHeight() / 2);
            sleep(500);
            togglePauseResume();
            sleep(500);
            uidev.click(uidev.getDisplayWidth() / 2, uidev.getDisplayHeight() / 2);
            sleep(500);

            // use the progress bar
            Log.d(TAG, "testMXPlayer: dragging the progress bar");
            success = showProgressBar();
            if(success)
            {
                uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/progressBar");
                uiobj = new UiObject(uisel);
                bounds = uiobj.getBounds();

                Log.d(TAG, "testMXPlayer: advancing progress...");
                uidev.drag(bounds.left + 200, bounds.centerY(), bounds.right - 200, bounds.centerY(), 500);
                togglePauseResume(); // continue playing
                sleep(4000);

                Log.d(TAG, "testMXPlayer: pausing...");
                togglePauseResume();
                success = showProgressBar();
                if(success)
                {
                    Log.d(TAG, "testMXPlayer: reversing progress...");
                    uidev.drag(bounds.right - 200, bounds.centerY(), bounds.left + 200, bounds.centerY(), 500);
                    togglePauseResume();
                    sleep(10000);
                }

                // exit the video
                uidev.pressBack();
            }
        }
        sleep(1000);
        uidev.pressHome();
        Log.d(TAG, "testMXPlayer: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    public void testMXPlayerMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testMXPlayerMultiRes";
        UiSelector      uisel;
        UiObject        uiobj;
        String          originalModeString;
        List<String>    modeList = new ArrayList<String>(Arrays.asList("1280*720P@60Hz", "800*600P@60Hz"));
        boolean         success = true;

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testMXPlayerMultiRes: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        Log.d(TAG, "testMXPlayerMultiRes: opening HDMI settings");
        uiautomatorHelper.openSettings(this, "HDMI");

        uisel = new UiSelector().text("HDMI Connected");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "testMXPlayerMultiRes: HDMI not connected - aborting test");
            uiautomatorHelper.SendNotRunStatus(this, testName);
            return;
        }
        else
        {
            Log.d(TAG, "testMXPlayerMultiRes: HDMI connected");

            originalModeString = uiautomatorHelper.getSelectedHDMIMode(this);
            Log.d(TAG, "testMXPlayerMultiRes: current mode " + originalModeString);

            for(int i = 0; i < modeList.size(); ++i)
            {
                try
                {
                    if(uiautomatorHelper.selectHDMIMode(this, modeList.get(i)))
                    {
                        Log.d(TAG, "testMXPlayerMultiRes: selected mode successfully, running testMXPlayer...");
                        sleep(3000);
                        testMXPlayer();
                        sleep(3000);

                        // return to the HDMI settings panel for the next mode
                        uiautomatorHelper.openSettings(this, "HDMI");
                        uisel = new UiSelector().text("HDMI Connected");
                        uiobj = new UiObject(uisel);
                        if(!uiobj.exists())
                        {
                            Log.d(TAG, "testMXPlayerMultiRes: HDMI unplugged - aborting test");
                            success = false;
                            break;
                        }
                    }
                }
                catch(UiObjectNotFoundException nfex)
                {
                    Log.w(TAG, "testMXPlayerMultiRes: mode doesn't exist - skipping (not an error)");
                }
            }
            uiautomatorHelper.openSettings(this, "HDMI");
            uiautomatorHelper.selectHDMIMode(this, originalModeString);
        }

        Log.d(TAG, "testMXPlayerMultiRes: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    public void testMXPlayerLock() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testMXPlayerLock";
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        int             screenWidth, screenHeight;
        int             attempts;
        boolean         success;

        screenWidth = getUiDevice().getDisplayWidth();
        screenHeight = getUiDevice().getDisplayHeight();
        success = launchVideo();
        if(success)
        {
            // check for resume play dialog
            sleep(1000);
            checkForResumePlay();

            // allow the video to start playing
            sleep(2000);

            success = lockVideo();
        }

        if(success)
        {
            sleep(10000);

            Log.d(TAG, "testMXPlayerLock: testing lock...");
            attempts = 10;
            do
            {
                uisel = new UiSelector().description("More options");
                uiobj = new UiObject(uisel);
                success = uiobj.exists();
                if(!success)
                {
                    Log.d(TAG, "testMXPlayerLock: can't see the menu");

                    // tap the screen to bring up the menu
                    Log.d(TAG, "testMXPlayerLock: tapping the screen and looking for the menu...");
                    getUiDevice().click(screenWidth / 2, screenHeight / 2);
                    sleep(100);
                }
            }
            while(!success && --attempts > 0);
            if(success)
            {
                Log.e(TAG, "testMXPlayerLock: found the menu - obviously not locked!");
                success = false;
            }
            else
            {
                Log.d(TAG, "testMXPlayerLock: lock was successful");
            }

            Log.d(TAG, "testMXPlayerLock: performing unlock...");
            getUiDevice().click(screenWidth - 50, 50);
            sleep(200);
            getUiDevice().click(screenWidth - 50, screenHeight - 50);
            sleep(200);
            getUiDevice().click(50, screenHeight - 50);
            sleep(200);
            getUiDevice().click(50, 50);
            sleep(200);

            Log.d(TAG, "testMXPlayerLock: testing unlock...");
            attempts = 10;
            do
            {
                uisel = new UiSelector().description("More options");
                uiobj = new UiObject(uisel);
                success = uiobj.exists();
                if(!success)
                {
                    Log.d(TAG, "testMXPlayerLock: can't see the menu");

                    // tap the screen to bring up the menu
                    Log.d(TAG, "testMXPlayerLock: tapping the screen and looking for the menu...");
                    getUiDevice().click(screenWidth / 2, screenHeight / 2);
                    sleep(100);
                }
            }
            while(!success && --attempts > 0);
            if(success)
            {
                Log.d(TAG, "testMXPlayerLock: unlock was successful");
            }
            else
            {
                Log.e(TAG, "testMXPlayerLock: didn't find the menu - still locked - hitting the power button to unlock the app");
                uiautomatorHelper.lockDevice(this);
                sleep(2000);
                if(uiautomatorHelper.isDeviceLocked(this))
                {
                    uiautomatorHelper.unlockDevice(this);
                }
            }
        }

        if(isPaused())
        {
            togglePauseResume();
        }
        Log.d(TAG, "testMXPlayerLock: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    public void testMXPlayerLockMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testMXPlayerLockMultiRes";
        UiSelector      uisel;
        UiObject        uiobj;
        String          originalModeString;
        List<String>    modeList = new ArrayList<String>(Arrays.asList("1280*720P@60Hz", "800*600P@60Hz"));
        boolean         success = true;

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testMXPlayerLockMultiRes: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        Log.d(TAG, "testMXPlayerLockMultiRes: opening HDMI settings");
        uiautomatorHelper.openSettings(this, "HDMI");

        uisel = new UiSelector().text("HDMI Connected");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "testMXPlayerLockMultiRes: HDMI not connected - aborting test");
            uiautomatorHelper.SendNotRunStatus(this, testName);
            return;
        }
        else
        {
            Log.d(TAG, "testMXPlayerLockMultiRes: HDMI connected");

            originalModeString = uiautomatorHelper.getSelectedHDMIMode(this);
            Log.d(TAG, "testMXPlayerLockMultiRes: current mode " + originalModeString);

            for(int i = 0; i < modeList.size(); ++i)
            {
                try
                {
                    if(uiautomatorHelper.selectHDMIMode(this, modeList.get(i)))
                    {
                        Log.d(TAG, "testMXPlayerLockMultiRes: selected mode successfully, running testMXPlayer...");
                        sleep(3000);
                        testMXPlayerLock();
                        sleep(3000);

                        // return to the HDMI settings panel for the next mode
                        uiautomatorHelper.openSettings(this, "HDMI");
                        uisel = new UiSelector().text("HDMI Connected");
                        uiobj = new UiObject(uisel);
                        if(!uiobj.exists())
                        {
                            Log.d(TAG, "testMXPlayerLockMultiRes: HDMI unplugged - aborting test");
                            success = false;
                            break;
                        }
                    }
                }
                catch(UiObjectNotFoundException nfex)
                {
                    Log.w(TAG, "testMXPlayerLockMultiRes: mode doesn't exist - skipping (not an error)");
                }
            }
            uiautomatorHelper.openSettings(this, "HDMI");
            uiautomatorHelper.selectHDMIMode(this, originalModeString);
        }

        Log.d(TAG, "testMXPlayerLockMultiRes: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    // ***********************************************
    // *************** private methods ***************
    // ***********************************************

    private boolean launchVideo() throws RemoteException, UiObjectNotFoundException
    {
        UiDevice        uidev = getUiDevice();
        String          video;
        UiSelector      uisel, uiselChild;
        UiObject        uiobj;
        UiCollection    uicoll;
        boolean         success = true;

        video = getParams().getString("video");

        Log.d(TAG, "launchVideo: locking device");
        uiautomatorHelper.lockDevice(this);

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "launchVideo: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        uiautomatorHelper.gotoApps(this);
        if(!uiautomatorHelper.launchApp(this, "MX Player"))
        {
            Log.e(TAG, "launchVideo: error, couldn't find MX Player");
            success = false;
        }
        else
        {
            sleep(1000);
            checkForAd();
            sleep(1000);
            checkForHwAccelerationInfoBox();
            sleep(1000);

            uisel = new UiSelector().text("Folders");
            uiobj = new UiObject(uisel);
            if(!uiobj.waitForExists(5000))
            {
                boolean hitBackButton = false;

                Log.d(TAG, "launchVideo: not in the Folders panel - trying to navigate to it");
                while(success)
                {
                    uisel = new UiSelector().description("Navigate up");
                    uiobj = new UiObject(uisel);
                    while(uiobj.exists())
                    {
                        uiobj.clickAndWaitForNewWindow();
                        uiobj = new UiObject(uisel);
                    }

                    uisel = new UiSelector().text("Folders");
                    uiobj = new UiObject(uisel);
                    if(!uiobj.waitForExists(500))
                    {
                        if(!hitBackButton)
                        {
                            // last attempt - try the back button
                            Log.d(TAG, "launchVideo: may be in the video, trying the back button");
                            uidev.pressBack();
                            hitBackButton = true;
                        }
                        else
                        {
                            Log.e(TAG, "launchVideo: error - can't find Folders panel");
                            success = false;
                        }
                    }

                    uisel = new UiSelector().text("Folders");
                    uiobj = new UiObject(uisel);
                    if(uiobj.exists())
                    {
                        break;
                    }
                }
            }
        }
        if(success)
        {
            // first, hit refresh
            uisel = new UiSelector().description("Folder scan");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();

            // now click the Download folder
            uisel = new UiSelector().textContains("Download");
            uiobj = new UiObject(uisel);
            if(!uiobj.waitForExists(5000))
            {
                // should have adb pushed a video to /sdcard/Download/
                Log.e(TAG, "launchVideo: error, there's no download folder - valetm error");
                success = false;
            }
            else
            {
                uiobj.clickAndWaitForNewWindow();
                uisel = new UiSelector().resourceId("android:id/list");
                uicoll = new UiCollection(uisel);

                if(video != null)
                {
                    uisel = new UiSelector().className(android.widget.TextView.class.getName());
                    uiobj = uicoll.getChildByText(uisel, video);
                    if(!uiobj.exists())
                    {
                        Log.e(TAG, "launchVideo: error, can't find video '" + video + "'");
                        video = null;
                    }
                }
                if(video == null)
                {
                    // just pick the first item
                    uisel = new UiSelector().className(android.widget.RelativeLayout.class.getName());
                    uiobj = uicoll.getChildByInstance(uisel, 0);
                    uisel = new UiSelector().className(android.widget.ImageView.class.getName());
                    uiobj = uiobj.getChild(uisel);
                }
                if(!uiobj.waitForExists(5000))
                {
                    // should have adb pushed a video to /sdcard/download/
                    Log.e(TAG, "launchVideo: error, are no videos listed - valetm error");
                    success = false;
                }
                // play it

                Log.d(TAG, "launchVideo: playing the video");
                uiobj.clickAndWaitForNewWindow();

            }
        }
        return success;
    }

    private boolean lockVideo()
    {
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        int             screenWidth, screenHeight;
        int             attempts;
        boolean         success = true;

        screenWidth = getUiDevice().getDisplayWidth();
        screenHeight = getUiDevice().getDisplayHeight();

        // click to open the menu
        attempts = 10;
        do
        {
            try
            {
                if(attempts == 5)
                {
                    Log.d(TAG, "lockVideo: pausing the video and trying again");
                    togglePauseResume();
                }

                uisel = new UiSelector().description("More options");
                uiobj = new UiObject(uisel);
                success = uiobj.waitForExists(500);
                if(success)
                {
                    // open the menu
                    Log.d(TAG, "lockVideo: opening the menu...");
                    uiobj.click();
                    sleep(500);

                    // select the tools menu item
                    Log.d(TAG, "lockVideo: selecting Tools...");
                    uisel = new UiSelector().text("Tools");
                    uiobj = new UiObject(uisel);
                    uiobj.click();
                    sleep(500);

                    // select the lock submenu item to bring up the lock dialog
                    Log.d(TAG, "lockVideo: selecting Lock...");
                    uisel = new UiSelector().text("Lock");
                    uiobj = new UiObject(uisel);
                    uiobj.clickAndWaitForNewWindow();
                    sleep(500);

                    // hit OK
                    Log.d(TAG, "lockVideo: clicking OK...");
                    uisel = new UiSelector().text("OK");
                    uiobj = new UiObject(uisel);
                    uiobj.clickAndWaitForNewWindow();
                }
                else
                {
                    Log.d(TAG, "lockVideo: can't see the menu");

                    // tap the screen to bring up the menu
                    Log.d(TAG, "lockVideo: tapping the screen and looking for the menu...");
                    getUiDevice().click(screenWidth / 2, screenHeight / 2);
                }
            }
            catch(UiObjectNotFoundException nfex)
            {
                Log.d(TAG, "lockVideo: failed to set the lock");
                success = false;
            }
        }
        while(!success && --attempts > 0);
        if(!success)
        {
            Log.e(TAG, "lockVideo: returning failure");
        }
        return success;
    }

    private void checkForAd() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        // if this is the first run, we'll see an advert - just close it
        //
        Log.d(TAG, "checkForAd: checking for 'What's New' advert");
        uisel = new UiSelector().textContains("What's new in");
        uiobj = new UiObject(uisel);
        if(uiobj.waitForExists(1000))
        {
            Log.d(TAG, "checkForAd: found it - clicking OK");
            uisel = new UiSelector().text("OK");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }
    }

    private void checkForHwAccelerationInfoBox() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        // if this is the first run, we'll see an advert - just close it
        //
        Log.d(TAG, "checkForHwAccelerationAd: checking for 'Hardware acceleration' info box");
        uisel = new UiSelector().textContains("Hardware acceleration");
        uiobj = new UiObject(uisel);
        if(uiobj.waitForExists(1000))
        {
            Log.d(TAG, "checkForAd: found it - clicking OK");
            uisel = new UiSelector().text("OK");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }
    }

    private void checkForResumePlay() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        // if this is the first run, we'll see an advert - just close it
        //
        Log.d(TAG, "checkForResumePlay: checking for 'resume' dialog");
        uisel = new UiSelector().textContains("you wish to resume");
        uiobj = new UiObject(uisel);
        if(uiobj.exists())
        {
            Log.d(TAG, "checkForResumePlay: found it - Start over");
            uisel = new UiSelector().text("Start over");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }
    }

    private void showControls() throws UiObjectNotFoundException
    {
        UiDevice    uidev = getUiDevice();
        UiSelector  uisel;
        UiObject    uiobj;
        int         screenWidth, screenHeight;
        int         attempt;
        boolean     success = false;

        screenWidth = getUiDevice().getDisplayWidth();
        screenHeight = getUiDevice().getDisplayHeight();

        uisel = new UiSelector().className(android.widget.ListView.class.getName());
        uiobj = new UiObject(uisel);
        if(uiobj.exists())
        {
            Log.d(TAG, "showControls: the list is showing, clicking it away");
            uidev.click(screenWidth / 2, screenHeight / 2);
            if(!uiobj.waitUntilGone(1000))
            {
                Log.d(TAG, "showControls: can't get rid of it!");
                throw(new UiObjectNotFoundException("More options list won't go away"));
            }
        }

        uisel = new UiSelector().resourceIdMatches("(.*)(posText)");
        uiobj = new UiObject(uisel);

        if(uiobj.exists())
        {
            // reshow the controls so that they're freshly visible (and their
            // auto hide counter is reset)
            //
            Log.d(TAG, "showControls: clicking to hide the controls");
            uidev.click(screenWidth / 2, screenHeight / 2);
            uiobj.waitUntilGone(1000);
        }
        Log.d(TAG, "showControls: clicking to show the controls");
        uidev.click(screenWidth / 2, screenHeight / 2);
        Log.d(TAG, "showControls: done");
    }

    private boolean isPaused()
    {
        UiSelector  uisel;
        UiObject    uiobj;
        boolean     paused = false;

        try
        {
            uisel = new UiSelector().resourceIdMatches("(.*)(posText)");
            uiobj = new UiObject(uisel);
            if(!uiobj.exists())
            {
                Log.d(TAG, "isPaused: can't find playback position, assuming the video's running");
            }
            else
            {
                String t1, t2;

                t1 = uiobj.getText();
                sleep(1100);
                t2 = uiobj.getText();
                if(t1.equals(t2))
                {
                    Log.d(TAG, "isPaused: playback position(" + t1 + ") not progressing, we're paused");
                    paused = true;
                }
                else
                {
                    Log.d(TAG, "isPaused: playback position(" + t1 + ") advanced to (" + t2 + ") - playback in progress");
                }
            }
        }
        catch(UiObjectNotFoundException nfex)
        {
            Log.d(TAG, "isPaused: playback position disappeared - playback in progress");
        }
        return paused;
    }

    private void togglePauseResume()
    {
        Log.d(TAG, "togglePauseResume: called");
        getUiDevice().pressKeyCode(KeyEvent.KEYCODE_SPACE);
    }

    private void rewind() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        Log.d(TAG, "rewind: trying a left swipe");
        uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/ui_layout");
        uiobj = new UiObject(uisel);
        uiobj.swipeLeft(100);
    }

    private boolean showProgressBar() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;
        boolean     success = true;

        Log.d(TAG, "showProgressBar: called");
        uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/progressBar");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "showProgressBar: not currently visible - tapping the screen");

            // try tapping the screen
            getUiDevice().click(getUiDevice().getDisplayWidth() / 2, getUiDevice().getDisplayHeight() / 2);
            sleep(500);

            uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/progressBar");
            uiobj = new UiObject(uisel);
            if(!uiobj.exists())
            {
                // last attempt: there's a toast message: "Swipe down from the top to exit full screen. OK"
                // which is invisible - give that a go
                Log.d(TAG, "showProgressBar: can't see the progress bar - trying a down swipe");
                sleep(1000);
                uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/ui_layout");
                uiobj = new UiObject(uisel);
                uiobj.swipeDown(100);

                // now tap the screen to reveal the progress bar
                getUiDevice().click(getUiDevice().getDisplayWidth() / 2, getUiDevice().getDisplayHeight() / 2);
                sleep(500);

                uisel = new UiSelector().resourceId("com.mxtech.videoplayer.ad:id/progressBar");
                uiobj = new UiObject(uisel);
                if(!uiobj.waitForExists(1000))
                {
                    Log.e(TAG, "showProgressBar: error - can't find the progress bar!");
                    success = false;
                }
            }
        }
        Log.d(TAG, "showProgressBar: returning: " + success);
        return success;
    }
}

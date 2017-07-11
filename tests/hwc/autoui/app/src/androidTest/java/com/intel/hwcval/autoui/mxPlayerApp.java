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

 File Name:     mxPlayer.java

 Description:   tests for the MX Player video player application.

 Environment:   tested with com.mxtech.videoplayer.ad-1.7.38-APK4Fun.com.apk

 Notes:         MX Player doesn't seem to work with UiAutomator's rotation
                functions.

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.app.UiAutomation;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiCollection;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;
import android.view.KeyEvent;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Pattern;


public class mxPlayerApp extends uiautomatorHelper
{
    // ETM X-Component test: External display_Multiple resolutions (single res)
    public void testMXPlayer() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testMXPlayer";

        Log.d(TAG, "testMXPlayer: opening the video player...");
        if(!launchApp(MXPLAYER_PACKAGE))
        {
            Log.e(TAG, "testMXPlayer: failed to open MX Player - trying the 'all apps' panel");
            if(!launchApp(MXPLAYER_PACKAGE, "MX Player"))
            {
                Log.e(TAG, "testMXPlayer: failed to open MX Player");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!launchVideoMxPlayer("Amazon_1080"))
        {
            SendNotRunStatus(testName);
            return;
        }

        // if we're in fullscreen, the controls won't be visible
        if(!clearFullScreenToast())
        {
            Log.d(TAG, "testMXPlayer: can't find the play controls!");
            SendNotRunStatus(testName);
            return;
        }

        // wait for it to go into extended mode (if there's a second display connected)
        SystemClock.sleep(7500);

        Log.d(TAG, "testMXPlayer: advancing video playback...");
        pauseVideo();
        mDevice.swipe(mDevice.getDisplayWidth() / 10,       mDevice.getDisplayHeight() / 2,
                      (mDevice.getDisplayWidth() / 10) * 4, mDevice.getDisplayHeight() / 2,
                100);
        playVideo();
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testMXPlayer: reversing video playback...");
        pauseVideo();
        mDevice.swipe((mDevice.getDisplayWidth() / 10) * 4, mDevice.getDisplayHeight() / 2,
                      mDevice.getDisplayWidth() / 10,       mDevice.getDisplayHeight() / 2,
                      100);
        playVideo();
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testMXPlayer: exiting the video");
        mDevice.pressBack();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testMXPlayer: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: External display_Multiple resolutions (multi res)
    public void testMXPlayerMultiRes() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testMXPlayerMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1280*720P@60Hz", "800*600P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testMXPlayerMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testMXPlayerMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testMXPlayerMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testMXPlayerMultiRes: selected mode %s, running testMXPlayer()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testMXPlayer();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testMXPlayerMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testMXPlayerMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testMXPlayerMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Video_Lock_Unlock (single res)
    public void testMXPlayerLock() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testMXPlayerLock";
        UiObject2 uiobj;
        boolean success;

        Log.d(TAG, "testMXPlayerLock: opening the video player...");
        if(!launchApp(MXPLAYER_PACKAGE))
        {
            Log.e(TAG, "testMXPlayerLock: failed to open MX Player - trying the 'all apps' panel");
            if(!launchApp(MXPLAYER_PACKAGE, "MX Player"))
            {
                Log.e(TAG, "testMXPlayerLock: failed to open MX Player");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!launchVideoMxPlayer("Amazon_1080"))
        {
            SendNotRunStatus(testName);
            return;
        }

        // if we're in fullscreen, the controls won't be visible
        if(!clearFullScreenToast())
        {
            Log.d(TAG, "testMXPlayerLock: can't find the play controls!");
            SendNotRunStatus(testName);
            return;
        }

        success = lockVideo();
        if(success)
        {
            Log.d(TAG, "testMXPlayerLock: testing the lock");
            for(int i = 0; i < 10; ++i)
            {
                Log.d(TAG, "testMXPlayerLock: tapping the screen and looking for the menu...");
                showPlayControls();
                uiobj = waitUiObject2(By.desc("More options"), mShortUpdateTimeout);
                if (uiobj != null)
                {
                    Log.e(TAG, "testMXPlayerLock: found the menu - obviously not locked!");
                    success = false;
                    break;
                }
            }
        }

        if(success)
        {
            Log.d(TAG, "testMXPlayerLock: performing unlock...");
            mDevice.click(mDevice.getDisplayWidth() - 50, 50);
            SystemClock.sleep(mShortUpdateTimeout);
            mDevice.click(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() - 50);
            SystemClock.sleep(mShortUpdateTimeout);
            mDevice.click(50, mDevice.getDisplayHeight() - 50);
            SystemClock.sleep(mShortUpdateTimeout);
            mDevice.click(50, 50);
            SystemClock.sleep(mShortUpdateTimeout);
        }

        if(success)
        {
            Log.d(TAG, "testMXPlayerLock: testing the unlock");
            showPlayControls();
            uiobj = waitUiObject2(By.desc("More options"), mShortUpdateTimeout);
            if (uiobj == null)
            {
                Log.e(TAG, "testMXPlayerLock: didn't find the menu - the unlock didn't work");
                success = false;
            }
        }

        playVideo();

        Log.d(TAG, "testMXPlayerLock: finished");
        SendSuccessStatus(testName, success);
    }

    // ETM X-Component test: Video_Lock_Unlock (multi res)
    public void testMXPlayerLockMultiRes() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testMXPlayerLockMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testMXPlayerLockMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testMXPlayerLockMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testMXPlayerLockMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testMXPlayerLockMultiRes: selected mode %s, running testMXPlayerLock()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testMXPlayerLock();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testMXPlayerLockMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testMXPlayerLockMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testMXPlayerLockMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Playback_Portrait_Landscape (single res)
    public void testVideoPortraitLandscape() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testVideoPortraitLandscape";
        String oldOrientation;

        Log.d(TAG, "testVideoPortraitLandscape: opening the video player...");
        if(!launchApp(MXPLAYER_PACKAGE))
        {
            Log.e(TAG, "testVideoPortraitLandscape: failed to open MX Player - trying the 'all apps' panel");
            if(!launchApp(MXPLAYER_PACKAGE, "MX Player"))
            {
                Log.e(TAG, "testVideoPortraitLandscape: failed to open MX Player");
                SendNotRunStatus(testName);
                return;
            }
        }

        Log.d(TAG, "testVideoPortraitLandscape: setting 'Landscape' orientation");
        oldOrientation = setOrientation("Landscape");
        if(oldOrientation == null)
        {
            Log.e(TAG, "testVideoPortraitLandscape: couldn't set landscape orientation");
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testVideoPortraitLandscape: playing the video...");
        if(!launchVideoMxPlayer("Video_Camera_Portrait_Mode"))
        {
            Log.e(TAG, "testVideoPortraitLandscape: failed to run the video");
            SendNotRunStatus(testName);
            return;
        }
        SystemClock.sleep(10000);

        Log.d(TAG, "testVideoPortraitLandscape: quitting the video");
        mDevice.pressBack();

        Log.d(TAG, String.format("testVideoPortraitLandscape: setting '%s' orientation", oldOrientation));
        setOrientation(oldOrientation);

        Log.d(TAG, "testVideoPortraitLandscape: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Playback_Portrait_Landscape (multi res)
    public void testVideoPortraitLandscapeMultiRes() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testVideoPortraitLandscapeMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testVideoPortraitLandscapeMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testVideoPortraitLandscapeMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testVideoPortraitLandscapeMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testVideoPortraitLandscapeMultiRes: selected mode %s, running testVideoPortraitLandscape()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testVideoPortraitLandscape();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testVideoPortraitLandscapeMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testVideoPortraitLandscapeMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testVideoPortraitLandscapeMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Video_Rotation (single res)
    public void testVideoRotation() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testVideoRotation";
        final int timeInOrientation = 10000;
        UiAutomation uia = getInstrumentation().getUiAutomation();
        String originalOrientation;
        String videos[] = {"Video_Camera_Portrait_Mode", "6.avc_sub.mp4"};
        int rotations[] = {UiAutomation.ROTATION_FREEZE_270, UiAutomation.ROTATION_FREEZE_180, UiAutomation.ROTATION_FREEZE_90};
        boolean success = true;

        for(int vid = 0; vid < videos.length; ++vid)
        {
            if(!launchApp(MXPLAYER_PACKAGE))
            {
                Log.e(TAG, "testVideoRotation: can't find MX player");
                success = false;
            }
            else
            {
                Log.d(TAG, "testVideoRotation: setting orientation to 'Use system default'");
                originalOrientation = setOrientation("Use system default");

                launchVideoMxPlayer(videos[vid]);
                SystemClock.sleep(mLongDelayForObserver);

                for(int rot = 0; rot < rotations.length; ++rot)
                {
                    Log.d(TAG, String.format("testVideoRotation: testing rotation_index %d...", rot));
                    if(!uia.setRotation(rotations[rot]))
                    {
                        Log.e(TAG, String.format("testVideoRotation: rotation_index %d, setRotation failed", rot));
                        continue;
                    }
                    SystemClock.sleep(timeInOrientation);

                    Log.d(TAG, String.format("testVideoRotation: rotation_index %d, pausing video playback...", rot));
                    pauseVideo();
                    SystemClock.sleep(timeInOrientation);

                    Log.d(TAG, String.format("testVideoRotation: rotation_index %d, rewinding video playback...", rot));
                    mDevice.swipe((mDevice.getDisplayWidth() / 10) * 4, mDevice.getDisplayHeight() / 2,
                            mDevice.getDisplayWidth() / 10, mDevice.getDisplayHeight() / 2,
                            100);
                    Log.d(TAG, String.format("testVideoRotation: rotation_index %d, restarting video...", rot));
                    playVideo();
                    SystemClock.sleep(mLongDelayForObserver);
                }

                Log.d(TAG, "testVideoRotation: quickly flipping through the rotations...");
                pauseVideo();
                setOrientation(originalOrientation);
                playVideo();
                for(int rot = 0; rot < rotations.length; ++rot)
                {
                    SystemClock.sleep(4000);
                    Log.d(TAG, String.format("testVideoRotation: testing rotation_index %d...", rot));
                    if(!uia.setRotation(rotations[rot]))
                    {
                        Log.e(TAG, String.format("testVideoRotation: rotation_index %d, setRotation(2) failed", rot));
                        continue;
                    }
                }

                Log.d(TAG, "testVideoRotation: quitting the video");
                mDevice.pressBack();

                SystemClock.sleep(mLongDelayForObserver);
                Log.d(TAG, "testVideoRotation: resetting orientation");
                setOrientation(originalOrientation);
            }
        }
        Log.d(TAG, "testVideoRotation: finished");
        SendSuccessStatus(testName, success);
    }

    // ETM X-Component test: Video_Rotation (multi res)
    public void testVideoRotationMultiRes() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testVideoRotationMultiRes";
        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testVideoRotationMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testVideoRotationMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testVideoRotationMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testVideoRotationMultiRes: selected mode %s, running testVideoPortraitLandscape()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testVideoRotation();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testVideoRotationMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testVideoRotationMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testVideoRotationMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    private String setOrientation(String orientation) throws UiObjectNotFoundException
    {
        UiObject2 uiobj;
        UiObject uiobj1;
        UiSelector uisel;
        UiCollection uicoll;
        String oldOrientation;

        Log.d(TAG, "setOrientation: opening player settings...");
        if(!openSettings("Player"))
            return null;

        Log.d(TAG, "setOrientation: selecting Screen...");
        uiobj = waitUiObject2(By.text("Screen"), mShortUpdateTimeout);
        uiobj.click();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "setOrientation: selecting Orientation...");
        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(orientation)")), mShortUpdateTimeout);
        uiobj.click();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "setOrientation: getting current orientation...");
        uisel = new UiSelector().className(android.widget.ListView.class.getName());
        uicoll = new UiCollection(uisel);
        uisel = new UiSelector().checked(true);
        uiobj1 = uicoll.getChild(uisel);
        oldOrientation = uiobj1.getText();

        Log.d(TAG, String.format("setOrientation: changing orientation to '%s'...", orientation));
        uisel = new UiSelector().className(android.widget.CheckedTextView.class.getName());
        uiobj1 = uicoll.getChildByText(uisel, orientation);
        uiobj1.click();

        // press OK to return to the Player menu
        uiobj = waitUiObject2(By.text("OK"), mShortUpdateTimeout);
        uiobj.click();
        SystemClock.sleep(mShortDelayForObserver);

        // press back to exit from the settings menu
        Log.d(TAG, "setOrientation: hitting back button");
                mDevice.pressBack();
        SystemClock.sleep(mShortDelayForObserver);

        return oldOrientation;
    }

    public boolean openSettings(String option)
    {
        UiObject2 uiobj;
        boolean success = false;

        uiobj = waitUiObject2(By.desc("More options"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "openSettings: opening the menu...");
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            Log.d(TAG, "openSettings: selecting Settings...");
            uiobj = waitUiObject2(By.text("Settings"), mShortUpdateTimeout);
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            Log.d(TAG, String.format("openSettings: selecting '%s'...", option));
            uiobj = waitUiObject2(By.text(option), mShortUpdateTimeout);
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);
            success = true;
        }

        Log.d(TAG, String.format("openSettings(%s): returning %s", option, success ? "Success" : "Failure"));
        return success;
    }

    private boolean lockVideo()
    {
        UiObject2 uiobj;
        boolean success = false;

        Log.d(TAG, "lockVideo: showing the play controls");
        showPlayControls();

        uiobj = waitUiObject2(By.desc("More options"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "lockVideo: opening the menu...");
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            Log.d(TAG, "lockVideo: selecting Tools...");
            uiobj = waitUiObject2(By.text("Tools"), mShortUpdateTimeout);
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            Log.d(TAG, "lockVideo: selecting Lock...");
            uiobj = waitUiObject2(By.text("Lock"), mShortUpdateTimeout);
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            Log.d(TAG, "lockVideo: clicking OK...");
            uiobj = waitUiObject2(By.text("OK"), mShortUpdateTimeout);
            uiobj.click();
            SystemClock.sleep(mShortDelayForObserver);

            success = true;
        }
        return success;
    }

    private boolean clearFullScreenToast()
    {
        if(!showPlayControls())
        {
            Log.d(TAG, "clearFullScreenToast: seem to be in fullscreen mode - trying a down swipe");
            mDevice.swipe(mDevice.getDisplayWidth() / 2, 0,
                          mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2,
                          100);
            return showPlayControls();
        }
        return true;
    }

    private boolean showPlayControls()
    {
        Pattern playPausePattern = Pattern.compile("(.*)(playpause)");
        UiObject2 uiobj;

        uiobj = findUiObject2(By.res(playPausePattern));
        if (uiobj != null)
        {
            Log.d(TAG, "showPlayControls: play controls are visible - we're not fullscreen");

            // make sure they're not about to disappear by tapping the screen once to hide them,
            // then again to show them
            Log.v(TAG, "showPlayControls: tapping the screen to hide the controls");
            tapScreen();
            SystemClock.sleep(250);
            Log.d(TAG, "showPlayControls: tapping the screen to show them once more");
            tapScreen();
            return true;
        }

        // click the centre of the screen - if we're not in full screen mode, this will bring the
        // play controls up
        Log.v(TAG, "showPlayControls: play controls aren't visible - trying a screen tap");
        tapScreen();
        uiobj = waitUiObject2(By.res(playPausePattern), mUpdateTimeout);
        if (uiobj != null)
        {
            Log.d(TAG, "showPlayControls: play controls are now visible");
            return true;
        }

        Log.d(TAG, "showPlayControls: play controls aren't visible, we may be in fullscreen mode");
        return false;
    }

    private void pauseVideo() throws UiObjectNotFoundException
    {
        if(isVideoPlaying())
        {
            Log.d(TAG, "pauseVideo: pausing...");
            mDevice.pressKeyCode(KeyEvent.KEYCODE_SPACE);
        }
        else
        {
            Log.d(TAG, "pauseVideo: video is already paused");
        }
    }

    private void playVideo() throws UiObjectNotFoundException
    {
        if(!isVideoPlaying())
        {
            Log.d(TAG, "playVideo: playing...");
            mDevice.pressKeyCode(KeyEvent.KEYCODE_SPACE);
        }
        else
        {
            Log.d(TAG, "playVideo: video is already playing");
        }
    }

    private boolean isVideoPlaying() throws UiObjectNotFoundException
    {
        Pattern progressPattern = Pattern.compile("(.*)(posText)");
        UiObject2 uiobj;
        String[] time = new String[2];
        boolean paused;

        for(int i = 0; i < 2; ++i)
        {
            if(i != 0) SystemClock.sleep(1250);

            showPlayControls();
            uiobj = findUiObject2(By.res(progressPattern));
            if (uiobj == null)
            {
                Log.e(TAG, "isVideoPlaying - error, can't find progress counter");
                throw new UiObjectNotFoundException("isVideoPlaying: didn't find progress counter");
            }
            time[i] = uiobj.getText();
        }
        paused = time[0].equals(time[1]);
        return !paused;
    }

}

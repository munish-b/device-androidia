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

 File Name:     camera.java

 Description:   tests for the bundled Camera application.

 Environment:

 Notes:         Camera appears to only partially work with UiAutomator's
                rotation functions: it rotates its UI but still records
                videos in portrait/landscape mode according to its physical
                orientation.

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.app.UiAutomation;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Pattern;


public class camera extends uiautomatorHelper
{
    // ETM X-Component test: Camera_Basic_Pic_Video (single res)
    public void testBasicCamera() throws UiObjectNotFoundException, RemoteException
    {
        final String    testName = "testBasicCamera";
        Pattern         shutterButtonPattern = Pattern.compile("(.*)(shutter_button)");
        UiObject2       uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroll;

        Log.d(TAG, "testBasicCamera: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testBasicCamera: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testBasicCamera: failed to open the camera ");
                SendNotRunStatus(testName);
                return;
            }
        }

        if(!cameraIsPresent())
        {
            Log.e(TAG, "testBasicCamera: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            // An app, when it is loaded, starts always on the same screen;
            // if the app has been already loaded, then not sure on which screen it will start.
            // Most apps can start only in one or two places.
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testBasicCamera: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testBasicCamera: taking a photo");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectCamera())
        {
            Log.e(TAG, "testBasicCamera: couldn't select the camera");
            SendSuccessStatus(testName, false);
            return;
        }
        uiobj = waitUiObject2(By.res(shutterButtonPattern), mUpdateTimeout);
        uiobj.click();
        Log.d(TAG, "testBasicCamera: clicked camera shutter");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

        Log.d(TAG, "testBasicCamera: making a 10 second video");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation
        if(!selectVideo())
        {
            SendNotRunStatus(testName);
            return;
        }
        uiobj = waitUiObject2(By.res(shutterButtonPattern), mUpdateTimeout);
        uiobj.click();
        Log.d(TAG, "testBasicCamera: started video recording...");
        SystemClock.sleep(10000);
        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();
        Log.d(TAG, "testBasicCamera: ...stopped video recording");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

        Log.d(TAG, "testBasicCamera: opening the film strip");
        SystemClock.sleep(mShortDelayForObserver);
        mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                      0, mDevice.getDisplayHeight() / 2,
                      100);

        Log.d(TAG, "testBasicCamera: clicking through images...");
        uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
        uiscroll = new UiScrollable(uisel).setAsHorizontalList();

        Log.d(TAG, "testBasicCamera: selecting the video");
        uisel = new UiSelector().className(android.widget.ImageView.class.getName());
        uiscroll.getChildByInstance(uisel, 0).click();

        // if it's the first time the Camera activity has been asked to show
        // a video we'll get a resolver dialog
        // quite a lot of the apps when run the first time ask for
        // some info and configuration. In Settings I can erase everything and reboot
        // so I will get again the dialog with the settings. Good way of checking the
        // initial installation. TO TRY
        HandleResolverDialog("Photos", "Always");

        long startPlayMillis = SystemClock.elapsedRealtime();
        long videoTime;
        do
        {
            Log.d(TAG, "testBasicCamera: the video is playing...");
            SystemClock.sleep(1000);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(videoplayer)")));
            videoTime = SystemClock.elapsedRealtime() - startPlayMillis;
            if(videoTime > 60000)
            {
                Log.e(TAG, String.format("testBasicCamera: video still playing after %d seconds?! Aborting", videoTime / 1000));
                mDevice.pressBack();
                break;
            }
        }
        while(uiobj != null && uiobj.isFocused());

        try
        {
            Log.d(TAG, "testBasicCamera: deleting the video");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
            uiobj.click();
            SystemClock.sleep(mUpdateTimeout);
        }
        catch(NullPointerException npe)
        {
            Log.d(TAG, "testBasicCamera: failed to delete video - attempting to re-open the film strip");
            SystemClock.sleep(mShortDelayForObserver);
            SystemClock.sleep(mShortDelayForObserver);
            mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                    0, mDevice.getDisplayHeight() / 2,
                    100);

            Log.d(TAG, "testBasicCamera: trying to delete the video again");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
            uiobj.click();
            SystemClock.sleep(mUpdateTimeout);
        }

        try
        {
            Log.d(TAG, "testBasicCamera: selecting the photo");
            SystemClock.sleep(mShortDelayForObserver);
            uisel = new UiSelector().className(android.widget.ImageView.class.getName());
            uiscroll.getChildByInstance(uisel, 0).click();
        }
        catch(UiObjectNotFoundException nfe)
        {
            Log.d(TAG, "testBasicCamera: attempting to re-open the film strip");
            SystemClock.sleep(mShortDelayForObserver);
            SystemClock.sleep(mShortDelayForObserver);
            mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                    0, mDevice.getDisplayHeight() / 2,
                    100);

            uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
            uiscroll = new UiScrollable(uisel).setAsHorizontalList();

            Log.d(TAG, "testBasicCamera: selecting the photo again");
            SystemClock.sleep(mShortDelayForObserver);
            uisel = new UiSelector().className(android.widget.ImageView.class.getName());
            uiscroll.getChildByInstance(uisel, 0).click();
        }

        Log.d(TAG, "testBasicCamera: admiring the photo for a bit...");
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testBasicCamera: deleting the photo");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);

        Log.d(TAG, "testBasicCamera: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Basic_Pic_Video (multi res)
    // This test goes through the HDMI modes and then selectes the non HDMI version of the test.
    public void testBasicCameraMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testBasicCameraMultiRes";

        Log.d(TAG, "testBasicCameraMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testBasicCameraMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testBasicCameraMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testBasicCameraMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testBasicCameraMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testBasicCameraMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testBasicCameraMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testBasicCameraMultiRes: selected mode %s, running testBasicCamera()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testBasicCamera();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testBasicCameraMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testBasicCameraMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testBasicCameraMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM XComponent test: Camera_Basic_Preview (single res)
    public void testCameraBasicPreview() throws RemoteException, UiObjectNotFoundException
    {
        final String testName = "testCameraBasicPreview";
        UiObject2 uiobj;
        int switchedCamera = 0;

        Log.d(TAG, "testCameraBasicPreview: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraBasicPreview: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraBasicPreview: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraBasicPreview: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testCameraBasicPreview: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testCameraBasicPreview: selecting the camera");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectCamera())
        {
            SendNotRunStatus(testName);
            return;
        }

        for(int i = 0; i < 5; ++i)
        {
            Log.d(TAG, "testCameraBasicPreview: admiring the preview");
            SystemClock.sleep(mLongDelayForObserver);

            if(toggleCamera())
                ++switchedCamera;
        }

        if(switchedCamera == 0)
        {
            Log.e(TAG, "testCameraBasicPreview: didn't manage to toggle the camera");
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, String.format("testCameraBasicPreview: toggled front/back camera %d times", switchedCamera));
        Log.d(TAG, "testCameraBasicPreview: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM XComponent test: Camera_Basic_Preview (multi res)
    public void testCameraBasicPreviewMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testCameraBasicPreviewMultiRes";

        Log.d(TAG, "testCameraBasicPreviewMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraBasicPreviewMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraBasicPreviewMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraBasicPreviewMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testCameraBasicPreviewMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testCameraBasicPreviewMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testCameraBasicPreviewMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testCameraBasicPreviewMultiRes: selected mode %s, running testCameraBasicPreview()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testCameraBasicPreview();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testCameraBasicPreviewMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testCameraBasicPreviewMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testCameraBasicPreviewMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Video Preview Landscape Mode_1080p (single res)
    public void testVideoPreview1080p() throws UiObjectNotFoundException, RemoteException
    {
        final String testName = "testVideoPreview1080p";
        UiObject2 uiobj;

        Log.d(TAG, "testVideoPreview1080p: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testVideoPreview1080p: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testVideoPreview1080p: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testVideoPreview1080p: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testVideoPreview1080p: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testVideoPreview1080p: selecting the video camera");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectVideo())
        {
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testVideoPreview1080p: opening the menu...");
        SystemClock.sleep(mShortDelayForObserver);
        mDevice.swipe(0, mDevice.getDisplayHeight() / 2,
                mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2,
                100);

        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(settings_button)")), mUpdateTimeout);
        if (uiobj == null)
        {
            Log.e(TAG, "testVideoPreview1080p: failed to open the mode options overlay");
            SendNotRunStatus(testName);
            return;
        }
        uiobj.click();

        Log.d(TAG, "testVideoPreview1080p: selecting resolution...");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = waitUiObject2(By.text("Resolution & quality"), mUpdateTimeout);
        uiobj.click();

        // there will normally be both back and front camera, just pick the first on the assumption
        // that the highest res camera is first in the list
        Log.d(TAG, "testVideoPreview1080p: selecting video camera...");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = waitUiObject2(By.text(Pattern.compile("(.*)(camera video)")), mShortUpdateTimeout);
        uiobj.click();

        Log.d(TAG, "testVideoPreview1080p: selecting 1080p...");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = waitUiObject2(By.text(Pattern.compile("(.*)(1080p)")), mShortUpdateTimeout);
        if(uiobj == null)
        {
            Log.d(TAG, "testVideoPreview1080p: 1080p unavailable, trying 720p...");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.text(Pattern.compile("(.*)(720p)")), mShortUpdateTimeout);
            if(uiobj == null)
            {
                Log.d(TAG, "testVideoPreview1080p: 720p unavailable, giving up...");
                SendNotRunStatus(testName);
                return;
            }
        }
        uiobj.click();

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testVideoPreview1080p:navigating back to the video screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testVideoPreview1080p: previewing the video (not recording)");
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testVideoPreview1080p: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Playback_Pic_Video (single res)
    public void testCameraPlaybackPicVideo() throws UiObjectNotFoundException, RemoteException
    {
        final String    testName = "testCameraPlaybackPicVideo";
        Pattern         shutterButtonPattern = Pattern.compile("(.*)(shutter_button)");
        UiObject2       uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroll;

        Log.d(TAG, "testCameraPlaybackPicVideo: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraPlaybackPicVideo: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraPlaybackPicVideo: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraPlaybackPicVideo: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testCameraPlaybackPicVideo: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testCameraPlaybackPicVideo: selecting the camera");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectCamera())
        {
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testCameraPlaybackPicVideo: taking a photo");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();

        Log.d(TAG, "testCameraPlaybackPicVideo: opening the film strip");
        SystemClock.sleep(mShortDelayForObserver);
        mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                0, mDevice.getDisplayHeight() / 2,
                100);

        Log.d(TAG, "testCameraPlaybackPicVideo: clicking through images...");
        uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
        uiscroll = new UiScrollable(uisel).setAsHorizontalList();

        Log.d(TAG, "testCameraPlaybackPicVideo: selecting the photo");
        uisel = new UiSelector().className(android.widget.ImageView.class.getName());
        uiscroll.getChildByInstance(uisel, 0).click();

        Log.d(TAG, "testCameraPlaybackPicVideo: admiring the photo for a bit...");
        SystemClock.sleep(mLongDelayForObserver);

        Log.d(TAG, "testCameraPlaybackPicVideo: deleting the photo");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);

        Log.d(TAG, "testCameraPlaybackPicVideo: making a 30 second video");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation
        if(!selectVideo())
        {
            SendNotRunStatus(testName);
            return;
        }
        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();
        Log.d(TAG, "testCameraPlaybackPicVideo: started video recording...");
        SystemClock.sleep(30000);
        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();
        Log.d(TAG, "testCameraPlaybackPicVideo: ...stopped video recording");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

        Log.d(TAG, "testCameraPlaybackPicVideo: opening the film strip");
        SystemClock.sleep(mShortDelayForObserver);
        mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                0, mDevice.getDisplayHeight() / 2,
                100);

        Log.d(TAG, "testCameraPlaybackPicVideo: clicking through images...");
        uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
        uiscroll = new UiScrollable(uisel).setAsHorizontalList();

        Log.d(TAG, "testCameraPlaybackPicVideo: selecting the video");
        uisel = new UiSelector().className(android.widget.ImageView.class.getName());
        uiscroll.getChildByInstance(uisel, 0).click();

        // if it's the first time the Camera activity has been asked to show
        // a video we'll get a resolver dialog
        HandleResolverDialog("Photos", "Always");

        // allow the video to start playing, then tap the screen to bring up the play controls
        SystemClock.sleep(3000);
        tapScreen();
        SystemClock.sleep(500);
        uiobj = waitUiObject2(By.clazz(android.widget.SeekBar.class.getName()), 1000);
        if(uiobj != null)
        {
            Log.d(TAG, "testCameraPlaybackPicVideo: rewinding...");
            uiobj.scroll(Direction.RIGHT, 0.75f, 100);
        }
        SystemClock.sleep(3000);
        tapScreen();
        SystemClock.sleep(500);
        uiobj = waitUiObject2(By.clazz(android.widget.SeekBar.class.getName()), 1000);
        if(uiobj != null)
        {
            Log.d(TAG, "testCameraPlaybackPicVideo: fast forwarding...");
            uiobj.scroll(Direction.LEFT, 0.75f, 100);
        }

        SystemClock.sleep(3000);

        long startPlayMillis = SystemClock.elapsedRealtime();
        long videoTime;
        do
        {
            Log.d(TAG, "testCameraPlaybackPicVideo: the video is playing...");
            SystemClock.sleep(1000);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(videoplayer)")));
            if(uiobj != null)
            {
                Log.d(TAG, String.format("testCameraPlaybackPicVideo: video player focused? %c",
                        uiobj.isFocused() ? 'y' : 'n'));
            }
            videoTime = SystemClock.elapsedRealtime() - startPlayMillis;
            if(videoTime > 60000)
            {
                Log.e(TAG, String.format("testCameraPlaybackPicVideo: video still playing after %d seconds?! Aborting", videoTime / 1000));
                mDevice.pressBack();
                break;
            }
        }
        while(uiobj != null && uiobj.isFocused());

        try
        {
            Log.d(TAG, "testCameraPlaybackPicVideo: deleting the video");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
            uiobj.click();
            SystemClock.sleep(mUpdateTimeout);
        }
        catch(NullPointerException npe)
        {
            Log.d(TAG, "testCameraPlaybackPicVideo: failed to delete video - attempting to re-open the film strip");
            SystemClock.sleep(mShortDelayForObserver);
            SystemClock.sleep(mShortDelayForObserver);
            mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                    0, mDevice.getDisplayHeight() / 2,
                    100);

            Log.d(TAG, "testCameraPlaybackPicVideo: trying to delete the video again");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
            uiobj.click();
            SystemClock.sleep(mUpdateTimeout);
        }

        Log.d(TAG, "testCameraPlaybackPicVideo: finished");
        SendSuccessStatus(testName, true);

    }

    // ETM X-Component test: Camera_Playback_Pic_Video (multi res)
    public void testCameraPlaybackPicVideoMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testCameraPlaybackPicVideoMultiRes";

        Log.d(TAG, "testCameraPlaybackPicVideoMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraPlaybackPicVideoMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraPlaybackPicVideoMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraPlaybackPicVideoMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testCameraPlaybackPicVideoMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testCameraPlaybackPicVideoMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testCameraPlaybackPicVideoMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testCameraPlaybackPicVideoMultiRes: selected mode %s, running testCameraPlaybackPicVideo()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testCameraPlaybackPicVideo();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testCameraPlaybackPicVideoMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testCameraPlaybackPicVideoMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testCameraPlaybackPicVideoMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Playback_Portrait_Landscape (single res) - this doesn't work because
    // Camera switches its controls to portrait mode but still records the video in landscape mode. The
    // workaround is to run a canned portrait video through MX player, where it can be selected by name
    // - see mxPlayerApp.testVideoPortraitLandscape
    public void testVideoPortraitLandscape() throws UiObjectNotFoundException, RemoteException, IOException
    {
        final String    testName = "testVideoPortraitLandscape";
        Pattern         shutterButtonPattern = Pattern.compile("(.*)(shutter_button)");
        UiObject2       uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroll;

        Log.d(TAG, "testVideoPortraitLandscape: opening the camera...");
        if (!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testVideoPortraitLandscape: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testVideoPortraitLandscape: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testVideoPortraitLandscape: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if (uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testVideoPortraitLandscape: in the menu, navigating to main screen...");
            }
        }
        while (uiobj != null);

        Log.d(TAG, "testVideoPortraitLandscape: selecting the video camera");
        SystemClock.sleep(mShortDelayForObserver);
        if (!selectVideo())
        {
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testVideoPortraitLandscape: disabling accelerometer");
        DisableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        // switch out of the camera app so that the disabling on the accelerometer is seen when we
        // switch back in
        mDevice.pressHome();
        SystemClock.sleep(mShortDelayForObserver);
        switchToApp("Camera");
        SystemClock.sleep(mShortDelayForObserver);


        Log.d(TAG, "testVideoPortraitLandscape: switching to portrait...");
        mDevice.setOrientationRight();
        SystemClock.sleep(mShortDelayForObserver);

        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();
        Log.d(TAG, "testVideoPortraitLandscape: started video recording...");
        SystemClock.sleep(10000);
        uiobj = findUiObject2(By.res(shutterButtonPattern));
        uiobj.click();
        Log.d(TAG, "testVideoPortraitLandscape: ...stopped video recording");
        SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

        Log.d(TAG, "testVideoPortraitLandscape: switching to natural orientation...");
        mDevice.setOrientationNatural();

        Log.d(TAG, "testVideoPortraitLandscape: opening the film strip");
        SystemClock.sleep(mShortDelayForObserver);
        mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                0, mDevice.getDisplayHeight() / 2,
                100);

        Log.d(TAG, "testVideoPortraitLandscape: clicking through images...");
        uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
        uiscroll = new UiScrollable(uisel).setAsHorizontalList();

        Log.d(TAG, "testVideoPortraitLandscape: selecting the video");
        uisel = new UiSelector().className(android.widget.ImageView.class.getName());
        uiscroll.getChildByInstance(uisel, 0).click();

        // if it's the first time the Camera activity has been asked to show
        // a video we'll get a resolver dialog
        HandleResolverDialog("Photos", "Always");

        long startPlayMillis = SystemClock.elapsedRealtime();
        long videoTime;
        do
        {
            Log.d(TAG, "testVideoPortraitLandscape: the video is playing...");
            SystemClock.sleep(1000);
            uiobj = findUiObject2(By.res(Pattern.compile("(.*)(videoplayer)")));
            videoTime = SystemClock.elapsedRealtime() - startPlayMillis;
            if(videoTime > 60000)
            {
                Log.e(TAG, String.format("testVideoPortraitLandscape: video still playing after %d seconds?! Aborting", videoTime / 1000));
                mDevice.pressBack();
                break;
            }
        }
        while (uiobj != null && uiobj.isFocused());

        Log.d(TAG, "testVideoPortraitLandscape: deleting the video");
        SystemClock.sleep(mShortDelayForObserver);
        uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);

        mDevice.pressHome();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testVideoPortraitLandscape: enabling accelerometer");
        EnableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testVideoPortraitLandscape: finished");
        SendSuccessStatus(testName, true);
    }

    // ETM X-Component test: Camera_Playback_Portrait_Landscape (multi res)
    public void testVideoPortraitLandscapeMultiRes() throws RemoteException, UiObjectNotFoundException, IOException
    {
        final String    testName = "testVideoPortraitLandscapeMultiRes";

        Log.d(TAG, "testVideoPortraitLandscapeMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testVideoPortraitLandscapeMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testVideoPortraitLandscapeMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testVideoPortraitLandscapeMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

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

    // Non-Widi Test: Camera Preview Rotation (single display)
    public void testCameraPreviewRotate() throws RemoteException, UiObjectNotFoundException
    {
        final String testName = "testCameraPreviewRotate";
        UiAutomation uia = getInstrumentation().getUiAutomation();
        int rotations[] = {UiAutomation.ROTATION_FREEZE_270, UiAutomation.ROTATION_FREEZE_180, UiAutomation.ROTATION_FREEZE_90, UiAutomation.ROTATION_FREEZE_0};
        int rotation_angles[] = {270, 180, 90, 0};
        UiObject2 uiobj;

        Log.d(TAG, "testCameraPreviewRotate: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraPreviewRotate: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraPreviewRotate: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraPreviewRotate: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testCameraPreviewRotate: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testCameraPreviewRotate: selecting the camera");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectCamera())
        {
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testCameraPreviewRotate: disabling accelerometer rotation");
        DisableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        // switch out of the camera app so that the disabling on the accelerometer is seen when we
        // switch back in
        mDevice.pressHome();
        SystemClock.sleep(mShortDelayForObserver);
        switchToApp("Camera");
        SystemClock.sleep(mShortDelayForObserver);

        for(int fromRotation = 0; fromRotation < rotations.length; ++fromRotation)
        {
            for(int toRotation = 0; toRotation < rotations.length; ++toRotation)
            {
                if(fromRotation == toRotation)
                    continue;

                // try both front and back cameras
                for(int camera = 0; camera < 2; ++camera)
                {
                    Log.d(TAG, String.format("testCameraPreviewRotate: starting in orientation %d", rotation_angles[fromRotation]));
                    uia.setRotation(rotations[fromRotation]);

                    Log.d(TAG, "testCameraPreviewRotate: admiring the preview");
                    SystemClock.sleep(mLongDelayForObserver);

                    uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(camera_toggle_button)")), mShortUpdateTimeout);
                    if(uiobj == null)
                    {
                        Log.d(TAG, "testCameraPreviewRotate: toggling mode/options button");
                        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(mode_options_toggle)")), mShortUpdateTimeout);
                        if(uiobj == null)
                        {
                            // not an error if it's already open - see if the camera toggle is visible, below
                            Log.e(TAG, "testCameraPreviewRotate: failed to find the mode/options toggle");
                            continue;
                        }
                        else
                        {
                            uiobj.click();
                        }
                    }

                    Log.d(TAG, "testCameraPreviewRotate: toggling the front/back camera");
                    uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(camera_toggle_button)")), mShortUpdateTimeout);
                    if(uiobj == null)
                    {
                        Log.e(TAG, "testCameraPreviewRotate: failed to find the camera toggle");
                        continue;
                    }
                    uiobj.click();
                    SystemClock.sleep(mShortDelayForObserver);

                    Log.d(TAG, String.format("testCameraPreviewRotate: changing to orientation %d", rotation_angles[toRotation]));
                    uia.setRotation(rotations[toRotation]);

                    Log.d(TAG, "testCameraPreviewRotate: admiring the preview of the new orientation");
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
        }

        Log.d(TAG, "testCameraPreviewRotate: enabling accelerometer rotation");
        EnableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testCameraPreviewRotate: finished");
        SendSuccessStatus(testName, true);
    }

    // Non-Widi Test: Camera Preview Rotation (multi res)
    public void testCameraPreviewRotateMultiRes() throws RemoteException, UiObjectNotFoundException, IOException
    {
        final String    testName = "testCameraPreviewRotateMultiRes";

        Log.d(TAG, "testCameraPreviewRotateMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraPreviewRotateMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraPreviewRotateMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraPreviewRotateMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testCameraPreviewRotateMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testCameraPreviewRotateMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testCameraPreviewRotateMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testCameraPreviewRotateMultiRes: selected mode %s, running testCameraPreviewRotate()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testCameraPreviewRotate();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testCameraPreviewRotateMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testCameraPreviewRotateMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testCameraPreviewRotateMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    // Non-Widi Test: Camera Image+Video Capture (single res)
    public void testCameraVideoRotate() throws UiObjectNotFoundException, RemoteException
    {
        final String    testName = "testCameraVideoRotate";
        Pattern         shutterButtonPattern = Pattern.compile("(.*)(shutter_button)");
        UiAutomation    uia = getInstrumentation().getUiAutomation();
        int             rotations[] = {UiAutomation.ROTATION_FREEZE_270, UiAutomation.ROTATION_FREEZE_180, UiAutomation.ROTATION_FREEZE_90, UiAutomation.ROTATION_FREEZE_0};
        int             rotation_angles[] = {270, 180, 90, 0};
        UiObject2       uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroll;

        Log.d(TAG, "testCameraVideoRotate: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraVideoRotate: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraVideoRotate: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraVideoRotate: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        do
        {
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = waitUiObject2(By.desc("Navigate up"), mUpdateTimeout);
            if(uiobj != null)
            {
                uiobj.click();
                Log.d(TAG, "testCameraVideoRotate: in the menu, navigating to main screen...");
            }
        }
        while(uiobj != null);

        Log.d(TAG, "testCameraVideoRotate: selecting the camera");
        SystemClock.sleep(mShortDelayForObserver);
        if(!selectCamera())
        {
            SendNotRunStatus(testName);
            return;
        }

        Log.d(TAG, "testCameraVideoRotate: disabling accelerometer rotation");
        DisableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        // switch out of the camera app so that the disabling on the accelerometer is seen when we
        // switch back in
        mDevice.pressHome();
        SystemClock.sleep(mShortDelayForObserver);
        switchToApp("Camera");
        SystemClock.sleep(mShortDelayForObserver);

        for(int rotation = 0; rotation < rotations.length; ++rotation)
        {
            Log.d(TAG, String.format("testCameraVideoRotate: starting in orientation %d", rotation_angles[rotation]));
            uia.setRotation(rotations[rotation]);

            // Camera 1...
            //
            Log.d(TAG, "testCameraVideoRotate: taking a photo");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();

            Log.d(TAG, "testCameraVideoRotate: making a 20 second video");
            SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation
            if(!selectVideo())
            {
                SendNotRunStatus(testName);
                return;
            }
            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();
            Log.d(TAG, "testCameraVideoRotate: started video recording...");
            SystemClock.sleep(20000);
            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();
            Log.d(TAG, "testCameraVideoRotate: ...stopped video recording");
            SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

            // Camera 2...
            //
            Log.d(TAG, "testCameraVideoRotate: selecting the camera");
            SystemClock.sleep(mShortDelayForObserver);
            selectCamera();

            Log.d(TAG, "testCameraVideoRotate: switching to the other camera");
            SystemClock.sleep(mUpdateTimeout);
            toggleCamera();

            Log.d(TAG, "testCameraVideoRotate: taking a photo");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();

            Log.d(TAG, "testCameraVideoRotate: making a 20 second video");
            SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation
            if(!selectVideo())
            {
                SendNotRunStatus(testName);
                return;
            }

            Log.d(TAG, "testCameraVideoRotate: switching to the other video camera");
            SystemClock.sleep(mUpdateTimeout);
            toggleCamera();

            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();
            Log.d(TAG, "testCameraVideoRotate: started video recording...");
            SystemClock.sleep(20000);
            uiobj = findUiObject2(By.res(shutterButtonPattern));
            uiobj.click();
            Log.d(TAG, "testCameraVideoRotate: ...stopped video recording");
            SystemClock.sleep(mUpdateTimeout); // longer wait required because of the saved image animation

            Log.d(TAG, "testCameraVideoRotate: returning to the original video camera");
            SystemClock.sleep(mUpdateTimeout);
            toggleCamera();

            // Camera 1...
            Log.d(TAG, "testCameraVideoRotate: selecting the camera");
            SystemClock.sleep(mShortDelayForObserver);
            selectCamera();

            Log.d(TAG, "testCameraVideoRotate: returning to the original camera");
            SystemClock.sleep(mUpdateTimeout);
            toggleCamera();

            // Review the recordings...
            // Loop twice: once for the front camera recordings, a second time for the back camera...
            //
            Log.d(TAG, "testCameraVideoRotate: opening the film strip");
            SystemClock.sleep(mShortDelayForObserver);
            mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                    0, mDevice.getDisplayHeight() / 2,
                    100);

            for(int i = 0; i < 2; ++i)
            {
                Log.d(TAG, "testCameraVideoRotate: clicking through images...");
                uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
                uiscroll = new UiScrollable(uisel).setAsHorizontalList();

                Log.d(TAG, "testCameraVideoRotate: selecting the video");
                uisel = new UiSelector().className(android.widget.ImageView.class.getName());
                uiscroll.getChildByInstance(uisel, 0).click();

                // if it's the first time the Camera activity has been asked to show
                // a video we'll get a resolver dialog
                HandleResolverDialog("Photos", "Always");

                // allow the video to start playing, then tap the screen to bring up the play controls
                SystemClock.sleep(3000);
                tapScreen();
                SystemClock.sleep(500);
                uiobj = waitUiObject2(By.clazz(android.widget.SeekBar.class.getName()), 1000);
                if(uiobj != null)
                {
                    Log.d(TAG, "testCameraVideoRotate: rewinding...");
                    uiobj.scroll(Direction.RIGHT, 0.75f, 100);
                }
                SystemClock.sleep(3000);
                tapScreen();
                SystemClock.sleep(500);
                uiobj = waitUiObject2(By.clazz(android.widget.SeekBar.class.getName()), 1000);
                if(uiobj != null)
                {
                    Log.d(TAG, "testCameraVideoRotate: fast forwarding...");
                    uiobj.scroll(Direction.LEFT, 0.75f, 100);
                }

                SystemClock.sleep(3000);

                long startPlayMillis = SystemClock.elapsedRealtime();
                long videoTime;
                do
                {
                    Log.d(TAG, "testCameraVideoRotate: the video is playing...");
                    SystemClock.sleep(1000);
                    uiobj = findUiObject2(By.res(Pattern.compile("(.*)(videoplayer)")));
                    if(uiobj != null)
                    {
                        Log.d(TAG, String.format("testCameraVideoRotate: video player focused? %c",
                                uiobj.isFocused() ? 'y' : 'n'));
                    }
                    videoTime = SystemClock.elapsedRealtime() - startPlayMillis;
                    if(videoTime > 60000)
                    {
                        Log.e(TAG, String.format("testCameraVideoRotate: video still playing after %d seconds?! Aborting", videoTime / 1000));
                        mDevice.pressBack();
                        break;
                    }
                }
                while(uiobj != null && uiobj.isFocused());

                try
                {
                    Log.d(TAG, "testCameraVideoRotate: deleting the video");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
                    uiobj.click();
                    SystemClock.sleep(mUpdateTimeout);
                }
                catch(NullPointerException npe)
                {
                    Log.d(TAG, "testCameraVideoRotate: failed to delete video - attempting to re-open the film strip");
                    SystemClock.sleep(mShortDelayForObserver);
                    SystemClock.sleep(mShortDelayForObserver);
                    mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                            0, mDevice.getDisplayHeight() / 2,
                            100);

                    Log.d(TAG, "testCameraVideoRotate: trying to delete the video again");
                    SystemClock.sleep(mShortDelayForObserver);
                    uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
                    if(uiobj != null)
                    {
                        uiobj.click();
                        SystemClock.sleep(mUpdateTimeout);
                    }
                }

                uiobj = findUiObject2(By.res(shutterButtonPattern));
                if(uiobj == null)
                {
                    Log.d(TAG, "testCameraVideoRotate: the film strip is still open");
                    SystemClock.sleep(mUpdateTimeout);
                }
                else
                {
                    Log.d(TAG, "testCameraVideoRotate: opening the film strip");
                    SystemClock.sleep(mShortDelayForObserver);
                    mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2,
                            0, mDevice.getDisplayHeight() / 2,
                            100);
                    SystemClock.sleep(mUpdateTimeout);

                    Log.d(TAG, "testCameraVideoRotate: clicking through images...");
                    uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
                    uiscroll = new UiScrollable(uisel).setAsHorizontalList();
                    SystemClock.sleep(mUpdateTimeout);
                }

                Log.d(TAG, "testCameraVideoRotate: selecting the photo");
                uisel = new UiSelector().className(android.widget.ImageView.class.getName());
                uiscroll.getChildByInstance(uisel, 0).click();

                Log.d(TAG, "testCameraVideoRotate: admiring the photo for a bit...");
                SystemClock.sleep(mLongDelayForObserver);

                Log.d(TAG, "testCameraVideoRotate: deleting the photo");
                SystemClock.sleep(mShortDelayForObserver);
                uiobj = findUiObject2(By.res(Pattern.compile("(.*)(filmstrip_bottom_control_delete)")));
                uiobj.click();
                SystemClock.sleep(mUpdateTimeout);
            }
        }

        Log.d(TAG, "testCameraVideoRotate: enabling accelerometer rotation");
        EnableAccelerometerRotation();
        SystemClock.sleep(mShortDelayForObserver);

        Log.d(TAG, "testCameraVideoRotate: finished");
        SendSuccessStatus(testName, true);
    }

    // Non-Widi Test: Camera Image+Video Capture (multi res)
    public void testCameraVideoRotateMultiRes() throws UiObjectNotFoundException, RemoteException
    {
        final String    testName = "testCameraVideoRotateMultiRes";

        Log.d(TAG, "testCameraVideoRotateMultiRes: opening the camera...");
        if(!launchApp(CAMERA_PACKAGE))
        {
            Log.e(TAG, "testCameraVideoRotateMultiRes: failed to open the camera - trying the 'all apps' panel");
            if(!launchApp(CAMERA_PACKAGE, "Camera"))
            {
                Log.e(TAG, "testCameraVideoRotateMultiRes: failed to open the camera");
                SendNotRunStatus(testName);
                return;
            }
        }
        if(!cameraIsPresent())
        {
            Log.e(TAG, "testCameraVideoRotateMultiRes: the camera is not working");
            SendNotRunStatus(testName);
            SystemClock.sleep(mLongDelayForObserver);
            return;
        }

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testCameraVideoRotateMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testCameraVideoRotateMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testCameraVideoRotateMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testCameraVideoRotateMultiRes: selected mode %s, running testCameraPreviewRotate()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testCameraVideoRotate();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testCameraVideoRotateMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testCameraVideoRotateMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testCameraVideoRotateMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    private boolean selectCamera()
    {
        Pattern modeOptionsPattern = Pattern.compile("(.*)(mode_options_overlay)");
        UiObject2 uiobj;
        boolean retrying = false;

        uiobj = findUiObject2(By.res(modeOptionsPattern));
        do
        {
            if (uiobj == null)
            {
                Log.d(TAG, "selectCamera: the mode options overlay is closed - swiping it open");
                mDevice.swipe(0, mDevice.getDisplayHeight() / 2,
                        mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2,
                        100);
                uiobj = findUiObject2(By.res(modeOptionsPattern));
                if (uiobj == null)
                {
                    Log.d(TAG, "selectCamera: failed to open the mode options overlay");
                    return false;
                }
            }
            Log.d(TAG, String.format("SelectCamera: mode_options_overlay - children(%d) enabled(%c) focusable(%c) focused(%c)",
                    uiobj.getChildCount(), uiobj.isEnabled() ? 'y' : 'n', uiobj.isFocusable() ? 'y' : 'n', uiobj.isFocused() ? 'y' : 'n'));

            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.desc("Switch to Camera Mode"));
            if (uiobj == null)
            {
                Log.d(TAG, "selectCamera: failed to click on the camera selector");
                if(retrying)
                    return false;
                retrying = true;
            }
        }
        while(uiobj == null);
        SystemClock.sleep(mShortDelayForObserver);
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);
        return true;
    }

    private boolean selectVideo()
    {
        Pattern modeOptionsPattern = Pattern.compile("(.*)(mode_options_overlay)");
        UiObject2 uiobj;
        boolean retrying = false;

        uiobj = findUiObject2(By.res(modeOptionsPattern));
        do
        {
            if (uiobj == null)
            {
                Log.d(TAG, "selectVideo: the mode options overlay is closed - swiping it open");
                mDevice.swipe(0, mDevice.getDisplayHeight() / 2,
                        mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2,
                        100);
                uiobj = findUiObject2(By.res(modeOptionsPattern));
                if (uiobj == null)
                {
                    Log.d(TAG, "selectVideo: failed to open the mode options overlay");
                    return false;
                }
            }
            Log.d(TAG, String.format("selectVideo: mode_options_overlay - children(%d) enabled(%c) focusable(%c) focused(%c)",
                    uiobj.getChildCount(), uiobj.isEnabled() ? 'y' : 'n', uiobj.isFocusable() ? 'y' : 'n', uiobj.isFocused() ? 'y' : 'n'));

            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.desc("Switch to Video Camera"));
            if (uiobj == null)
            {
                Log.d(TAG, "selectVideo: failed to click on the video camera selector");
                if(retrying)
                    return false;
                retrying = true;
            }
        }
        while(uiobj == null);
        SystemClock.sleep(mShortDelayForObserver);
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);
        return true;
    }

    private boolean cameraIsPresent()
    {
        UiSelector uisel;
        UiObject uiobj1;
        UiObject2 uiobj;
        boolean success = true;

        Log.d(TAG, "cameraIsPresent: Camera - looking for camera error dialog");
        uisel = new UiSelector().resourceIdMatches("(.*)(alertTitle)");
        uisel.childSelector(new UiSelector().text("Camera error"));
        uiobj1 = mDevice.findObject(uisel);
        if(uiobj1.waitForExists(1000))
        {
            Log.d(TAG, "cameraIsPresent: Camera - found alert dialog");
            SystemClock.sleep(mShortDelayForObserver);
            uiobj = findUiObject2(By.text("OK"));
            if(uiobj != null)
                uiobj.click();
            success = false;
        }
        return success;
    }

    private boolean toggleCamera() throws NullPointerException
    {
        UiObject2 uiobj;

        Log.d(TAG, "toggleCamera: toggling mode/options button");
        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(mode_options_toggle)")), mShortUpdateTimeout);
        if (uiobj == null)
        {
            // not an error if it's already open - see if the camera toggle is visible, below
            Log.d(TAG, "toggleCamera: failed to find the mode/options toggle");
        }
        else
        {
            uiobj.click();
        }

        Log.d(TAG, "toggleCamera: toggling the front/back camera");
        uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(camera_toggle_button)")), mShortUpdateTimeout);
        if (uiobj == null)
        {
            Log.e(TAG, "toggleCamera: failed to find the camera toggle");
            return false;
        }
        uiobj.click();
        SystemClock.sleep(mUpdateTimeout);
        return true;
    }
}

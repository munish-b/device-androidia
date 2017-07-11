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

File Name:      camera.java

Description:    UiAutomator tests for the camera.

Environment:

Notes:

****************************************************************************/

package com.intel.hwcval;

import java.util.List;
import java.util.ListIterator;

import android.os.RemoteException;
import android.util.Log;
import android.graphics.Rect;


import com.android.uiautomator.core.UiDevice;
import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.core.UiCollection;
import com.android.uiautomator.core.UiScrollable;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;
import com.intel.hwcval.uiautomatorHelper;
import com.intel.hwcval.androidDialogs;

public class camera extends UiAutomatorTestCase
{
    private static final String TAG = uiautomatorHelper.TAG;

    public void testBasicCamera() throws RemoteException, UiObjectNotFoundException
    {
        final String testName = "testBasicCamera";
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel;
        UiObject        uiobj;
        Rect            bounds;
        UiScrollable    uiscroll;
        boolean         success = true;

        Log.d(TAG, "testBasicCamera: locking device");
        uiautomatorHelper.lockDevice(this);

        if(uiautomatorHelper.isDeviceLocked(this))
        {
            Log.d(TAG, "testBasicCamera: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        uiautomatorHelper.gotoApps(this);
        if(!uiautomatorHelper.launchApp(this, "Camera"))
        {
            Log.d(TAG, "testBasicCamera: failed to open the camera");
            success = false;
        }
        else
        {
            Log.d(TAG, "testBasicCamera: looking for camera error dialog");
            uisel = new UiSelector().resourceIdMatches("(.*)(alertTitle)");
            uisel.childSelector(new UiSelector().text("Camera error"));
            uiobj = new UiObject(uisel);
            if(uiobj.waitForExists(1000))
            {
                Log.d(TAG, "testBasicCamera: aborting test - there doesn't appear to be a camera");
                uiautomatorHelper.SendNotRunStatus(this, testName);
                sleep(1000);
                uisel = new UiSelector().text("OK");
                uiobj = new UiObject(uisel);
                uiobj.click();
                return;
            }
        }

        if(success)
        {
            selectCamera();
            Log.d(TAG, "testBasicCamera: taking a photo");
            sleep(2000);
            uisel = new UiSelector().resourceIdMatches("(.*)(shutter_button)");
            uiobj = new UiObject(uisel);
            uiobj.click();

            selectVideo();
            Log.d(TAG, "testBasicCamera: making a 10 second video");
            sleep(2000);
            uisel = new UiSelector().resourceIdMatches("(.*)(shutter_button)");
            uiobj = new UiObject(uisel);
            uiobj.click();
            sleep(10000);
            uiobj.click();

            Log.d(TAG, "testBasicCamera: opening the film strip");
            sleep(2000);
            uisel = new UiSelector().resourceIdMatches("(.*)(mode_options_overlay)");
            uiobj = new UiObject(uisel);
            bounds = uiobj.getBounds();
            uidev.swipe(bounds.width(), bounds.height()/2, bounds.width()/2, bounds.height()/2, 100);

            Log.d(TAG, "testBasicCamera: clicking through images...");
            uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_view)");
            uiscroll = new UiScrollable(uisel).setAsHorizontalList();
            uisel = new UiSelector().className(android.widget.ImageView.class.getName());

            Log.d(TAG, "testBasicCamera: selecting the video");
            uiobj = uiscroll.getChildByInstance(uisel, 0);
            uiobj.click();

            // if it's the first time the Camera activity has been asked to show
            // a video we'll get a resolver dialog
            androidDialogs dialogHelper = new androidDialogs();
            dialogHelper.HandleResolverDialog("Photos", "Always");

            uisel = new UiSelector().resourceIdMatches("(.*)(videoplayer)");
            uiobj = new UiObject(uisel);
            if(uiobj.exists())
            {
                Log.d(TAG, "testBasicCamera: the video is playing...");
                while(uiobj.exists() && uiobj.isFocused())
                {
                    Log.d(TAG, "testBasicCamera: waiting for the video to finish...");
                    sleep(1000);
                }
            }

            Log.d(TAG, "testBasicCamera: deleting the video");
            sleep(1000);
            uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_bottom_control_delete)");
            uiobj = new UiObject(uisel);
            uiobj.click();

            Log.d(TAG, "testBasicCamera: selecting the photo");
            uisel = new UiSelector().className(android.widget.ImageView.class.getName());
            uiobj = uiscroll.getChildByInstance(uisel, 0);
            uiscroll.scrollIntoView(uiobj);
            uiobj.click();
            sleep(5000);

            Log.d(TAG, "testBasicCamera: deleting the photo");
            sleep(1000);
            uisel = new UiSelector().resourceIdMatches("(.*)(filmstrip_bottom_control_delete)");
            uiobj = new UiObject(uisel);
            uiobj.click();
        }

        Log.d(TAG, "testBasicCamera: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    public void testBasicCameraMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String            testName = "testBasicCameraMultiRes";
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
            Log.d(TAG, "testBasicCameraMultiRes: unlocking device");
            uiautomatorHelper.unlockDevice(this);
        }

        uiautomatorHelper.gotoApps(this);
        if(!uiautomatorHelper.launchApp(this, "Camera"))
        {
            Log.d(TAG, "testBasicCameraMultiRes: failed to open the camera");
            success = false;
        }
        else
        {
            Log.d(TAG, "testBasicCameraMultiRes: looking for camera error dialog");
            uisel = new UiSelector().resourceIdMatches("(.*)(alertTitle)");
            uisel.childSelector(new UiSelector().text("Camera error"));
            uiobj = new UiObject(uisel);
            if(uiobj.waitForExists(1000))
            {
                Log.d(TAG, "testBasicCameraMultiRes: aborting test - there doesn't appear to be a camera");
                uiautomatorHelper.SendNotRunStatus(this, testName);
                sleep(1000);
                uisel = new UiSelector().text("OK");
                uiobj = new UiObject(uisel);
                uiobj.click();
                return;
            }
        }

        Log.d(TAG, "testBasicCameraMultiRes: opening HDMI settings");
        uiautomatorHelper.openSettings(this, "HDMI");

        uisel = new UiSelector().text("HDMI Connected");
        uiobj = new UiObject(uisel);
        if(!uiobj.exists())
        {
            Log.d(TAG, "testBasicCameraMultiRes: HDMI not connected - aborting test");
            uiautomatorHelper.SendNotRunStatus(this, testName);
            return;
        }
        else
        {
            Log.d(TAG, "testBasicCameraMultiRes: HDMI connected");

            originalModeString = uiautomatorHelper.getSelectedHDMIMode(this);
            modeList = uiautomatorHelper.getHDMIModeList(this);
            modeListIter = modeList.listIterator();

            while(modeListIter.hasNext())
            {
                modeString = modeListIter.next();
                Log.d(TAG, "testBasicCameraMultiRes: mode " + modeString);
                if(uiautomatorHelper.selectHDMIMode(this, modeString))
                {
                    Log.d(TAG, "testBasicCameraMultiRes: selected mode successfully, running testNotification...");
                    sleep(3000);
                    testBasicCamera();
                    sleep(3000);

                    if(++modesTested == 3)
                    {
                        Log.d(TAG, "testBasicCameraMultiRes: tested " + modesTested + " modes - that's quite enough");
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
                            Log.d(TAG, "testBasicCameraMultiRes: HDMI unplugged - aborting test");
                            success = false;
                            break;
                        }
                    }
                }
                else
                {
                    Log.d(TAG, "testBasicCameraMultiRes: failed to select mode " + modeString);
                }
            }
            uiautomatorHelper.openSettings(this, "HDMI");
            uiautomatorHelper.selectHDMIMode(this, originalModeString);
        }

        Log.d(TAG, "testBasicCameraMultiRes: finished");
        uiautomatorHelper.SendSuccessStatus(this, testName, success);
    }

    private void selectCamera() throws UiObjectNotFoundException
    {
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel;
        UiObject        uiobjOptions;
        UiObject        uiobj;
        Rect            bounds;

        Log.d(TAG, "selectCamera: selecting the camera");
        uisel = new UiSelector().resourceIdMatches("(.*)(mode_options_overlay)");
        uiobjOptions = new UiObject(uisel);
        bounds = uiobjOptions.getBounds();
        uisel = new UiSelector().description("Switch to Camera Mode");
        uiobj = new UiObject(uisel);
        if(uiobj.exists())
        {
            // try clicking it - if it really exists it will go away, otherwise
            // it will do no harm
            Log.d(TAG, "selectCamera: looks like it exists, clicking on it");
            uiobj.click();
            sleep(1000);
        }
        Log.d(TAG, "selectCamera: swiping for the options menu");
        uidev.swipe(0, bounds.height()/2, bounds.width()/2, bounds.height()/2, 100);
        Log.d(TAG, "selectCamera: clicking on it");
        sleep(1000);
        uiobj.click();
    }

    private void selectVideo() throws UiObjectNotFoundException
    {
        UiDevice        uidev = getUiDevice();
        UiSelector      uisel;
        UiObject        uiobjOptions;
        UiObject        uiobj;
        Rect            bounds;

        Log.d(TAG, "selectVideo: selecting the video");
        uisel = new UiSelector().resourceIdMatches("(.*)(mode_options_overlay)");
        uiobjOptions = new UiObject(uisel);
        bounds = uiobjOptions.getBounds();
        uisel = new UiSelector().description("Switch to Video Camera");
        uiobj = new UiObject(uisel);
        if(uiobj.exists())
        {
            // try clicking it - if it really exists it will go away, otherwise
            // it will do no harm
            Log.d(TAG, "selectVideo: looks like it exists, clicking on it");
            uiobj.click();
            sleep(1000);
        }
        Log.d(TAG, "selectVideo: swiping for the options menu");
        uidev.swipe(0, bounds.height()/2, bounds.width()/2, bounds.height()/2, 100);
        Log.d(TAG, "selectVideo: clicking on it");
        sleep(1000);
        uiobj.click();
    }
}

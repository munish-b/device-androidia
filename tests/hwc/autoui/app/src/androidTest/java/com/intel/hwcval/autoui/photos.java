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

 File Name:     photos.java

 Description:   tests using the bundled Photos app which supersedes Gallery.

 Environment:

 Notes:         Photos is part of the Google+ package which will load when
                Photos is started and try to log-in to a Google account. The
                Google+ activity may be run in the foreground, necessitating
                a switch via Recent Apps (the launchApp helper function does
                this for us).

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.graphics.Point;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;

import java.util.regex.Pattern;


public class photos extends uiautomatorHelper
{
    // ETM X-Component test: TOUCH_GESTURE
    public void testPhotos() throws RemoteException, UiObjectNotFoundException
    {
        final String testName = "testPhotos";
        UiObject2 uiobj;
        UiSelector uisel;
        Point p1Start, p1End, p2Start, p2End;
        UiObject uiObjGesture;
        boolean success = true;

        Log.d(TAG, "testPhotos: starting photos app...");
        if(!launchApp(PHOTOS_PACKAGE, "Photos"))
        {
            Log.d(TAG, "testPhotos: failed to find the Photos app");
            SendNotRunStatus(testName);
            return;
        }

        uiobj = waitUiObject2(By.res("com.google.android.apps.plus:id/title"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "testPhotos: closing the menu");
            tapScreen();
        }

        uiobj = waitUiObject2(By.res("com.google.android.apps.plus:id/promo_signin_not_now_btn"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "testPhotos: selecting 'NO THANKS' for cloud backup offer");
            uiobj.click();
        }

        // navigate to the photos list
        do
        {
            uiobj = waitUiObject2(By.desc("Navigate up"), mShortUpdateTimeout);
            if(uiobj != null)
            {
                Log.d(TAG, "testPhotos: navigating to photos list...");
                uiobj.click();
                mDevice.waitForWindowUpdate(PHOTOS_PACKAGE, mShortUpdateTimeout);
            }
            uiobj = waitUiObject2(By.res(Pattern.compile("(.*)(action_bar)")).hasChild(By.text("Photos")), mShortUpdateTimeout);
            if(uiobj != null)
            {
                Log.d(TAG, "testPhotos: found photos list");
                break;
            }
        }
        while(uiobj != null);

        uiobj = findUiObject2(By.res(Pattern.compile("(.*)(action_bar)")).hasChild(By.text("Photos")));
        if(uiobj == null)
        {
            Log.d(TAG, "testPhotos: not in photos list (may be in trash). Trying to navigate to Photos");
            uiobj = findUiObject2(By.desc("Open navigation drawer"));
            uiobj.click();
            uiobj = waitUiObject2(By.text(("Photos")), mShortUpdateTimeout);
            uiobj.click();
        }

        // all photos share the same description - we don't mind which we pick
        try
        {
            Log.d(TAG, "testPhotos: opening a photo...");
            uiobj = waitUiObject2(By.desc("Photo"), mUpdateTimeout);
            uiobj.click();
            mDevice.waitForWindowUpdate(PHOTOS_PACKAGE, mUpdateTimeout);
        }
        catch (NullPointerException npe)
        {
            Log.e(TAG, "testPhotos: there are no photos ");
            success = false;
        }

        if (success)
        {
            // double tap to zoom
            Log.d(TAG, "testPhotos: zooming...");
            SystemClock.sleep(1000);
            mDevice.click(mDevice.getDisplayWidth() / 2, 200);
            SystemClock.sleep(200);
            mDevice.click(mDevice.getDisplayWidth() / 2, 200);
            SystemClock.sleep(200);

            uisel = new UiSelector().resourceIdMatches("(.*)(photo_view_pager)");
            uiObjGesture = mDevice.findObject(uisel);

            p1Start = new Point(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2 - 50);
            p2Start = new Point(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2 + 50);
            p1End = new Point(mDevice.getDisplayWidth() / 2, 50);
            p2End = new Point(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() - 50);
            Log.d(TAG, "testPhotos: zooming in...");
            uiObjGesture.performTwoPointerGesture(p1Start, p2Start, p1End, p2End, 100);
            Log.d(TAG, "testPhotos: zooming in...");
            uiObjGesture.performTwoPointerGesture(p1Start, p2Start, p1End, p2End, 100);

            Log.d(TAG, "testPhotos: scrolling left...");
            mDevice.swipe(mDevice.getDisplayWidth()/4, mDevice.getDisplayHeight()/2,
                    3 * mDevice.getDisplayWidth()/4, mDevice.getDisplayHeight()/2, 100);

            Log.d(TAG, "testPhotos: scrolling up...");
            mDevice.swipe(mDevice.getDisplayWidth()/2, mDevice.getDisplayHeight()/4,
                    mDevice.getDisplayWidth()/2, 3*mDevice.getDisplayHeight()/4, 100);

            Log.d(TAG, "testPhotos: scrolling right...");
            mDevice.swipe(3*mDevice.getDisplayWidth()/4, mDevice.getDisplayHeight()/2,
                    mDevice.getDisplayWidth()/4,   mDevice.getDisplayHeight()/2, 100);

            Log.d(TAG, "testPhotos: scrolling down...");
            mDevice.swipe(mDevice.getDisplayWidth()/2, (int)3*mDevice.getDisplayHeight()/4,
                    mDevice.getDisplayWidth()/2, (int)mDevice.getDisplayHeight()/4, 100);

            Log.d(TAG, "testPhotos: zooming out...");
            p1Start = new Point(100, mDevice.getDisplayHeight()/2);
            p2Start = new Point(mDevice.getDisplayWidth() - 100, mDevice.getDisplayHeight()/2);
            p1End = new Point(mDevice.getDisplayWidth()/2, mDevice.getDisplayHeight()/2);
            p2End = new Point(mDevice.getDisplayWidth()/2, mDevice.getDisplayHeight()/2);
            uiObjGesture.performTwoPointerGesture(p1Start, p2Start, p1End, p2End, 100);
        }

        Log.d(TAG, "testPhotos: finished");
        SendSuccessStatus(testName, success);
    }
}

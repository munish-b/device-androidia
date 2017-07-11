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

 File Name:     gfxbench.java

 Description:   UI Automator works with elements in the Accessibility library.
 This library describes all the elements of a screen, apart from those who are
 default and cannot be modified (i.e. status bar & co.). They are called
 NAF = Not Accessibility Friendly. This class is an example of app with
 NAF controls. When this happens it is necessary to use different formulas to
 find the area in the screen where the mouse click should generate an action.

 Environment:

 ****************************************************************************/

package com.intel.hwcval.autoui;

import android.graphics.Point;
import android.graphics.Rect;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiCollection;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class gfxbench extends uiautomatorHelper
{
    // ETM X-Component test: Check_app_GLBenchMark_2.7 (single res) updated for
    // the latest version of the benchmark
    public void testManhattan() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testManhattan";
        final String    benchmark = "Manhattan ES 3.1";
        UiObject2       uiobj;
        int             run;
        boolean         success = true;

        success = launchApp(GFXBENCH_PACKAGE);
        for(run = 0; run < 5 && success; ++run)
        {
            success = selectBenchmark(benchmark);
            if(!success)
            {
                Log.e(TAG, String.format("testManhattan - failed to select benchmark '%s'", benchmark));
            }

            SystemClock.sleep(mUpdateTimeout);
            uiobj = findUiObject2(By.text("Start"));
            if(uiobj == null)
            {
                Log.e(TAG, "testManhattan - failed to start benchmark");
                success = false;
            }
            else
            {
                Log.d(TAG, String.format("testManhattan - starting benchmark run %d...", run));
                uiobj.click();
            }

            uiobj = waitUiObject2(By.res("net.kishonti.gfxbench.gl:id/results_navbar"), 120 * 1000);
            if(uiobj == null)
            {
                Log.e(TAG, "testManhattan: benchmark didn't complete in time - aborting");
                success = false;
            }
            else
            {
                Log.d(TAG, String.format("testManhattan - benchmark run %d completed", run));
                mDevice.pressBack();
            }
        }

        Log.d(TAG, "testManhattan: finished");
        SendSuccessStatus(testName, success);
    }

    // ETM X-Component test: Check_app_GLBenchMark_2.7 (multi res) updated for
    // the latest version of the benchmark
    public void testManhattanMultiRes() throws RemoteException, UiObjectNotFoundException
    {
        final String    testName = "testManhattanMultiRes";

        launchApp(GFXBENCH_PACKAGE);

        String originalMode;
        List<String> modeList = new ArrayList<>(Arrays.asList("1600*1200P@60Hz", "1280*1024P@60Hz"));
        List<String> availableModes = getHDMIModeList();

        originalMode = getSelectedHDMIMode();
        for(String mode : modeList)
        {
            boolean modeFound = false;
            Log.d(TAG, String.format("testManhattanMultiRes: looking for mode %s", mode));
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
                Log.d(TAG, String.format("testManhattanMultiRes: found mode %s, selecting it", mode));
                if(!selectHDMIMode(mode))
                {
                    Log.e(TAG, String.format("testManhattanMultiRes: failed to select mode %s", mode));
                }
                else
                {
                    Log.d(TAG, String.format("testManhattanMultiRes: selected mode %s, running testCameraPlaybackPicVideo()...", mode));
                    SystemClock.sleep(mLongDelayForObserver);
                    testManhattan();
                    SystemClock.sleep(mLongDelayForObserver);
                }
            }
            else
            {
                Log.d(TAG, String.format("testManhattanMultiRes: mode %s not found, skipping it", mode));
            }
        }

        Log.d(TAG, "testManhattanMultiRes: resetting the original mode");
        selectHDMIMode(originalMode);

        Log.d(TAG, "testManhattanMultiRes: finished");
        SendSuccessStatus(testName, true);
    }

    private boolean selectBenchmark(String benchmark) throws UiObjectNotFoundException
    {
        UiObject2       uiobj;
        UiSelector      uisel;
        UiScrollable    uiscroll;
        UiObject        uio;
        Rect            bounds;
        Point           testSelection = new Point();
        boolean         foundBenchmark = false;
        boolean         success = true;

        uiobj = waitUiObject2(By.text("Test selection"), mShortUpdateTimeout);
        if(uiobj != null)
        {
            Log.d(TAG, "selectBenchmark - already in test selection screen");
        }
        else
        {
            Log.d(TAG, "selectBenchmark - on the home screen");

            // circleControl is NAF = this can be seen in the UI Automator Viewer in the "index" field.
            // -> the ony thing known is that there is a rectangle with bounds-> need to figure out where to click
            uiobj = waitUiObject2(By.res("net.kishonti.gfxbench.gl:id/main_circleControl"), mShortUpdateTimeout);
            bounds = uiobj.getVisibleBounds();
            Log.d(TAG, String.format("selectBenchmark - main_circleControl height, width %d, %d", bounds.height(), bounds.width()));
            Log.d(TAG, String.format("selectBenchmark - main_circleControl bottom, right %d, %d", bounds.bottom, bounds.right));

            // testSelection circle is 23% of the width of the circleControl and 36% of its height.
            // It is padded 13% from the right edge and 5% from the bottom
            testSelection.x = bounds.right - (int) ((bounds.width() * 0.23f) / 2 + bounds.width() * 0.13f);
            testSelection.y = bounds.bottom - (int) ((bounds.height() * 0.36f) / 2 + bounds.height() * 0.05f);
            Log.d(TAG, String.format("selectBenchmark - testSelection circle centre is %d, %d", testSelection.x, testSelection.y));
            mDevice.click(testSelection.x, testSelection.y);
            SystemClock.sleep(mUpdateTimeout);

            uiobj = waitUiObject2(By.text("Test selection"), mUpdateTimeout);
            if(uiobj == null)
            {
                Log.e(TAG, "selectBenchmark - failed to open test selection screen");
                return false;
            }
        }

        Log.d(TAG, "selectBenchmark - deselecting tests");
        uisel = new UiSelector().resourceIdMatches("net.kishonti.gfxbench.gl:id/main_testSelectListView");
        uiscroll = new UiScrollable(uisel).setAsVerticalList();

        uisel = new UiSelector().className(android.widget.TextView.class.getName());
        uio = uiscroll.getChildByText(uisel, "High-Level Tests", true);
        uisel = new UiSelector().className(android.widget.CheckBox.class.getName());
        uio = uio.getFromParent(uisel);
        if(uio.isChecked())
        {
            Log.d(TAG, "selectBenchmark - disabling High-Level Tests");
            uio.click();
        }

        uisel = new UiSelector().className(android.widget.TextView.class.getName());
        uio = uiscroll.getChildByText(uisel, "Low-Level Tests", true);
        uisel = new UiSelector().className(android.widget.CheckBox.class.getName());
        uio = uio.getFromParent(uisel);
        if(uio.isChecked())
        {
            Log.d(TAG, "selectBenchmark - disabling Low-Level Tests");
            uio.click();
        }

        uisel = new UiSelector().className(android.widget.TextView.class.getName());
        uio = uiscroll.getChildByText(uisel, "Special Tests", true);
        uisel = new UiSelector().className(android.widget.CheckBox.class.getName());
        uio = uio.getFromParent(uisel);
        if(uio.isChecked())
        {
            Log.d(TAG, "selectBenchmark - disabling Special Tests");
            uio.click();
        }

        uisel = new UiSelector().className(android.widget.TextView.class.getName());
        uio = uiscroll.getChildByText(uisel, "Battery Test", true);
        uisel = new UiSelector().className(android.widget.CheckBox.class.getName());
        uio = uio.getFromParent(uisel);
        if(uio.isChecked())
        {
            Log.d(TAG, "selectBenchmark - disabling Battery Test");
            uio.click();
        }

        // it's not obvious how to associate an anonymous checkbox with a test name, but what we do
        // know is that the checkbox following a test enables/disables it; therefore, search for the
        // test name first, then locate the first checkbox which follows it.
        Log.d(TAG, String.format("selectBenchmark - selecting '%s' test", benchmark));
        uiscroll.flingToBeginning(1);
        uisel = new UiSelector();
        try
        {
            success = false;
            for(int child = 0; !success; ++child)
            {
                uio = uiscroll.getChildByInstance(uisel, child);
                if(uio == null)
                    break;

                String strText = uio.getText();
                String strClass = uio.getClassName();

                Log.d(TAG, String.format("selectBenchmark - traversing scroller. UiObject text(%s) class(%s)", strText, strClass));

                if(!foundBenchmark)
                {
                    if(strText.equals(benchmark))
                    {
                        Log.d(TAG, String.format("selectBenchmark - found %s", strText));
                        foundBenchmark = true;
                    }
                }
                else
                {
                    if(strClass.equals(android.widget.CheckBox.class.getName()))
                    {
                        Log.d(TAG, String.format("selectBenchmark - found %s", strClass));
                        if(!uio.isChecked())
                            uio.click();
                        success = true;
                    }
                }
            }
        }
        catch(UiObjectNotFoundException nfe)
        {
            ; // done
        }
        return success;
    }
}

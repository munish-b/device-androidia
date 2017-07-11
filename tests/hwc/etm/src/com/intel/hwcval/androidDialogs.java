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

File Name:      androidDialogs.java

Description:    UiAutomator functions for system dialogs.

Environment:

Notes:

****************************************************************************/

package com.intel.hwcval;

import android.util.Log;

import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;
import com.intel.hwcval.uiautomatorHelper;

public class androidDialogs extends UiAutomatorTestCase
{
    private static final String TAG = uiautomatorHelper.TAG;

    // when a package is installed Android can open a dialog -
    //
    // "Allow Google to regularly check device activity for security problems,
    //  and prevent or warn about potential harm."
    // "Learn more in the Google Settings app."
    //
    // This function checks for the dialog and closes it
    //
    public void checkActivity() throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        Log.d(TAG, "checkActivity: checking for vending package dialog");
        uisel = new UiSelector().resourceId("com.android.vending:id/action_bar_root");
        uiobj = new UiObject(uisel);
        if(uiobj.waitForExists(5000))
        {
            Log.d(TAG, "checkActivity: found it - clicking DECLINE");
            uisel = new UiSelector().text("DECLINE");
            uiobj = new UiObject(uisel);
            uiobj.clickAndWaitForNewWindow();
        }
    }

    public void HandleResolverDialog(String option, String action) throws UiObjectNotFoundException
    {
        UiSelector  uisel;
        UiObject    uiobj;

        Log.d(TAG, "HandleResolverDialog: checking for resolver dialog");
        sleep(1000);
        uisel = new UiSelector().classNameMatches("(.*)(ResolverDrawerLayout)");
        uiobj = new UiObject(uisel);
        if(uiobj.waitForExists(5000))
        {
            Log.d(TAG, "HandleResolverDialog: found it - selecting '" + option + "'");
            uisel = new UiSelector().text(option);
            uiobj = new UiObject(uisel);
            uiobj.click();

            Log.d(TAG, "HandleResolverDialog: selecting '" + action + "'");
            uisel = new UiSelector().text(action);
            uiobj = new UiObject(uisel);
            uiobj.click();
        }
    }
}

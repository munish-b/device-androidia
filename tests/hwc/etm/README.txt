Building and running the UiAutomator ETM tests
==============================================
For more information: http://developer.android.com/tools/testing/testing_ui.html


(0_a) Proxy settings:
create a file called androidtool.cfg containing the following two lines
http.proxyHost=proxy-chain.intel.com
http.proxyPort=911
And put this file into $HOME/.android directory.


(0_b) Check your jdk and javac version: you need to use version 7.
sudo update-alternatives --config java
sudo update-alternatives --config javac


(1) Install Android Studio
---------------------------
http://developer.android.com/sdk/index.html

This is used to run the tests on the target device.


(2) Update the Android SDK to get UiAutomator 2.0 support
-----------------------------------------------------------
https://developer.android.com/tools/testing-support-library/index.html

Additional Information:
http://developer.android.com/training/testing/ui-testing/index.html
http://developer.android.com/training/testing/ui-testing/uiautomator-testing.html


(3) Check Android SDK Manager configuration via Auto_UI_sdk_1/2.png images
--------------------------------------------------------------------------
Note1: it may happen that AutoUI is still set up on the wrong Java version.
In that case, change it via File -> Project Structure.
Note2: If you get "OpenJDK shows intermittent performance and UI issues.
We recommend using the Oracle JRE/JDK" -> click on "don't show again"


(4) Create the UI Test project
------------------------------
Go to the hwcval etm directory (i.e. the location of this README.txt file).
Create the test project and build it -

$ $ANDROID_HOME/tools/android create uitest-project -n etm -t 1 -p .


(5) Build the JAR binary
------------------------
From the etm directory -

$ ant build


(6) Install the JAR binary
--------------------------
Push it to the hwc validation directory. From the etm directory -

$ adb push bin/etm.jar /data/validation/hwc/etm.jar


(6) Run the UI Automator tests
------------------------------
$ adb shell uiautomator runtest <jar> [-c <packageName>[.<className>][#<methodname>]]]

e.g.

# run all tests...
$ adb shell uiautomator runtest /data/validation/hwc/etm.jar

# run one test...
$ adb shell uiautomator runtest /data/validation/hwc/etm.jar -c com.intel.hwcval.notificationShade

# run one method...
$ adb shell uiautomator runtest /data/validation/hwc/etm.jar -c com.intel.hwcval.notificationShade#testNotification


(6) Logcatting the tests
------------------------
To reduce output verbosity -

$ adb logcat -s com.intel.hwcval.uiautomator QueryController


(7) Open AutoUI project from AndroidStudio and run it (can run individual test cases)
---------------------------------------------------------------------------------------
Proxy -> need to setup proxy!!
File -> Open -> <HWCVAL>/tests/hwc/autoui
autoui-> app -> sr -> androidTest -> java -> com -> intel -> hwcval -> autoui -> notificationShade


(8) Launch Android Studio (AutoUI)
------------------------------------
* cd <android-studio-path>/bin
* ./studio.sh
* click on project in the vertical middle bar on the left
* got to java -> com.intel-hwcval.autoui -> notificationShade
* click on testNotification -> run -> right click -> littledroid
* set a breakdown = click on column left
* bug symbol = rerun in debug mode
* arrow symbol = rerun no debug mode (= ignores breakpoints)


(9) How to launch test via Android Studio command line
-------------------------------------------------------
To run all tests in a class:
export HWCVAL_ETM_CLASS=<name_class>  -> run all tests in this class
./runOnDevice

To run only one test in a class:
export HWCVAL_ETM_CLASS=<name_class>  -> run all tests in this class
export HWCVAL_ETM_METHOD=<name_test>  -> run only this test
./runOnDevice

NOTE1: Android Studio command line does NOT build tests -> need to have run them
first in Android Studio GUI so that they have been compiled.
NOTE2: Android Studio Compilation = generation of apks.

**TODO**
find a way to compile from command line. (must be a gradle command. i.e. "gradle bla bla ")


(10) How to use UI Automator Viewer
---------------------------------------------
adb devices -> do I have a device connected?
adb root
adb remount
cd $HOME/Android/Sdk/tools/
./uiautomatorviewer
-> second icon from the left to view tablet screen
-> second icon from the left to update view

Every time I click twice on an element in the screen I see a red border and his properties are listed.
If there are no properties, that means it is not part of the display but of the default interface
(i.e. status bar ). The properties listed are used in the auotui to check values to progress with the tests.


(11) Tests structure
<test_name>
<test_name>MultiRes -> external display/HDMI version of the test.



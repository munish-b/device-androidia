# Jenkins build script platform specific includes

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 20th September 2015

# Author: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 4th January 2017

# Description: Jenkins script to define platform specific settings
#              Depends heavily on parameter JENKINS_PLATFORM being defined by Jenkins Job config

if [[ $JENKINS_PLATFORM == "CHT" ]]
then
  echo "Configuring environment for Cherrytrail (mainline)"

  export TOP=$WORKSPACE/build/android_1andr_m_010416
  export P4_ROOT=$WORKSPACE/build/ufo_main_040416/gfx_Development
  export BUILD_VARIANT=userdebug
  export JENKINS_JOBS="Hardware_Composer_Builder_CHT Hardware_Composer_Validation_Builder_CHT"

  export ANDROID_SERIAL=SPC342400077
  export REBOOT_CMD="$ADB reboot"
  export STATUS_CMD="$ADB shell getprop init.svc.surfaceflinger | dos2unix"
  #export REBOOT_CMD="device-ctl -b /usr2/jenkins/bin/cht_rvp_device_config.ini reboot 2> /dev/null"
  #export STATUS_CMD="device-ctl -b /usr2/jenkins/bin/cht_rvp_device_config.ini status"
  export STATUS_OK="ONLINE"

  # Common CHT definitions
  export PRODUCT=r2_cht_rvp

elif [[ $JENKINS_PLATFORM == "BXTP" ]]
then
  echo "Configuring environment for Broxton P, Gordon Ridge system"
  export TOP=$WORKSPACE/build/android
  export P4_ROOT=$WORKSPACE/build/ufo/gfx_Development
  export BUILD_VARIANT=eng
  export JENKINS_JOBS="Hardware_Composer_Builder_BXTP Hardware_Composer_Validation_Builder_BXTP"

  # Setup target
  export PRODUCT=bxtp_abl
  #export ANDROID_SERIAL=R1J56L2d702c8e
  export ANDROID_SERIAL=R1J56Lc58f996e
  export REBOOT_CMD="sudo echo r > /dev/ttyUSB2; sleep 1; sudo echo n1# > /dev/ttyUSB2"
  #export REBOOT_CMD="$ADB reboot"
  export STATUS_CMD="$ADB shell getprop init.svc.surfaceflinger | dos2unix"
  export STATUS_OK="running"
  if [[ -e /dev/ttyUSB2 ]]
  then
    sudo chmod 666 /dev/ttyUSB2
  fi
else
  echo "Skipping environment configuration $JENKINS_PLATFORM"
fi

# Common definitions
export NEW_ANDROID_CENTRAL_PATH=$HOME/android-trees/$NEW_ANDROID

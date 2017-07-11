#!/usr/bin/env bash

# Author: Robert Pinsker
# Email: robert.pinsker@intel.com
# Date: 25th February 2016
# Description: Post-run script to gather logs if we got a build timeout

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"

cd $WORKSPACE/$BUILD_NUMBER
if [[ "`grep \"Build timed out\" log`" == "" ]]
then
    exit 0
fi

# If the build timed out, then set the verified flag to -1 in Gerrit
if [[ $GERRIT_REFSPEC =~ "refs/changes/" ]]
then
    echo "Build Timed Out ... Updating Gerrit Verified Flag to -1"
    GERRIT_CHANGE_NUMBER=`echo $GERRIT_REFSPEC | awk -F '/' '{ print $(NF-1) }'`
    GERRIT_PATCHSET_NUMBER=`echo $GERRIT_REFSPEC | awk -F '/' '{ print $NF }'`

    ssh -p 29418 sys_hwcjenkins@vpg-git.iind.intel.com gerrit review --verified="-1" \
        \"--message="$1 timed out"\" \
        $GERRIT_CHANGE_NUMBER,$GERRIT_PATCHSET_NUMBER
fi

echo "Initiating recovery..."
source ~/bin/common.sh

# Setup Android build environment
cd $TOP
source build/envsetup.sh
lunch $PRODUCT-eng

# Setup environment for valhwch
export CLIENT_LOGS=$UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER
export HWCVAL_ROOT=$OLD_WORKSPACE/$BUILD_NUMBER/artifacts/val_hwc
export PATH=$HWCVAL_ROOT/host_scripts:$PATH
export HWCVAL_TARGET_DIR=/data/validation/hwc

# Pull the logs from the target
# Don't let this take longer than 10 minutes
echo "Recovering logs..."
timeout 600 valhwch -recover_logs

# If log recovery times out or fails, reboot and try again
if [[ $? -eq 124 ]] || [[ $? -eq 125 ]]
then
    echo "Rebooting..."
    eval $REBOOT_CMD
    $ADB wait-for-device
    echo "Second attempt to recover logs..."
    timeout 600 valhwch -recover_logs
fi

# Report log locations
LOG_URL="${UPSTREAM_JOB_URL%?}/ws/$UPSTREAM_BUILD_NUMBER"
echo "Last test timed out."
echo "hwclog: $LOG_URL/hwclog_hwch.log"
echo "logcat: $LOG_URL/logcat_hwch.log"
echo "kmsg  : $LOG_URL/kmsg_hwch.log"


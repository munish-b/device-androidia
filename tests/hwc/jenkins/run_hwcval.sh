#!/usr/bin/env bash

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 20th May 2015
# Version: 2.0
# Description: Jenkins script to run the HWC validation suite

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"
source ~/bin/common.sh

# Add a link (to this job) into the upstream builder log
if [[ -e $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/log ]]
then
  sed -i "/Change URL: .*$/a Runner console: ${BUILD_URL}console" $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/log
fi

# Setup Android build environment
cd $TOP
source build/envsetup.sh
lunch $PRODUCT-eng

# Setup HWCVAL build environment (for valpush etc.)
cd $WORKSPACE/build/hwcval/tests/hwc
source init.sh

# Reboot the board
echo "Rebooting board ..."
reboot_and_wait
$ADB shell rm -rf /data/logs/*
if [[ "`valhwcbackup compare`" != "" ]]
then
  echo "valhwcbackup compare shows target has shims installed, cleaning"
  $ADB shell stop
  valclean target
  eval $REBOOT_CMD
  $ADB wait-for-device
fi
echo "Reboot Successful"

# If this is running on the FFRD8, check the battery level and sleep if its less than 40%.
if [[ $JENKINS_PLATFORM == "BYT" ]]
then
  CHARGE_STR=`$ADB shell cat /sys/class/power_supply/max17047_battery/capacity`
  CHARGE=${CHARGE_STR::-1}
  while [[ $CHARGE -lt 40 ]]
  do
    echo "Battery level at $CHARGE%. Sleeping for 10 minutes to charge ..."
    sleep 600

    CHARGE_STR=`$ADB shell cat /sys/class/power_supply/max17047_battery/capacity`
    CHARGE=${CHARGE_STR::-1}
  done
else # Running on an RVP
    CHARGE="a/c"
fi

# Look for a test set override
if [[ -e $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/test_set.override ]]
then
  JENKINS_TEST_SET=`cat $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/test_set.override`
  echo "Overridden default Test Set with value: $JENKINS_TEST_SET"
fi

# Run the tests.
if [[ -z $JENKINS_TEST_SET ]]
then
  echo "Running default tests on $ANDROID_SERIAL (power level: $CHARGE)."
else
  echo "Running $JENKINS_TEST_SET tests on $ANDROID_SERIAL (power level: $CHARGE)."
fi

# Now wait for reboot to finish
wait_for_boot_complete

# The target is ready - clean any old binaries
echo "Reboot complete detected, now running valclean"
valclean target

# Copy the HWC log files into the upstream workspace.
echo "Exporting logs to: $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER"
echo ""
export CLIENT_LOGS=$UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER

echo "Looking for artifacts in $OLD_WORKSPACE/$BUILD_NUMBER/artifacts.gz"
if [[ -e $OLD_WORKSPACE/$BUILD_NUMBER/artifacts.gz ]]
then
  echo "Importing artifacts from $OLD_WORKSPACE/$BUILD_NUMBER/artifacts.gz"
  source valimport $OLD_WORKSPACE/$BUILD_NUMBER/artifacts.gz
else
  echo "Could not find artifacts! Using valpush."
  valpush hwc hwcval
fi

# Store the final output (that's uploaded to Gerrit) in the upstream workspace.
TEST_RESULTS=$UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/test_results.log
TMPFILE=$UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/tmp_test_results.log

# Run test set.
echo "Calling valsmoke on $JENKINS_TEST_SET"
valsmoke $JENKINS_TEST_SET |tee $TEST_RESULTS
valclean target
$ADB shell start

# Parse the results and determine the 'verified' flag.
REGEX="^.*TESTS PASSED:([0-9]+) FAILED:([0-9]+) ABORTED:([0-9]+)"
while read LINE; do
  LINE=`echo $LINE | tr '[:lower:]' '[:upper:]'`
  if [[ $LINE =~ $REGEX ]]
  then
    PASSED="${BASH_REMATCH[1]}"
    FAILED="${BASH_REMATCH[2]}"
    ABORTED="${BASH_REMATCH[3]}"
    break
  fi
done < $TEST_RESULTS

if [[ $PASSED -gt 0 ]] && [[ $FAILED -eq 0 ]] && [[ $ABORTED -eq 0 ]]
then
  VERIFIED="+1"
else
  VERIFIED="-1"
fi

# Write a summary of the commit stack into the output that will be uploaded to Gerrit.
if [[ -e $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/commit.log ]]
then
  cat $UPSTREAM_WORKSPACE/$UPSTREAM_BUILD_NUMBER/commit.log >> $TEST_RESULTS
fi

# Add some links to make it easier to navigate to the logs
echo "Link to logs:" | tee -a $TEST_RESULTS
echo "  ${UPSTREAM_JOB_URL%?}/ws/$UPSTREAM_BUILD_NUMBER" | tee -a $TEST_RESULTS
echo "" | tee -a $TEST_RESULTS

# If this is a Gerrit commit, then extract the change number and patchset from the RefSpec
if [[ $GERRIT_REFSPEC =~ "refs/changes/" ]]
then
  GERRIT_CHANGE_NUMBER=`echo $GERRIT_REFSPEC | awk -F '/' '{ print $(NF-1) }'`
  GERRIT_PATCHSET_NUMBER=`echo $GERRIT_REFSPEC | awk -F '/' '{ print $NF }'`

  # Remove double quotes to avoid problems uploading to gerrit.
  sed -i s/\"//g $TEST_RESULTS

  # Compose an initial line of text for the gerrit posting
  if [[ $VERIFIED == "+1" ]]
  then
    TEST_RESULTS_SUMMARY="$1 APPROVED: $PASSED tests executed"
  else
    TEST_RESULTS_SUMMARY="$1 **REJECTED**: $PASSED passed, $FAILED failed, $ABORTED aborted"
  fi

  REG_EX_HWC_BRANCH="refs/heads/.+"
  if [ $HWC_REFSPEC != "" ] && ! [[ $HWC_REFSPEC =~ $REG_EX_HWC_BRANCH ]]
  then
    TEST_RESULTS_SUMMARY="$TEST_RESULTS_SUMMARY\nMANUAL VALIDATION BUILD of $GERRIT_REFSPEC against HWC $HWC_REFSPEC"
  fi

  if [[ $HWCVAL_REFSPEC != "" && $HWCVAL_REFSPEC != "refs/heads/jenkins" ]]
  then
    TEST_RESULTS_SUMMARY="$TEST_RESULTS_SUMMARY\nMANUAL BUILD of $GERRIT_REFSPEC against HWCVAL $HWCVAL_REFSPEC"
  fi

  # Prefix the test results with the summary
  echo -e $TEST_RESULTS_SUMMARY | cat - $TEST_RESULTS > $TMPFILE
  mv $TMPFILE $TEST_RESULTS

  # Update Gerrit with the test results - in all cases.
  echo "Posting results to Gerrit change number: $GERRIT_CHANGE_NUMBER, patchset: $GERRIT_PATCHSET_NUMBER"
  ssh -p 29418 sys_hwcjenkins@vpg-git.iind.intel.com gerrit review \"--message="`cat $TEST_RESULTS`"\" \
    $GERRIT_CHANGE_NUMBER,$GERRIT_PATCHSET_NUMBER

  # Check if the Gerrit change has already been marked as failed by another runner. If so, don't
  # mark it as verified if this run has passed.
  ALREADY_FAILED=`ssh -p 29418 sys_hwcjenkins@vpg-git.iind.intel.com gerrit query \
    "change:$GERRIT_CHANGE_NUMBER AND label:Verified<=-1" | grep rowCount | sed "s/rowCount: //"`
  if [[ $ALREADY_FAILED == 1 ]]
  then
    if [[ $VERIFIED == "+1" ]]
    then
      exit 0
    fi
  else
    # Update the verified flag in Gerrit
    ssh -p 29418 sys_hwcjenkins@vpg-git.iind.intel.com gerrit review --verified=$VERIFIED \
      $GERRIT_CHANGE_NUMBER,$GERRIT_PATCHSET_NUMBER
  fi
fi

# Signal the error to Jenkins if any of the tests have failed.
if [[ $VERIFIED == "-1" ]]
then
    exit 1
fi

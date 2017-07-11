# Jenkins build script common includes

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 8th May 2015
# Description: Jenkins script to define common settings

# Turn off debug printing
set +x

# Choose ADB instance to use
export ADB=adb

# Setup build constants - note - this remapping
# has to be done before sourcing platform.sh.
if [[ $UPSTREAM_WORKSPACE != "" ]]
then
    export OLD_WORKSPACE=$WORKSPACE
    export WORKSPACE=$UPSTREAM_WORKSPACE
fi

# Source the platform specific includes
source ~/bin/platform.sh

# Prints useful information for logs (not Gerrit)
# This information is used by jenkins jobs to set discriptions.
# Do this at the start, not the end!
if [[ $GERRIT_CHANGE_SUBJECT != "\$GERRIT_CHANGE_SUBJECT" ]] && \
   [[ $GERRIT_CHANGE_OWNER_EMAIL != "\$GERRIT_CHANGE_OWNER_EMAIL" ]] && \
   [[ $GERRIT_CHANGE_URL != "\$GERRIT_CHANGE_URL" ]] && \
   [[ -n "$GERRIT_CHANGE_SUBJECT" ]] && \
   [[ -n "$GERRIT_CHANGE_OWNER_EMAIL" ]] && \
   [[ -n "$GERRIT_CHANGE_URL" ]]
then
  echo "Change subject: $GERRIT_CHANGE_SUBJECT"
  echo "Change owner: $GERRIT_CHANGE_OWNER_EMAIL"
  echo "Change URL: $GERRIT_CHANGE_URL"
  echo ""
else
  if [[ $JENKINS_JOB_DESCRIPTION =~ "Manual HWC"[VAL]{3}?" Build" ]]
  then
      echo "Change subject: $GERRIT_REFSPEC"
  else
      echo "Change subject: $JENKINS_JOB_DESCRIPTION"
  fi
  echo "Change URL: $BUILD_URL"
  echo ""
fi

# Prints the commit stack
print_summary_git_log()
{
  echo "Last $1 $2 commits:"
  echo ""
  git log -n $1 --oneline | sed -e 's/^/ /'
  echo ""
}

# Look for a 'test set' override in the commit message (only for Gerrit builds)
look_for_test_set_override()
{
  PREFIX="Test-set: "
  COMMIT_TEST_SET=`git log --format=%B -1 | grep -i "^$PREFIX" | sed "s/$PREFIX//i"`

  if [[ -n $COMMIT_TEST_SET ]]
  then
    echo $COMMIT_TEST_SET > $WORKSPACE/$BUILD_ID/test_set.override
    echo "Overridden default Test Set with value: $COMMIT_TEST_SET"
  fi
}

# Look for a 'Tracked-On' field in the commit message (only for Gerrit builds)
look_for_tracked_on()
{
  PREFIX="Tracked-On: "
  COMMIT_TRACKED_ON=`git log --format=%B -1 | grep -i "^$PREFIX" | sed "s/$PREFIX//i"`

  if [[ -z $COMMIT_TRACKED_ON ]]
  then
    echo "WARNING: Missing or empty 'Tracked-On' field! All work should be linked to a Jira."
  fi
}

# Dumps the HWC git log to a file (takes directory as a parameter)
dump_hwc_git_log()
{
  cd $1
  git log > $WORKSPACE/$BUILD_ID/hwc.gitlog
  cd -
}

reboot_and_wait()
{
  # We only want to reboot to ADB so we can revert the binaries
  # We don't want to wait for Android to come up : we do that later.
  eval $REBOOT_CMD
  $ADB wait-for-device
  $ADB root
  $ADB wait-for-device
  $ADB disable-verity
  $ADB remount
}
export -f reboot_and_wait

# this function is called from inside valhwch, so its output may be filtered by vallogadd
# All echos must start w/ *** to avoid filtering out on jenkins.
wait_for_boot_complete()
{
    # Wait for boot to complete
    echo "*** Entering wait for boot complete"
    $ADB wait-for-device
    bc="`$ADB shell getprop sys.boot_completed`"
    if [[ "$bc" != 1* ]]
    then
        echo "*** Wait for boot to complete..."
    fi

    loop_counter=0
    boot_counter=0
    while [[ "$bc" != 1* ]]
    do
        sleep 10

        $ADB wait-for-device || true
        bc="`$ADB shell getprop sys.boot_completed`" || true
        let "loop_counter+=1"

        if [[ $loop_counter -gt 30 ]]
        then
            echo "*** Boot did not complete within 5 minutes, rebooting..."
            eval $REBOOT_CMD
            let "boot_counter+=1"
            if [[ $boot_counter -ge 3 ]]
            then
                exit 1
            else
                $ADB wait-for-device || true
                loop_counter=0
            fi
        fi
    done
}
export -f wait_for_boot_complete


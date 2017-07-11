#!/usr/bin/env bash

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 20th May 2015
# Version: 2.0
# Description: Jenkins script to build the Hardware Composer

# Author: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 4th Jan 2016
# Version: 3.0

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"
source ~/bin/common.sh

# If this is a new patchset, then add Gary as a reviewer.
if [[ $GERRIT_EVENT_TYPE == "patchset-created" ]]
then
  export JENKINS_SCRIPT_ROOT="/usr2/jenkins/bin/core-scripts/"
  export REVIEWER_LIST="\
     gary.k.smith@intel.com \
     chandrasekaran.sakthivel@intel.com \
     daniel.t.keith@intel.com
  "

  echo "Adding reviewer for new patchset"
  CMD="$JENKINS_SCRIPT_ROOT/admin/val_core_add_reviewer_generic.sh"
  flock ~/tmp/jenkins.lock -c "$CMD"
fi

# Update and build the HWC Validation infrastructure
cd $WORKSPACE/build/hwcval/tests/hwc
# The user has specified a particular refspec for the HWCVAL
echo "Using HWCVAL $HWCVAL_REFSPEC"
git fetch ssh://sys_hwcjenkins@vpg-git.iind.intel.com:29418/hwcval $HWCVAL_REFSPEC
git checkout FETCH_HEAD

for REF in $JENKINS_ADDITIONAL_HWCVALREFS; do
    echo "Cherry picking HWCVAL $REF"
    git fetch ssh://sys_hwcjenkins@vpg-git.iind.intel.com:29418/hwcval $REF
    git cherry-pick FETCH_HEAD
done


# Dump the HWC Git log for DetermineCheckiDisables.awk
dump_hwc_git_log $TOP/hardware/intel/hwc

# Look for a 'test set' override in the commit message (only for Gerrit builds)
cd $TOP/hardware/intel/hwc
look_for_test_set_override

# Look for a 'Tracked-On' field in the commit message (only for Gerrit builds)
look_for_tracked_on

# Setup HWC build environment
cd $TOP
source build/envsetup.sh
lunch $PRODUCT-eng

# Setup HWCVAL build environment
cd $WORKSPACE/build/hwcval/tests/hwc
source init.sh

# Remove old binaries from build tree
valclean host

# Build HWC
cd $TOP/hardware/intel/hwc/build
mm -B -j8

# Build HWC validation
cd $WORKSPACE/build/hwcval/tests/hwc
mm -B -j8

# Archive the artifacts
cd $WORKSPACE
valexport artifacts &> /dev/null

# Display some summary information at the end of the build log
cd $TOP/hardware/intel/hwc
print_summary_git_log $JENKINS_NUM_COMMITS_TO_LOG "HWC" | tee $WORKSPACE/$BUILD_ID/commit.log

if [[ $JENKINS_HWCVAL_REFSPEC =~ "refs/changes/" ]]
then
  cd $WORKSPACE/build/hwcval/
  print_summary_git_log $JENKINS_NUM_COMMITS_TO_LOG "HWCVAL"
fi

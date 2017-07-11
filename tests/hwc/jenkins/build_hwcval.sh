#!/usr/bin/env/bash

# Author: James Pascoe
# Date: 20th May 2015
# Version: 2.0
# Description: Jenkins script to build the Hardware Composer Validation

# Owner: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 9th Sept 2016

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"
source ~/bin/common.sh

# If this is a new patchset, then add us as reviewers
if [[ $GERRIT_EVENT_TYPE == "patchset-created" ]]
then
  export JENKINS_SCRIPT_ROOT="/usr2/jenkins/bin/core-scripts/"
  export REVIEWER_LIST=" \
    daniel.t.keith@intel.com \
    chandrasekaran.sakthivel@intel.com \
    gary.k.smith@intel.com
    "

  echo "Adding reviewers for new patchset"
  CMD="$JENKINS_SCRIPT_ROOT/admin/val_core_add_reviewer_generic.sh"
  flock ~/tmp/jenkins.lock -c "$CMD"
fi

# Update the HWC
cd $TOP/hardware/intel/hwc
# The user has specified a particular refspec for the HWC
echo "Using HWC $HWC_REFSPEC"
git fetch ssh://sys_hwcjenkins@vpg-git.iind.intel.com:29418/vpg-ics/hardware/intel/hwcomposer $HWC_REFSPEC
git checkout FETCH_HEAD

for REF in $JENKINS_ADDITIONAL_HWCREFS; do
    echo "Cherry picking HWC $REF"
    git fetch ssh://sys_hwcjenkins@vpg-git.iind.intel.com:29418/vpg-ics/hardware/intel/hwcomposer $REF
    git cherry-pick FETCH_HEAD
done

# Dump the HWC Git log for DetermineCheckiDisables.awk
dump_hwc_git_log $TOP/hardware/intel/hwc

# Look for a 'test set' override in the commit message (only for Gerrit builds)
cd $WORKSPACE/build/hwcval
look_for_test_set_override

# Setup build environment
cd $TOP
source build/envsetup.sh
lunch $PRODUCT-eng

# Setup validation environment
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
cd $WORKSPACE/build/hwcval
print_summary_git_log $JENKINS_NUM_COMMITS_TO_LOG "HWCVAL" | tee $WORKSPACE/$BUILD_ID/commit.log

if [[ $HWC_REFSPEC =~ "refs/changes/" ]]
then
  cd $TOP/hardware/intel/ufo/ufo/Source/Android/hwc
  print_summary_git_log $JENKINS_NUM_COMMITS_TO_LOG "HWC"
fi

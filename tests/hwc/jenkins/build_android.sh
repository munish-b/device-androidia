#!/usr/bin/env bash
# Author: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 4th Jan 2016
# Version: 1.0
# Description: Jenkins script to build android trees

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"

source ~/bin/common.sh

export PATH=$HWCVAL_JENKINS_BIN:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:$PATH

# Attempt to build the image
for JOB in $JENKINS_JOBS; do

  JOB_BUILD_DIR=$HOME/jobs/$JOB/workspace/build

  export TOP=$HOME/jobs/$JOB/workspace/build/android
  export P4_ROOT=$JOB_BUILD_DIR/ufo/gfx_Development

  # Setup an Android build environment
  cd $TOP
  source build/envsetup.sh
  lunch $PRODUCT-$BUILD_VARIANT

  echo "$JOB: Building" `pwd`

  # Build Android. Use 'make' return value to trigger success or fail
  time make -C $P4_ROOT/Tools/Linux/ BUILD_TYPE=release TARGET_BUILD_VARIANT=eng ANDROID_SRC=$TOP \
   TARGET_PRODUCT=$PRODUCT UFO_OCL=n UFO_RS=n JOBS=14 PATCH=nopatch FLASHFILE_NO_OTA=true -k artifacts \
   |& tee $JOB_BUILD_DIR/makeoutput.log
done

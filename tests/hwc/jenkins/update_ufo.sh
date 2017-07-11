#!/bin/bash

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 31st May 2015
# Version: 1.0

# Author: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 23rd Dec 2016
# Version: 2.0
# Description: Jenkins script to update UFO and build it against the most
# recently installed Android tree.

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"
source ~/bin/common.sh 

# The plugin will have synced UFO top-of-tree. Move it to a central location.
NEW_UFO=ufo_${2}_`date +%d%m%y`
NEW_UFO_CENTRAL_PATH=$HOME/ufo-trees/$NEW_UFO

# Create the target directory
if [[ -d $NEW_UFO_CENTRAL_PATH ]]
then
  rm -rf $NEW_UFO_CENTRAL_PATH
fi
mkdir -p $NEW_UFO_CENTRAL_PATH
mv $WORKSPACE/gfx_* $NEW_UFO_CENTRAL_PATH

# Install the new UFO drop
for JOB in $JENKINS_JOBS; do
  JOB_BUILD_DIR=$HOME/jobs/$JOB/workspace/build
  cd $JOB_BUILD_DIR

  echo "created on" `date +%d%m%y` > ufo_tree_date 

  if [[ -d ufo ]]
  then
    rm -rf ufo
  fi
  mkdir -p ufo

  # Copy the new UFO drop into the build directory
  cp -ar $NEW_UFO_CENTRAL_PATH/gfx_* ufo

done



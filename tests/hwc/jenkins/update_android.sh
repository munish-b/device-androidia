#!/bin/bash

# Author: Daniel Keith
# Email: daniel.t.keith@intel.com
# Date: 27th December 2016
# Version: 2.0
# Description: Jenkins script to update the Android build

echo "Running $BASH_SOURCE $JENKINS_JOB_DESCRIPTION"
source ~/bin/common.sh 

export JENKINS_REPO_SYNC_CMD="repo sync -cqj5"

# move to our mirror directory
cd ~/repo-mirrors

#error checks, both vars come from Jenkins job
if [[ -z "$JENKINS_REPO_INIT_MIRROR_CMD" ]]
then
    echo "Error variable JENKINS_REPO_INIT_MIRROR_CMD is empty!"
    exit 1
fi
if [[ -z "$JENKINS_REPO_INIT_LOCAL_CMD" ]]
then
    echo "Error variable JENKINS_REPO_INIT_LOCAL_CMD is empty!"
    exit 1
fi


#repo init mirror
eval $JENKINS_REPO_INIT_MIRROR_CMD
eval $JENKINS_REPO_SYNC_CMD

echo "Result: Succesfully synced ~/repo-mirrors"

#FILES_TO_DISABLE="
#    intel/binarydownloader/binarydownloader
#    intel-imc/sflte_2/vendorsetup.sh
#    intel-imc/sofia3g/vendorsetup.sh
#    intel-imc/sofia_lte/vendorsetup.sh
#"

# Checkout the tree in each of the destination workspaces
for JOB in $JENKINS_JOBS; do

    cd $HOME/jobs/$JOB/workspace/build
    echo "Repo initialized with:  $JENKINS_REPO_INIT_LOCAL_CMD  ;  Created on" `date +%d%m%y` > android_tree_date 

    if ! [[ -d android ]]
    then
      mkdir -p android
    fi
    
    cd android
    if [[ -d out ]]
    then
      rm -rf out
    fi

    eval "$JENKINS_REPO_INIT_LOCAL_CMD --reference=/usr2/jenkins/repo-mirrors"
    eval $JENKINS_REPO_SYNC_CMD

    echo "Result: Succesfully synced" `pwd`

    #for FILE in $FILES_TO_DISABLE; do
    #  if [[ -e device/$FILE ]]
    #  then
    #    mv device/$FILE device/$FILE-disable
    #  fi
    #done

  # Clone the HWC to the android tree
  cd hardware/intel/hwc
  if  [ ! -d .git/refs/remotes/vpg-git ] ; then
    git remote add vpg-git -f ssh://vpg-git.iind.intel.com:29418/vpg-ics/hardware/intel/hwcomposer
  fi
  git fetch vpg-git
  git reset vpg-git/hwc-next --hard
  git config branch.master.merge refs/heads/hwc-next
  git config branch.master.rebase true
  git config remote.origin.push HEAD:refs/for/hwc-next

done



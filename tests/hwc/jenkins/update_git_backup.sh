#!/bin/bash

# Author: James Pascoe
# Email: james.pascoe@intel.com
# Date: 2nd February 2015
# Version: 1.0
# Description: Jenkins script to backup config updates in Git

cd /usr2/jenkins

git commit -a -m"Jenkins Config Commit" > /tmp/jenkins.log
if [[ $? == 1 ]]
then
  exit 0; # No changes - don't generate any output
fi

cat /tmp/jenkins.log
git diff HEAD~1

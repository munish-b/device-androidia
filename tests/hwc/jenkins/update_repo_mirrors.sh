#!/usr/bin/env bash

cd /usr2/jenkins/repo-mirrors

# MCGR5.1 (+15.33 UFO) for BYT
/usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo init -u git://vpg-git.iind.intel.com/manifests.git -b 15.33 -m mcg51-stable.xml
time /usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo sync -cq -j5

# 1Andr-M-R2 (+Mainline UFO) for CHT and BXT
/usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo init -u ssh://android.intel.com/manifests -b feature/gfx_gen/master/r2
time /usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo sync -cq -j5

# R1 Tree for BXT
/usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo init -u ssh://android.intel.com/manifests -b feature/m_mr0/gfx_gen/r1
time /usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo sync -cq -j5

# 1Andr-L-R2 (+L_MR1 UFO) for CHT
/usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo init -u ssh://android.intel.com/manifests -b abt/topic/gmin/l-dev/mr1/graphics-gen/r2 -m topic/graphics-gen/master
time /usr2/jenkins/bin-sparse/tests/hwc/jenkins/repo sync -cq -j5

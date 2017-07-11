#!/bin/bash
#
# INTEL CONFIDENTIAL
#
# Copyright © 2016 Intel Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents related to
# the source code ("Material"} are owned by Intel Corporation or its suppliers
# or licensors. Title to the Material remains with Intel Corporation or its
# suppliers and licensors. The Material contains trade secrets and proprietary
# and confidential information of Intel or its suppliers and licensors.
# The Material is protected by worldwide copyright and trade secret laws and
# treaty provisions. No part of the Material may be used, copied, reproduced,
# modified, published, uploaded, posted, transmitted, distributed, or
# disclosed in any way without Intel's prior express written permission.
#
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be
# express and approved by Intel in writing
#
# Authors:
#       Robert Pinsker <robert.pinsker@intel.com>
#       Dan Keith <daniel.t.keith@intel.com>
#

# Set up environment
cd $ANDROID_BUILD_TOP
source build/envsetup.sh
export TOP=$ANDROID_BUILD_TOP
lunch ${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}

# Determine artifact destination
if [ -f "${ANDROID_PRODUCT_OUT}/TARGET_ARCH" ]; then
    TARGET_ARCHITECTURE=`cat "${ANDROID_PRODUCT_OUT}/TARGET_ARCH"`
    if [ "${TARGET_ARCHITECTURE}" == "x86" ]; then
        ARTIFACT_DESTINATION+="/android32/${TARGET_PRODUCT}"
    elif [ "${TARGET_ARCHITECTURE}" == "x86_64" ]; then
        ARTIFACT_DESTINATION+="/android64/${TARGET_PRODUCT}"
    fi
fi

# Perform the build
cd vendor/intel/validation/hwc
mm

#!/bin/bash
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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

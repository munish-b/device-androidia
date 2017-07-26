#!/bin/bash
#
# Usage: source init.sh
#
# To be used  before building the HWC Validation tests.
#
LOCAL_PATH="`dirname \"$BASH_SOURCE\"`"
export HWCVAL_ROOT="`( cd \"$LOCAL_PATH\" && pwd)`"
export VAL_HWC_TOP="`( cd \"$LOCAL_PATH/../.." && pwd)`"
echo "HWC Validation path is $HWCVAL_ROOT"
export PATH=$HWCVAL_ROOT/host_scripts:$HWCVAL_ROOT/tools:$PATH
export CLIENT_LOGS=$HOME/client_logs

# Target directories
export HWCVAL_TARGET_DIR=/data/validation/hwc
export HWCVAL_TARGET_SCRIPT_DIR=/data/validation/hwc

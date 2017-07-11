#!/system/bin/sh
#
##############################################################################
# Stop the shims checks.
# Stop Android, uninstall the shims and start Android again.
# If called from the target itself, need to be called with ./
#
# Arguments:
#   None
# Returns:
#   0
##############################################################################

# Libraries and global variables
source ${0%/*}/valhwc_common

# Set PAVE environment variable
export IS_AUTOVAL=1

# Stop checks
./valhwc_util stop

sleep 1

# Some clean up
./valhwc_killapp.sh hwclogviewer
./valhwc_killapp.sh logcat

# Stop Android
stop

# Uninstall the shims
echo "Uninstalling shims"
./valhwc_unins.sh

# Re-start Android
start






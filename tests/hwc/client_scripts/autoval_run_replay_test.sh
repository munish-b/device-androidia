#!/system/bin/sh
#
##############################################################################
# Script used in PAVE and GTA automation validation context
#
# Sets the PAVE env variable, installs the shims, launches the replay test and
# uninstalls the shims
#
# Arguments:
#   $1: test to be executed
#   $2: flag to determine whether or not I am working in GTAX
# Returns:
#   0
##############################################################################


# Add busybox path to PATH for advanced unix commands
PATH=$PATH:/system/xbin/busybox:/system/xbin/busybox/busybox:/data/xbin/busybox


# Set PAVE environment variable
export IS_AUTOVAL=1


echo "Running HWC Replay Test for $1\n"


# Print some useful environmental information
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
echo "Path is: $PATH\n"
echo "LD_LIBRARY_PATH is: $LD_LIBRARY_PATH\n"
echo "PWD: $PWD contains:"
ls
echo ""


# Install the shims
./valhwc_install_shims.sh no_sf
echo ""


# Set up log files
rm hwclog_hwch.log
hwclogviewer -v -f > hwclog_hwch.log &


# Run the test
gunzip $1.log.gz
./valhwcharness -replay_hwcl $1.log | tee replay_output.log


# Uninstall the shims
./valhwc_unins.sh
start
echo ""


# All done
echo "Test Finished"
exit 0

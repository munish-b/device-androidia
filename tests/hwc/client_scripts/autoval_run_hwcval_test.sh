#!/system/bin/sh
#
##############################################################################
# Script used in PAVE and GTA automation validation context
#
# It sets the PAVE env variable, installs the shims, launches a test and
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
source ${0%/*}/valhwc_common


# GTA - move into designated asset destination directory
if [ "$2" != "" ]
then
    cd $2
fi


# Set PAVE environment variable
export IS_AUTOVAL=1


# Start HWCVAL test suite
echo "HWCVAL Tests Start"


# Install the shim layers
echo "Install SHIMS"
./valhwc_install_shims.sh no_sf


# To enable XXX & YYY checks/logs in PAVE, uncomment these two lines
#setprop hwcval.log.enable "XXX YYY"
#setprop hwcval.default_log_priority V


# Identify test parameters
test_name=$1                # name of the test to run
test_logname=$test_name     # suffix to be added to *.csv
test_list=autoval_testlist.txt     # name of the file with all the tests
test_commandline=$(grep -w "^${test_name}," $test_list | cut -f 2 -d ',')    # full test command line

if [ "$test_commandline" == "" ]
then
    echo "No test match for $test_name"
else
    # Run the test
    echo "Running Test $test_commandline -logname=$test_logname"
    ./valhwc_run_harness.sh "$test_commandline" -logname="$test_logname"
fi


 # Uninstall the shim layers
echo "\nUninstall SHIMS"
./valhwc_unins.sh
start


# Finish HWCVAL test suite
echo "HWCVAL Tests End"


exit 0


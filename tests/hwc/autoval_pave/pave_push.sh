#!/bin/bash
#
##############################################################################
# This script generates two directories which contain the binaries
# to be exported in PAVE to run the HWCVAL tests and the HWCVAL replay tests.
#
# This script needs to be launched with ./<name>
#
# Arguments:
#   None
# Returns:
#   None
##############################################################################

if [ "$HWCVAL_ROOT" = "" ]
then
  echo "HWCVAL_ROOT not defined"
  exit
fi


if [ "$TARGET_PRODUCT" = "" ]
then
  echo "TARGET_PRODUCT not defined"
  exit
fi


# Define different LIB and BIN PATH variables for BXT and CHT to make up for the change in
# the Android build system for the two platforms
if [ -e $ANDROID_PRODUCT_OUT/system/lib64 ]
then
    LIBDIR_BXT="/vendor/lib64"
    LIBDIR_CHT="/system/vendor/lib64"
else
    LIBDIR_BXT="/vendor/lib"
    LIBDIR_CHT="/system/vendor/lib"
fi

BINDIR_BXT="/vendor/bin"
BINDIR_CHT="/system/vendor/bin"


# Check which is the appropriate set of PATHs to be used for this platform
if [[ $TARGET_PRODUCT == *bxt* ]]
then
    LIBDIR=$LIBDIR_BXT
    BINDIR=$BINDIR_BXT
elif [[ $TARGET_PRODUCT == *cht* ]]
then
    LIBDIR=$LIBDIR_CHT
    BINDIR=$BINDIR_CHT
else
    echo "Target not supported"
fi


# Create and/or reset the input directories used by PAVE.
# One directory is getting generated for each platform to be used in PAVE
# One directory is getting generated to be used with the Unified Validation Software Component
PAVE_dir=$HWCVAL_ROOT/../../../PAVE_hwcval/"$TARGET_PRODUCT"_input
PAVE_dir_SC=$HWCVAL_ROOT/../../../PAVE_hwcval/unified_SC_input

rm -fr $PAVE_dir/*
rm -fr $PAVE_dir_SC/*

mkdir -p $PAVE_dir
mkdir -p $PAVE_dir_SC


# Copy scripts and executables to PAVE input directory (traditional)
cp -ra $HWCVAL_ROOT/autoval_pave/parser $PAVE_dir/..
cp $HWCVAL_ROOT/autoval_pave/pave_runner_command $PAVE_dir/..
cp $HWCVAL_ROOT/autoval_pave/pave_runner_launch.sh $PAVE_dir/..
cp $HWCVAL_ROOT/client_scripts/autoval_run_hwcval_test.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/autoval_testlist.txt $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/autoval_start_checks.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/autoval_stop_checks.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_install_shims.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_run_harness.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_unins.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_killapp.sh $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_common $PAVE_dir
cp $HWCVAL_ROOT/client_scripts/valhwc_wakeup_screen.sh $PAVE_dir
cp $HWCVAL_ROOT/host_scripts/TestCheckMatrix.py $PAVE_dir
cp $HWCVAL_ROOT/images/globe-scene-fish-bowl.png $PAVE_dir
cp $HWCVAL_ROOT/images/Spiderman.png $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$BINDIR/valhwcharness $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$BINDIR/hwclogviewer $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$BINDIR/valhwc_util $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$BINDIR/valhwc_surfaceflingershim $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$LIBDIR/libvalhwccommon.so $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$LIBDIR/libvalhwc_drmshim.so $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$LIBDIR/libvalhwc_ivpshim.so $PAVE_dir
cp $ANDROID_PRODUCT_OUT/$LIBDIR/valhwc_composershim.so $PAVE_dir
cp -f $ANDROID_PRODUCT_OUT/$LIBDIR/libvalhwc_widishim.so $PAVE_dir >& /dev/null
cp -f $ANDROID_PRODUCT_OUT/$LIBDIR/libvalhwc_mdsshim.so $PAVE_dir >& /dev/null


# Copy scripts and executables to PAVE input directory (Software Component)
cp $HWCVAL_ROOT/client_scripts/autoval_extract_hwcval.sh $PAVE_dir_SC
cp $HWCVAL_ROOT/host_scripts/TestCheckMatrix.py $PAVE_dir_SC


# Set rights
chmod 777 $PAVE_dir/*
chmod 777 $PAVE_dir/../*
chmod 777 $PAVE_dir_SC/*
chmod 777 $PAVE_dir_SC/../*




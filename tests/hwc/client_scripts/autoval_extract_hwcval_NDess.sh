#!/system/bin/sh
#
##############################################################################
# Script used in PAVE and GTA automation validation context
#
# Retrieves the location of the Software Component containing the hwcval software.
# This is in the form of a tar file. The environment variable PATH contains
# the path of the SC, whose name is similar to the SC itself. After having found
# the file, it decompresses it and it copies all the files in the source directory.
# Then launches the traditional installation script.
#
# Arguments:
#   $1: name of the test to be executed (i.e. Camera)
# Returns:
#   None
##############################################################################

set -x


# Add busybox path to PATH for advanced unix commands
PATH=$PATH:/system/xbin/busybox:/system/xbin/busybox/busybox:/data/xbin/busybox


# Suppress new type of error
export HWCVAL_LOG_SETWARNING=”eCheckSfFallback”


# Global variables
UNIFIED_VAL_PACKAGE_STRING_ID="android_val"
HWCVAL_PACKAGE_STRING_ID="val_hwc_"
OS_DESSERT_VERSION=".N."
TMP_DIR="tmp"
TEST=$1


# Retrieve name of the running host
BUILD_FLAVOR=$(getprop ro.build.flavor)
HOSTNAME=${BUILD_FLAVOR%-*}


###############################################################################
# Find the unified validation compressed file in the tree and copy that locally
###############################################################################

# Find the package and terminate command after first match
UNIFIED_VAL_PACKAGE=$(find ../ -name *$UNIFIED_VAL_PACKAGE_STRING_ID* -print | head -n 1)

# Copy package found locally
cp $UNIFIED_VAL_PACKAGE .

# Some debug info
FILES_HERE=$(ls -al)


###############################################################################
# Extract from the unified validation package the hwc validation packages
###############################################################################

# Get the unified validation package name with no path
UNIFIED_VAL_PACKAGE_NAME=$(basename "$UNIFIED_VAL_PACKAGE")

# Get the unified validation package name with no extension
UNIFIED_VAL_PACKAGE_NAME_ONLY="${UNIFIED_VAL_PACKAGE_NAME%.*}"

# Rename the file as gzip expects .tar.gz extension
mv $UNIFIED_VAL_PACKAGE_NAME_ONLY.tgz $UNIFIED_VAL_PACKAGE_NAME_ONLY.tar.gz

# Unzip the package first
gzip -d $UNIFIED_VAL_PACKAGE_NAME_ONLY.tar.gz

# Uncompress unified validation package
tar -xf $UNIFIED_VAL_PACKAGE_NAME_ONLY.tar

# Some debug info
FILES_HERE=$(ls -al)
ENV_VARIABLES=$(printenv)


###############################################################################
# Install the shims from the hwc validation package
###############################################################################

# Find the validation package for the specific HOST and terminate command after first match
HWCVAL_PACKAGE_NAME=$(find -name $HWCVAL_PACKAGE_STRING_ID*$OS_DESSERT_VERSION*$HOSTNAME* -print | head -n 1)

# Get the hwc validation package name with no extension
HWCVAL_PACKAGE_NAME_ONLY="${HWCVAL_PACKAGE_NAME%.*}"

# Rename the file as gzip expects .tar.gz extension
cp $HWCVAL_PACKAGE_NAME_ONLY.tgz $HWCVAL_PACKAGE_NAME_ONLY.tar.gz

# Unzip the package first
gzip -d $HWCVAL_PACKAGE_NAME_ONLY.tar.gz

# Create temporary directory where to unpack package
mkdir -p $TMP_DIR

# Untar the package in a temporary directory
tar -xf $HWCVAL_PACKAGE_NAME_ONLY.tar -C $TMP_DIR

# If target 64 bit, remove the 32 bit library and viceversa
if [ -e /system/lib64 ]
then
    rm -fr $TMP_DIR/val_hwc/lib
else
    rm -fr $TMP_DIR/val_hwc/lib64
fi

# Move all the files back here (overwrites if necessary) and remove the directory structure
find $TMP_DIR -type f -exec mv -if {} . \;

# Some debug info
FILES_HERE=$(ls -al)


#########################################################
# Run the normal execution script
#########################################################
./autoval_run_hwcval_test.sh $TEST














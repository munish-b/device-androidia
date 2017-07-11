#!/system/bin/sh
#
##############################################################################
# Install the shims if not already present and start Android.
# Start the shims checks.
# Start collecting hwclogviewer and logcat log files.
# If called from the target itself, need to be called with ./
#
# Arguments:
#   -h/--h/help/-help/--help::= helper function
#   <arg>                   ::= <loglevel> | <systrace> | <crcvalidation>\n"
#   <loglevel>              ::= -log_pri={E|W|I|D|V}\n"
#   <systrace>              ::= -systrace[=<seconds>]\n"
#   <crcvalidation>         ::= {<hwcval> | <drmval>}"
#   <drmval>                ::= -crc\n"
# Returns:
#   0
##############################################################################

# Libraries and global variables
source ${0%/*}/valhwc_common

# Set PAVE environment variable
export IS_AUTOVAL=1

# Ensure to have read/write access
mount -o rw,remount /system

# Create validation directory tree if not present and clean up if present
mkdir -p $HWCVAL_DIR
rm -rf $HWCVAL_DIR/dump
rm -f $HWCVAL_DIR/hwclog_start_android.log
rm -f $HWCVAL_DIR/logcat_start_android.log
logcat -c

sleep 1

# Test to see if the shims are running
drm_shim_installed=`grep -c drmshim $LIBDIR/libdrm.so`
hwc_shim_installed=`grep -c valhwc_composershim $LIBDIR/hw/hwcomposer.$TARGETSHIM.so`
sf_shim_installed=`grep -c surfaceflingershim /system/bin/surfaceflinger`
if [[ $drm_shim_installed == 0 || $hwc_shim_installed == 0 || $sf_shim_installed == 0 ]]
then
    shims_installed=0
else
    shims_installed=1
fi

setprop intel.hwc.debuglogbufk 512

# surfaceflinger will be run as the system user, make sure it can access the
# CRC drivers in debugfs
echo "Changing ownership of the display CRC drivers"
chown system:system /sys/kernel/debug/dri/0/i915_display_crc_ctl
chown system:system /sys/kernel/debug/dri/0/i915_pipe_*

# Install shims if not installed already
if [ $shims_installed -eq 0 ]
then
  echo "Installing shims"
  stop
  ./valhwc_install_shims.sh
  sleep 1
else
  echo "Shims already running"
  ./valhwc_killapp.sh hwclogviewer
  ./valhwc_killapp.sh logcat
fi

# Restart Android
android_process_count=`ps | grep -c android`
if [ $android_process_count -lt 2 ]
then
  echo "Restarting Android..."

  # Disable SELinux
  setenforce 0

  start
fi

# Start populating log files again
hwclogviewer -v -f > $HWCVAL_DIR/hwclog_start_android.log&
logcat -f $HWCVAL_DIR/logcat_start_android.log&

sleep 1

# Start checks
/system/vendor/bin/valhwc_util start $@

echo "Now please use Android"

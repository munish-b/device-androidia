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
"""
hwc_stop_valshims_checks

Description:
    Stops the checks done by the validation shims.
    Uninstalls the HWC validation shims.

Example:
    For Android:
        [test]
        hwc_stop_valshims_checks

Input variables:
    None
"""

import os
import logging


__authors__ = ['Chiara Dottorini']
__contact__ = ['chiara.dottorini@intel.com']

log = logging.getLogger('runner.plugin.hwc_stop_valshims_checks')


def get_assets(runner, line):
    """
    This function takes the assets from xxx_url and copies them locally into a:
    local_asset_dir = ./tests/0/<asset_name>/<asset_version>/

    if <asset_name> is not specified, asset_name="local_assets"
    if <asset_version> is not specified, asset_version="<name _plugin>/<name_asset>"

    This local directory is then copied into the target into:
    target_asset_dir = /data/local/tmp/gta/<local_asset_dir>
    """

    # Check if a target environment variable has been set in the current shell
    try:
        os.environ["TARGET_PRODUCT"]
    except Exception:
        print("ERROR: environment variable TARGET_PRODUCT needs to be set")
        return 'fail'

    # Build the asset_version string
    TARGET_PRODUCT = os.environ["TARGET_PRODUCT"]
    HWCVAL_REL = "Rel5.0.11"
    asset_version = HWCVAL_REL + "/" + TARGET_PRODUCT

    return {
        "hwcval_artifactory": {
            "root_url": "gta+http://gfx-assets.intel.com/artifactory",
            "asset_path": "gfx-hwc-assets-igk",
            "asset_name": "HWCVAL",
            "asset_version": asset_version
        }
    }


def run(runner, line):
    """
    This function stops the checks done by the validation shims and
    uninstalls the HWC validation shims.
    """

    print("****************************************************************")
    print("* Stop the Shims Checks and Uninstall the HWC Validation Shims *")
    print("****************************************************************")

    # Directory on the host where to pull logs and csv files
    l_logs_dir = runner.path('logs_dir') + '/'
    runner.log_comment('local directory with results: %s' % l_logs_dir)

    # Directory on the target where assets are getting copied and
    # where log files are getting generated during execution
    r_test_dir = runner.get_asset_dest_dir("hwcval_artifactory")
    runner.log_comment('asset destination directory on the target: %s' % r_test_dir)

    # Identify the script to stop the checks and uninstall the shims
    r_test_exe = 'autoval_stop_checks.sh'
    print(r_test_dir)
    r_test_path = os.path.join(r_test_dir, r_test_exe)

    # Ensure execution rights for the script
    chmod_cmd = 'chmod 755 %s' % r_test_path
    runner.add_result('chmod_cmd', chmod_cmd)
    proc = runner.rexecute(chmod_cmd, r_test_dir)
    runner.add_result('exit_code', proc.exit_code)
    if proc.exit_code != 0:
        print(r_test_path + " " + "chmod failed")
        return 'fail'

    # Execute the script
    runner.add_result('r_test_path', r_test_path)
    proc = runner.rexecute(r_test_path, r_test_dir)
    runner.add_result('exit_code', proc.exit_code)
    if proc.exit_code != 0:
        print(r_test_path + " " + "execution failed")
        return 'fail'

    # Pull log files
    log_files_directory = "/data/validation/hwc/"
    logcat_file = os.path.join(log_files_directory, "logcat_start_android.log")
    hwclog_file = os.path.join(log_files_directory, "hwclog_start_android.log")
    runner.rpull(logcat_file, l_logs_dir)
    runner.rpull(hwclog_file, l_logs_dir)

    return 'pass'

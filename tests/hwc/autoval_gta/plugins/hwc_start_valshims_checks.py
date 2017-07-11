#
# INTEL CONFIDENTIAL
# Copyright 2015 Intel Corporation All Rights Reserved.
# The source code contained or described herein and all documents related
# to the source code ("Material") are owned by Intel Corporation or its
# suppliers or licensors. Title to the Material remains with Intel Corporation
# or its suppliers and licensors. The Material contains trade secrets and
# proprietary and confidential information of Intel or its suppliers and
# licensors. The Material is protected by worldwide copyright and trade secret
# laws and treaty provisions. No part of the Material may be used, copied,
# reproduced, modified, published, uploaded, posted, transmitted, distributed,
# or disclosed in any way without Intel's prior express written permission.
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be express
# and approved by Intel in writing.
#
"""
hwc_start_valshims_checks

Description:
    Installs the HWC validation shims if not present already.
    Starts the validation shims checks.

Example:
    For Android:
        [test]
        hwc_start_valshims_checks

Input variables:
    None
"""

import os
import logging


__authors__ = ['Chiara Dottorini']
__contact__ = ['chiara.dottorini@intel.com']

log = logging.getLogger('runner.plugin.hwc_start_valshims_checks')


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
    This function installs the shims
    """

    print("***********************************************************")
    print("* Install HWC Validation Shims and start the Shims Checks *")
    print("***********************************************************")

    # Directory on the target where assets are getting copied and
    # where log files are getting generated during execution
    r_test_dir = runner.get_asset_dest_dir("hwcval_artifactory")
    runner.log_comment('asset destination directory on the target: %s' % r_test_dir)

    # Identify the script to start the shims checks
    r_test_exe = 'autoval_start_checks.sh'
    r_test_path = os.path.join(r_test_dir, r_test_exe)

    # Ensure execution rights for the script
    chmod_cmd = 'chmod 755 %s' % r_test_path
    runner.add_result('chmod_cmd', chmod_cmd)
    proc = runner.rexecute(chmod_cmd, r_test_dir)
    runner.add_result('exit_code', proc.exit_code)
    if proc.exit_code != 0:
        return 'fail'

    # Execute the script
    runner.add_result('r_test_path', r_test_path)
    proc = runner.rexecute(r_test_path, r_test_dir)
    runner.add_result('exit_code', proc.exit_code)
    if proc.exit_code != 0:
        return 'fail'

    return 'pass'

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
hwc_run_validation

Description:
    Launches the hwcval tests

Example:
    For Android:
        [test]
        hwc_run_validation

Input variables:
    None
"""

import os
import logging
from plugins.val_hwc_utils import hwc_val_parser

__authors__ = ['Chiara Dottorini']
__contact__ = ['chiara.dottorini@intel.com']

log = logging.getLogger('runner.plugin.hwc_run_validation')


###############################################################################
# TEST POOLS
###############################################################################

HWCVAL_TESTS = ["Api", "Camera", "Dialog", "Flicker1", "Flicker2", "Flicker3",
                "FlipRot", "Gallery", "Game", "Home", "MovieStudio",
                "NetflixStepped", "Notification", "NV12FullVideo", "NV12FullVideo2",
                "NV12PartVideo", "NV12PartVideo2", "PanelFitter", "PartComp", "Png",
                "RandomModes", "Skip", "Smoke", "TransparencyComposition", "VideoModes"]

HWCVAL_CP_TESTS = ["ComplexProtectedVideo", "InvalidProtectedVideo", "ProtectedVideo",
                   "ProtectedVideoHotPlug", "ProtectedVideoScreenDisable"]

HWCVAL_WIDI_TESTS = ["WidiSmoke", "WidiDisconnect", "WidiFrameTypeChange",
                     "WidiDirect", "WidiProcessingPolicy"]

HWCVAL_HWCCOMP = ["Api_hwccomp", "ComplexProtectedVideo_hwccomp", "FlipRot_hwccomp",
                  "NV12PartVideo2_hwccomp", "Smoke_hwccomp", "TransparencyComposition_hwccomp"]

###############################################################################


def get_assets(runner, line):
    """
    This function takes the assets from xxx_url and copies them locally into a:
    local_asset_dir = ./tests/0/<asset_name>/<asset_version>/

    if <asset_name> is not specified, asset_name="local_assets"
    if <asset_version> is not specified, asset_version="<name _plugin>/<name_asset>"

    I.e.
    ./tests/0/HWCVAL/Rel3.3.2_4/run_hwcval_test.sh
    ./tests/0/local_assets/hwc_run_validation/hwcval_local/run_hwcval_test.sh

    This local directory is then copied into the target into:
    target_asset_dir = /data/local/tmp/gta/<local_asset_dir>
    """

    # Check if a target environment variable has been set in the current shell
    try:
        os.environ["TARGET_PRODUCT"]
    except ValueError:
        print("Please set the environment variable TARGET_PRODUCT")
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


def create_test_list():
    """
    This function concatenates the different lists of tests and chooses the
    correct set of widi tests for the specific target
    """

    # Check which is the appropriate set of tests to be used for this platform
    # CP tests not run on BXT targets for now
    if "bxt" in os.environ["TARGET_PRODUCT"]:
        target_suffix = "_bxt"
        WIDI_TESTS = [x + target_suffix for x in HWCVAL_WIDI_TESTS]
        HWCVAL_TEST_LIST = HWCVAL_TESTS + WIDI_TESTS + HWCVAL_HWCCOMP
    else:
        HWCVAL_TEST_LIST = HWCVAL_TESTS + HWCVAL_CP_TESTS + HWCVAL_WIDI_TESTS + HWCVAL_HWCCOMP

    print("Tests which will be executed: ")
    print(HWCVAL_TEST_LIST)


def run(runner, line):
    """
    This function executes the tests
    """
    print("****************************************")
    print("* HWC Validation Test Suite Execution  *")
    print("****************************************")

    # Create the list of tests which will be executed
    create_test_list()

    # print path of DUT tests directory
    # print(runner.rtests_dir()) #/data/local/tmp/gta/tests
    # print path of DUT temp logs directory
    # print(runner.rtemp_logs_dir()) #/data/local/tmp/gta/.gta/logs

    # Directory on the target where assets are getting copied and
    # where log files are getting generated during execution
    r_test_dir = runner.get_asset_dest_dir("hwcval_artifactory")
    runner.log_comment('asset destination directory on the target: %s' % r_test_dir)

    # Directory on the target where csv files are getting generated during execution
    r_csv_dir = '/data/validation/hwc/'
    runner.log_comment('directory on the target with csv files: %s' % r_csv_dir)

    # Directory on the host where to pull logs and csv files
    l_logs_dir = runner.path('logs_dir') + '/'
    runner.log_comment('local directory with results: %s' % l_logs_dir)

    # Identify the script to launch the tests
    r_test_exe = 'autoval_run_hwcval_test.sh'
    r_test_path = os.path.join(r_test_dir, r_test_exe)

    # Ensure execution rights for the script
    chmod_cmd = 'chmod 755 %s' % r_test_path
    runner.add_result('chmod_cmd', chmod_cmd)
    proc = runner.rexecute(chmod_cmd, r_test_dir)
    runner.add_result('exit_code', proc.exit_code)
    if proc.exit_code != 0:
        return 'fail'

    # Clean directory with the .csv files - DOES NOT WORK. TO CHECK WITH NEW VERSION.
    runner.rremove(r_csv_dir, 0)

    # List of csv files with the results
    csv_files_list = []

    # Execute each test and pull *.csv files
    for test in HWCVAL_TESTS:
        log.info("Execute " + test + " test")

        test_cmd = r_test_path + ' ' + test + ' ' + r_test_dir
        test_csv_filename = "results_" + test + ".csv"

        runner.add_result('test_cmd', test_cmd)
        proc = runner.rexecute(test_cmd, r_test_dir)
        runner.add_result('exit_code', proc.exit_code)
        if proc.exit_code != 0:
            return 'fail'

        # pull *.csv files
        r_test_csv_file = os.path.join(r_csv_dir, test_csv_filename)
        runner.rpull(r_test_csv_file, l_logs_dir)

        # add file to the list
        csv_files_list.append(test_csv_filename)

        # TODO: pull and rename of logs

    # Check if the local directory with the results exists
    if not os.path.exists(l_logs_dir):
        runner.add_error('missing %s' % l_logs_dir)
        return 'fail'

    # Apply testcheckmatrix to results_xxx.csv files
    hwc_val_parser.main(csv_files_list, l_logs_dir)

    return 'pass'

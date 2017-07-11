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
hwc_val_parser


Description:
    A script for combining HWC validation test results
    into a test check matrix, that is, a matrix comprising of the individual
    test results expressed as a series of check failures.


Input variables:
    None
"""

from __future__ import division
from __future__ import print_function
from io import open
import sys
import os
import csv
import logging


__authors__ = ['Chiara Dottorini', 'James Pascoe', 'Robert Pinsker']
__contact__ = ['chiara.dottorini@intel.com', 'james.pascoe@intel.com', 'robert.pinsker@intel.com']

log = logging.getLogger('runner.plugin.TestCheckMatrix')


# Constants

DELIMITER = u","


# Classes

class ResultCount:
    def __init__(self):
        self.passes = 0
        self.fails = 0
        self.errors = 0

    def cpass(self):
        self.passes += 1

    def cfail(self):
        self.fails += 1

    def cerr(self):
        self.errors += 1

    def add(self, other):
        self.passes += other.passes
        self.fails += other.fails
        self.errors += other.errors

    def passRate(self):
        try:
            rate = (float(self.passes) / (self.passes + self.fails + self.errors)) * 100
        except ZeroDivisionError:
            rate = 0.0
        return rate


class Check:
    def __init__(self, name, component):
        self.name = name
        self.component = component
        self.values = []
        self.count = ResultCount()

    def add(self, startValueCount, values):
        while (len(self.values) < startValueCount):
            self.values.append("")
        self.values.extend(values)


# Main Code

def main(csv_files_list, csv_files_dir):

    # Check that there is at least one file to parse in the list
    if (len(csv_files_list) < 1):
        sys.stderr.write("Error: Need to provide at least one csv file to parse. Exit. \n")
        sys.exit(0)

    # Declare lists for the file names, erred files and tests. Use dictionaries
    # for the column and component totals (see below for details)
    list_test_names = []
    checks = {}

    # The csvfiles to parse are not in the local directory -> a new list needs to be
    # generated with path+csvfile elements
    csv_files_list_with_paths = [csv_files_dir + s for s in csv_files_list]

    # Process each file in-turn
    for csv_file in csv_files_list_with_paths:

        # Perform some sanity checks
        try:
            if os.stat(csv_file).st_size == 0:
                log.debug(u"Warning: " + csv_file + u" is empty - skipping file")
                continue
        except OSError:
            log.debug(u"Warning: " + csv_file + u" can not be found - skipping file")
            continue

        # Open the file
        with open(csv_file, 'r') as csvfile:

            # Retrieve the name of the test from the string
            line = csvfile.readline().strip()
            test_name = line.split(DELIMITER)[2:]

            # Each test is assigned a column. Update the number of columns
            # Add the name of the test to the list of tests
            numColumnsSoFar = len(list_test_names)
            list_test_names += test_name

            # Load the main body of the csvfile
            for line in csvfile:
                fields = line.strip().split(DELIMITER)
                if (len(fields) < 3):
                    log.debug("Warning: ignoring malformed line - " + line)
                    continue

                # Field 0 is the check name. Field 1 is the component and Fields 2..n
                # are the test value (blank indicates that the check was not exercised,
                # 0 indicates that the check was exercised but was not triggered and a
                # positive value of N indicates that the check failed N times
                checkName = fields[0]
                component = fields[1]

                if checkName in checks:
                    checks[checkName].add(numColumnsSoFar, fields[2:])
                else:
                    checkData = Check(checkName, component)
                    checkData.add(numColumnsSoFar, fields[2:])
                    checks[checkName] = checkData

    # Generate the row, column and component totals
    columnTotals = []
    componentTotals = {}
    for test in list_test_names:
        columnTotals.append(ResultCount())

    for checkName, check in checks.items():
        for c in range(len(check.values)):
            value = check.values[c]

            if check.component in componentTotals:
                comp = componentTotals[check.component]
            else:
                comp = ResultCount()

            if value == "":
                pass    # no result
            elif int(value) == 0:
                check.count.cpass()
                columnTotals[c].cpass()
                comp.cpass()
            else:
                if check.component == "Test":
                    check.count.cerr()
                    columnTotals[c].cerr()
                    comp.cerr()
                else:
                    check.count.cfail()
                    columnTotals[c].cfail()
                    comp.cfail()

            componentTotals[check.component] = comp

    # Generate grand total
    grandTotal = ResultCount()
    for compName, comp in componentTotals.items():
        grandTotal.add(comp)

    # Define and open the output file
    csv_results_final = os.path.join(csv_files_dir, "results_final.csv")
    with open(csv_results_final, "wt+") as out:

        # Write the output

        if not list_test_names:
            log.debug(u"No valid input data")
            sys.exit(1)

        writerobj = csv.writer(out)

        # Print the main summary section
        writerobj.writerow(["Grand Total", "Passes", "Fails", "Errors", "Pass Rate"])
        writerobj.writerow(["Total",
                            str(grandTotal.passes),
                            str(grandTotal.fails),
                            str(grandTotal.errors),
                            str(grandTotal.passRate()) + "%"])
        # Blank row
        writerobj.writerow("")

        # Print the component summary section
        writerobj.writerow(["Component", "Passes", "Fails", "Errors", "Pass Rate"])
        for component, values in iter(sorted(componentTotals.items())):
            writerobj.writerow([component,
                                str(values.passes),
                                str(values.fails),
                                str(values.errors),
                                str(values.passRate()) + "%"])
        # Blank row
        writerobj.writerow("")

        # Print the table header for the detailed section
        print(u"Check Name" + DELIMITER + u"Component", end=u"", file=out)
        for test in list_test_names:
            print(DELIMITER + test, end=u"", file=out)
        print(DELIMITER + u"Passes" + DELIMITER + u"Fails" + DELIMITER + u"Errors" + DELIMITER + u"Pass Rate", file=out)

        # Blank row
        writerobj.writerow("")

        # Print checks and final results
        for checkName, check in iter(sorted(checks.items())):
            print(check.name + DELIMITER + check.component, end=u"", file=out)

            for value in check.values:
                print(DELIMITER + value, end=u"", file=out)

            pr = check.count.passRate()
            print(DELIMITER + str(check.count.passes) + DELIMITER + str(check.count.fails) +
                  DELIMITER + str(check.count.errors) +
                  DELIMITER + str(pr) + u"%", file=out)

        # Blank row
        writerobj.writerow("")

        print(u"Passes" + DELIMITER, end=u"", file=out)
        for t in columnTotals:
            print(DELIMITER + str(t.passes), end=u"", file=out)
        writerobj.writerow("")

        print(u"Fails" + DELIMITER, end=u"", file=out)
        for t in columnTotals:
            print(DELIMITER + str(t.fails), end=u"", file=out)
        writerobj.writerow("")

        print(u"Errors" + DELIMITER, end=u"", file=out)
        for t in columnTotals:
            print(DELIMITER + str(t.errors), end=u"", file=out)
        writerobj.writerow("")

        print(u"Pass Rate" + DELIMITER, end=u"", file=out)
        for t in columnTotals:
            print(DELIMITER + str(t.passRate()) + u"%", end=u"", file=out)
        writerobj.writerow("")

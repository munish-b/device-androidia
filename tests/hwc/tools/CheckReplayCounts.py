#!/usr/bin/env python3

'''
CheckReplayCounts: A script to count the number of 'onSet Entry' headers
and layers. Extraction stops when the layer regex fails to match, at
which point the script proceeds to the next 'onSet Entry'.

This is useful as an independent verification of the Replay tool. The
statistics produced at the end of a replay can be checked with the values
produced by this script to identify parsing (or other) problems.
'''

import sys
import re

import PyReplay

__author__     = 'James Pascoe'
__copyright__  = 'Copyright 2015 Intel Corporation'
__version__    = '1.0'
__maintainer__ = 'James Pascoe'
__email__      = 'james.pascoe@intel.com'
__status__     = 'Alpha'

### Main Code ###

if __name__ == '__main__':
    if (len(sys.argv) != 2):
        print("Usage {0}: filename.log".format(sys.argv[0]), file=sys.stderr)
        exit(-1)

    onset_regex = re.compile(PyReplay.onset_pattern)
    layer_regex = re.compile(PyReplay.layer_hdr_pattern)
    onset_count = layer_count = 0

    file = open(sys.argv[1],"r")
    for line in file:
        if onset_regex.search(line):
            onset_count += 1

            line = file.readline()
            while(layer_regex.search(line)):
                layer_count += 1
                line = file.readline()

    print("Frame count: " + str(onset_count))
    print("Layer count: " + str(layer_count))

#!/usr/bin/env python

"""
This script is mainly for running autotests on the build server, however, it 
can also be used by engineers to run the tests locally on their machines.

It takes as optional parameters the path to the folder containing the test 
executables (which must have names ending in _tests), and a list of tests that 
need to be skipped, this list must be comma separated and contain no spaces. E.g.:

./run_desktop_tests.py -f ./omim-build-release -e drape_tests,some_other_tests

The script outputs the console output of the tests. It also checks the error 
code of each test suite, and after all the tests are executed, it prints the 
list of the failed tests, passed tests, skipped tests and tests that could not 
be found, i.e. the tests that were specified in the skip list, but do not exist.
"""

from __future__ import print_function
from os import listdir
from os.path import isfile, join
import subprocess
import sys
import getopt


tests_path = ""
workspace_path = "./omim-build-release"
skiplist = []


def print_pretty(result, tests):
    if len(tests) == 0:
        return

    print("")
    print(result.upper())

    for test in tests:
        print("- {}".format(test))


def usage():
    print("""
Possbile options:

-h --help   : print this help

-f --folder : specify the folder where the tests reside (absolute path or relative to the location of this script)

-e --exclude: list of tests to exclude, comma separated, no spaces allowed


Example

./run_desktop_tests.py -f /Users/Jenkins/Home/jobs/Multiplatform/workspace/omim-build-release -e drape_tests,some_other_tests
""")


def set_global_vars():
    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            "he:f:",
            ["help", "exclude=", "folder="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)

    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-e", "--exclude"):
            exclude_tests = a.split(",")
            for exclude_test in exclude_tests:
                global skiplist
                skiplist.append(exclude_test)
        elif o in ("-f", "--folder"):
            global workspace_path
            workspace_path = a
        else:
            assert False, "unhandled option"


def run_tests():
    tests_path = "{workspace_path}/out/release".format(
        workspace_path=workspace_path)

    failed = []
    passed = []
    skipped = []

    for file in listdir(tests_path):

        if not file.endswith("_tests"):
            continue
        if file in skiplist:
            print("===== Skipping {} =====".format(file))
            skipped.append(file)
            continue

        process = subprocess.Popen(
            tests_path +
            "/" +
            file,
            shell=True,
            stdout=subprocess.PIPE)
        process.wait()
        if process.returncode > 0:
            failed.append(file)
        else:
            passed.append(file)

    return {"failed": failed, "passed": passed, "skipped": skipped}


def main():
    set_global_vars()

    results = run_tests()

    print_pretty("failed", results["failed"])
    print_pretty("skipped", results["skipped"])
    print_pretty("passed", results["passed"])

    not_found = filter(lambda x: x not in results["skipped"], skiplist)

    print_pretty("not found", not_found)

    if len(results["failed"]) > 0:
        exit(1)


if (__name__ == "__main__"):
    main()


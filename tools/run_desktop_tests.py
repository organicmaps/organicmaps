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

import getopt
from os import listdir
from os.path import isfile, join
import os
import socket
import subprocess
import sys
import testserver
import urllib2


tests_path = ""
workspace_path = "omim-build-release/out/release"
skiplist = []
runlist = []
logfile = "testlog.log"
data_path = None
user_resource_path = None



TO_RUN = "to_run"
SKIP = "skip"
NOT_FOUND = "not_found"
FAILED = "failed"
PASSED = "passed"

PORT = 34568

def print_pretty(result, tests):
    if len(tests) == 0:
        return

    print("")
    print(result.upper())

    for test in tests:
        print("- {test}".format(test=test))


def usage():
    print("""
Possbile options:

-h --help   : print this help

-f --folder : specify the folder where the tests reside (absolute path or relative to the location of this script)

-e --exclude: list of tests to exclude, comma separated, no spaces allowed

-i --include: list of tests to be run, overrides -e

-o --output : resulting log file. Default testlog.log

-d --data_path : Path to data files (passed to the test executables as --data_path=<value>)

-u --user_resource_path : Path to resources, styles and classificators (passed to the test executables as --user_resource_path=<value>)  


Example

./run_desktop_tests.py -f /Users/Jenkins/Home/jobs/Multiplatform/workspace/omim-build-release/out/release -e drape_tests,some_other_tests -o my_log_file.log
""")


def set_global_vars():
    
    global skiplist
    global logfile
    global runlist
    global workspace_path
    global data_path
    global user_resource_path
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "he:f:o:i:d:u:",
                                   ["help", "exclude=", "include=", "folder=", "output=", "data_path=", "user_resource_path="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)

    for option, argument in opts:
        if option in ("-h", "--help"):
            usage()
            sys.exit()
        if option in ("-o", "--output"):
            logfile = argument
        elif option in ("-e", "--exclude"):
            skiplist = list(set(argument.split(",")))
        elif option in ("-i", "--include"):
            print("\n-i option found, -e option will be ignored!")
            runlist = argument.split(",")
        elif option in ("-f", "--folder"):
            workspace_path = argument
        elif option in ("-d", "--data_path"):
            data_path = " --data_path={argument} ".format(argument=argument)
        elif option in ("-u", "--user_resource_path"):
            user_resource_path = " --user_resource_path={argument} ".format(argument=argument)
            
        else:
            assert False, "unhandled option"


def start_server():
    server = testserver.TestServer()
    server.start_serving()


def stop_server():
    try:
        urllib2.urlopen('http://localhost:{port}/kill'.format(port=PORT), timeout=5)
    except (urllib2.URLError, socket.timeout):
        print("Failed to stop the server...")


def categorize_tests():
    global skiplist

    tests_to_run = []
    local_skiplist = []
    not_found = []
    
    test_files_in_dir = filter(lambda x: x.endswith("_tests"), listdir(workspace_path))

    on_disk = lambda x: x in test_files_in_dir
    not_on_disk = lambda x : not on_disk(x)

    if len(runlist) == 0:
        local_skiplist = filter(on_disk, skiplist)
        not_found = filter(not_on_disk, skiplist)
        tests_to_run = filter(lambda x: x not in local_skiplist, test_files_in_dir)
    else:
        tests_to_run = filter(on_disk, runlist)
        not_found = filter(not_on_disk, runlist)

    return {TO_RUN:tests_to_run, SKIP:local_skiplist, NOT_FOUND:not_found}        
        

def run_tests(tests_to_run):

    failed = []
    passed = []

    server = None
    
    for file in tests_to_run:

        if file == "platform_tests":
            start_server()
        
        file="{file}{data}{resources}".format(file=file, data=data_path, resources=user_resource_path)
        
        process = subprocess.Popen("{tests_path}/{file} 2>> {logfile}".
                                   format(tests_path=workspace_path, file=file, logfile=logfile),
                                   shell=True,
                                   stdout=subprocess.PIPE)

        process.wait()

        if file == "platform_tests":
            stop_server()

        if process.returncode > 0:
            failed.append(file)
        else:
            passed.append(file)

    return {FAILED: failed, PASSED: passed}


def rm_log_file():
    try:
        os.remove(logfile)
    except OSError:
        pass


def main():
    set_global_vars()
    rm_log_file()

    categorized_tests = categorize_tests()

    results = run_tests(categorized_tests[TO_RUN])

    print_pretty("failed", results[FAILED])
    print_pretty("skipped", categorized_tests[SKIP])
    print_pretty("passed", results[PASSED])
    print_pretty("not found", categorized_tests[NOT_FOUND])


if (__name__ == "__main__"):
    main()


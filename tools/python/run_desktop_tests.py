#!/usr/bin/env python3

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

from optparse import OptionParser
from os import listdir, remove, environ
from random import shuffle
import random
import socket
import subprocess
import testserver
import time
from urllib.error import URLError
from urllib.request import urlopen

import logging

TO_RUN = "to_run"
SKIP = "skip"
NOT_FOUND = "not_found"
FAILED = "failed"
PASSED = "passed"
WITH_SERVER = "with_server"

PORT = 34568

TESTS_REQUIRING_SERVER = ["platform_tests", "storage_tests", "map_tests"]

class TestRunner:

    def print_pretty(self, result, tests):
        if not tests:
            return

        logging.info(f"\n{result.upper()}")

        for test in tests:
            logging.info(f"- {test}")


    def set_global_vars(self):

        parser = OptionParser()
        parser.add_option("-f", "--folder", dest="folder", default="omim-build-release/out/release", help="specify the folder where the tests reside (absolute path or relative to the location of this script)")
        parser.add_option("-d", "--data_path", dest="data_path", help="Path to data files (passed to the test executables as --data_path=<value>)")
        parser.add_option("-u", "--user_resource_path", dest="resource_path", help="Path to resources, styles and classificators (passed to the test executables as --user_resource_path=<value>)")
        parser.add_option("-i", "--include", dest="runlist", action="append", default=[], help="Include test into execution, comma separated list with no spaces or individual tests, or both. E.g.: -i one -i two -i three,four,five")
        parser.add_option("-e", "--exclude", dest="skiplist", action="append", default=[], help="Exclude test from execution, comma separated list with no spaces or individual tests, or both. E.g.: -i one -i two -i three,four,five")
        parser.add_option("-b", "--boost_tests", dest="boost_tests", action="store_true", default=False, help="Treat all the tests as boost tests (their output is different and it must be processed differently).")
        parser.add_option("-k", "--keep_alive", dest="keep_alive", action="store_true", default=False, help="Keep the server alive after the end of the test. Because the server sometimes fails to start, this reduces the probability of false test failures on CI servers.")

        (options, args) = parser.parse_args()

        self.skiplist = set()
        self.runlist = []

        for tests in options.skiplist:
            for test in tests.split(","):
                self.skiplist.add(test)

        for tests in options.runlist:
            self.runlist.extend(tests.split(","))

        self.boost_tests = options.boost_tests

        if self.runlist:
            logging.warn("-i or -b option found, the -e option will be ignored")

        self.workspace_path = options.folder
        self.data_path = (f" --data_path={options.data_path}" if options.data_path else "")
        self.user_resource_path = (f" --user_resource_path={options.resource_path}" if options.resource_path else "")

        self.keep_alive = options.keep_alive


    def start_server(self):
        server = testserver.TestServer()
        server.start_serving()
        time.sleep(3)


    def stop_server(self):
        if self.keep_alive:
            return
        try:
            urlopen(f"http://localhost:{PORT}/kill", timeout=5)
        except (URLError, socket.timeout):
            logging.info("Failed to stop the server...")

    def categorize_tests(self):

        tests_to_run = []
        local_skiplist = []
        not_found = []

        test_files_in_dir = list(filter(lambda x: x.endswith("_tests"), listdir(self.workspace_path)))

        on_disk = lambda x: x in test_files_in_dir
        not_on_disk = lambda x : not on_disk(x)

        if not self.runlist:
            local_skiplist = list(filter(on_disk, self.skiplist))
            not_found = list(filter(not_on_disk, self.skiplist))
            tests_to_run = list(filter(lambda x: x not in local_skiplist, test_files_in_dir))
        else:
            tests_to_run = list(filter(on_disk, self.runlist))
            shuffle(tests_to_run)

            not_found = list(filter(not_on_disk, self.runlist))

        # now let's move the tests that need a server either to the beginning or the end of the tests_to_run list
        tests_with_server = TESTS_REQUIRING_SERVER[:]
        for test in TESTS_REQUIRING_SERVER:
            if test in tests_to_run:
                tests_to_run.remove(test)
            else:
                tests_with_server.remove(test)

        return {TO_RUN:tests_to_run, SKIP:local_skiplist, NOT_FOUND:not_found, WITH_SERVER:tests_with_server}


    def test_file_with_keys(self, test_file):
        boost_keys = " --report_format=xml --report_level=detailed --log_level=test_suite --log_format=xml " if self.boost_tests else ""
        return f"{test_file}{boost_keys}{self.data_path}{self.user_resource_path}"


    def run_tests(self, tests_to_run):
        failed = []
        passed = []

        for test_file in tests_to_run:

            test_file_with_keys = self.test_file_with_keys(test_file)

            logging.info(test_file_with_keys)

            # Fix float parsing
            # See more here:
            # https://github.com/mapsme/omim/pull/996
            current_env = environ
            current_env['LC_NUMERIC'] = 'C'
            #
            process = subprocess.Popen(f"{self.workspace_path}/{test_file_with_keys}",
                                   env=current_env,
                                   shell=True,
                                   stdout=subprocess.PIPE)
            logging.info(f"Pid: {process.pid}")

            process.wait()

            if process.returncode > 0:
                failed.append(test_file)
            else:
                passed.append(test_file)

        return {FAILED: failed, PASSED: passed}


    def __init__(self):
        self.set_global_vars()


    def execute(self):
        categorized_tests = self.categorize_tests()

        to_run_and_with_server_keys = [TO_RUN, WITH_SERVER]
        random.shuffle(to_run_and_with_server_keys)

        results = {FAILED: [], PASSED: []}

        for key in to_run_and_with_server_keys:
            if key == WITH_SERVER and categorized_tests[WITH_SERVER]:
                self.start_server()
            result = self.run_tests(categorized_tests[key])
            results[FAILED].extend(result[FAILED])
            results[PASSED].extend(result[PASSED])
            if key == WITH_SERVER and categorized_tests[WITH_SERVER]:
                self.stop_server()

        self.print_pretty("failed", results[FAILED])
        self.print_pretty("skipped", categorized_tests[SKIP])
        self.print_pretty("passed", results[PASSED])
        self.print_pretty("not found", categorized_tests[NOT_FOUND])
        return len(results[FAILED]) > 0 and 1 or 0

def tests_on_disk(path):
    return list(filter(lambda x: x.endswith("_tests"), listdir(path)))


if __name__ == "__main__":
    runner = TestRunner()
    quit(runner.execute())

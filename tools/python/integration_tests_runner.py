from __future__ import print_function

import logging
from contextlib import contextmanager
from multiprocessing import cpu_count
import multiprocessing
from optparse import OptionParser
from os import path
import shutil
import subprocess
import tempfile

from run_desktop_tests import tests_on_disk

__author__ = 't.danshin'

TEST_RESULT_LOG = "test_result_log"
TEMPFOLDER_TESTS = ["search_integration_tests", "storage_integration_tests"]

FILTER_KEY = "--filter"
DATA_PATH_KEY = "--data_path"
RESOURCE_PATH_KEY = "--user_resource_path"


@contextmanager
def TemporaryDirectory():
    name = tempfile.mkdtemp()
    try:
        yield name
    finally:
        shutil.rmtree(name)


def setup_test_result_log(log_file, level=logging.INFO):
    logger = logging.getLogger(TEST_RESULT_LOG)
    formatter = logging.Formatter("BEGIN: %(file)s\n%(message)s\nEND: %(file)s | result: %(result)d\n")
    file_handler = logging.FileHandler(log_file, mode='w')
    file_handler.setFormatter(formatter)
    logger.setLevel(level)
    logger.propagate = False
    logger.addHandler(file_handler)


def setup_jenkins_console_logger(level=logging.INFO):
    formatter = logging.Formatter('%(process)s: %(msg)s')
    # Time is logged by Jenkins. Any log files on disk will be removed by
    # Jenkins when the next job starts
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    multiprocessing.get_logger().setLevel(level)
    multiprocessing.get_logger().addHandler(stream_handler)


def with_logging(fn):
    def func_wrapper(test, flags):
        logger = multiprocessing.get_logger()
        logger.info("start: >{0} {1}".format(test, flags))
        result = fn(test, flags)
        logger.info("end: >{0} {1}".format(test, flags))
        return result

    return func_wrapper


@with_logging
def spawn_test_process(test, flags):
    spell = ["{0} {1}".format(test, flags)]

    process = subprocess.Popen(spell,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               shell=True
                               )

    multiprocessing.get_logger().info(spell[0])
    out, err = process.communicate()

    return filter(None, out.splitlines()), err, process.returncode


def exec_test(a_tuple):
    """
    Executes a test and returns the result
    :param a_tuple: a tuple consisting of
    * the path to the test file (e.g. ..../base_tests)
    * the name of the test to be executed from that file
    * dictionary with all the parameters to be passed to the executable. At
    this point the flags may contain --user_resource_path and --data_path if the
    executable file is not in the list of the tests that require temporary folders

    :return: (the name of the file, the name of the test),
    (standard error from the test, test's exit code)
    """
    test_file, test_name, params = a_tuple
    params[FILTER_KEY] = test_name

    out, err, result = None, None, None

    if test_file in TEMPFOLDER_TESTS:
        out, err, result = exec_test_with_temp(test_file, params)
    else:
        out, err, result = exec_test_without_temp(test_file, params)

    return (test_file, test_name), (err, result)


def exec_test_with_temp(test_file, params):
    with TemporaryDirectory() as tmpdir:
        params[DATA_PATH_KEY] = tmpdir
        return exec_test_without_temp(test_file, params)

def exec_test_without_temp(test_file, params):
    flags = params_from_dict(params)
    return spawn_test_process(test_file, flags)


def params_from_dict(params_dict):
    return " ".join(
        '"{0}={1}"'.format(key, value)
        for key, value in params_dict.iteritems()
        if value != ""
    )


class IntegrationRunner:
    def __init__(self):
        proc_count = cpu_count() / 5 * 4
        if proc_count < 1:
            proc_count = 1
        self.pool = multiprocessing.Pool(proc_count, initializer=setup_jenkins_console_logger)
        self.workspace_path = ""
        self.runlist = []
        self.params = {}
        self.process_cli()
        multiprocessing.get_logger().info(
            "\n{0}\nIntegration tests runner started.\n{0}\n".format(
                "*" * 80
            )
        )


    def run_tests(self):
        logger = logging.getLogger(TEST_RESULT_LOG)
        try:
            test_results = self.pool.imap_unordered(
                exec_test,
                self.prepare_list_of_tests()
            )
            for (test_file, _), (err, result) in test_results:
                logger.info(
                    err,
                    extra={"file" : path.basename(test_file), "result" : result}
                )

        finally:
            self.pool.close()
            self.pool.join()


    def map_args(self, test):
        test_full_path = path.join(self.workspace_path, test)
        tests = spawn_test_process(test_full_path, "--list_tests")[0]  # a list

        return map(
            lambda tests_in_file: (test_full_path, tests_in_file, self.params),
            tests
        )


    def prepare_list_of_tests(self):
        for exec_file in self.runlist:
            for test_tuple in self.map_args(exec_file):
                yield test_tuple


    def set_instance_vars_from_options(self, options):
        self.workspace_path = options.folder
        for opt in options.runlist:
            self.runlist.extend(map(str.strip, opt.split(",")))

        tests_on_disk_list = tests_on_disk(self.workspace_path)
        self.runlist = filter(lambda x: x in tests_on_disk_list, self.runlist)

        self.params[RESOURCE_PATH_KEY] = options.user_resource_path
        self.params[DATA_PATH_KEY] = options.data_path


    def process_cli(self):
        parser = self.prepare_cli_parser()
        (options, args) = parser.parse_args()

        if not options.runlist:
            parser.print_help()
            raise Exception("You must provide a list of tests to run")

        self.set_instance_vars_from_options(options)

        setup_test_result_log(options.output)
        setup_jenkins_console_logger()

        if options.log_start_finish:
            multiprocessing.get_logger().warning("The -l option is now deprecated. Please, remove it from your build scripts. It may be removed at any time.")


    def prepare_cli_parser(self):
        parser = OptionParser()
        parser.add_option(
            "-o", "--output",
            dest="output", default="testlog.log",
            help="resulting log file. Default testlog.log"
        )
        parser.add_option(
            "-f", "--folder",
            dest="folder", default="omim-build-release/out/release",
            help="specify the folder where the tests reside (absolute path or relative to the location of this script)"
        )
        parser.add_option(
            "-i", "--include",
            dest="runlist", action="append", default=[],
            help="Include test into execution, comma separated list with no spaces or individual tests, or both. E.g.: -i one -i two -i three,four,five"
        )
        parser.add_option(
            "-r", "--user_resource_path",
            dest="user_resource_path", default="",
            help="Path to user resources, such as MWMs"
        )
        parser.add_option(
            "-d", "--data_path",
            dest="data_path", default="",
            help="Path to the writable dir"
        )
        parser.add_option(
            "-l", "--log_start_finish",
            dest="log_start_finish", action="store_true", default=False,
            help="Write to log each time a test starts or finishes. May be useful if you need to find out which of the tests runs for how long, and which test hang. May slow down the execution of tests."
        )
        return parser


if __name__ == "__main__":
    runner = IntegrationRunner()
    runner.run_tests()
    multiprocessing.get_logger().info("Done")

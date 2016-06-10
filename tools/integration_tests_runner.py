from __future__ import print_function

import logging
import multiprocessing
from optparse import OptionParser
from os import path
from Queue import Queue
from random import shuffle
import shutil
import subprocess
import tempfile
from threading import Lock
from threading import Thread
from time import time
import traceback


from run_desktop_tests import tests_on_disk

__author__ = 't.danshin'


TEMPFOLDER_TESTS = ["search_integration_tests", "storage_integration_tests"]


class IntegrationRunner:
    def __init__(self):
        self.process_cli()

        self.proc_count = multiprocessing.cpu_count()
        logging.info("Number of processors is: {nproc}".format(nproc=self.proc_count))

        self.file_lock = Lock()
        self.start_finish_lock = Lock()
        self.tests = Queue()
        self.key_postfix = ""
        if self.user_resource_path:
            self.key_postfix += ' --user_resource_path="{0}"'.format(self.user_resource_path)
        if self.data_path:
            self.key_postfix += ' --data_path="{0}"'.format(self.data_path)


    def run_tests(self):
        intermediate_tests = []
        for exec_file in self.runlist:
            intermediate_tests.extend(map(lambda x: (exec_file, x), self.get_tests_from_exec_file(exec_file, "--list_tests")[0]))

        shuffle(intermediate_tests)
        for test in intermediate_tests:
            self.tests.put(test)

        with open(self.output, "w") as self.file, open("start-finish.log", "w") as self.start_finish_log:
            self.run_parallel_tests()


    def run_parallel_tests(self):
        threads = []

        for i in range(0, self.proc_count):
            thread = Thread(target=self.exec_tests_in_queue)
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()


    def exec_tests_in_queue(self):
        while not self.tests.empty():
            try:
                test_file, test = self.tests.get()
                self.exec_test(test_file, test, clean_env=(test_file in TEMPFOLDER_TESTS))

            except:
                logging.error(traceback.format_exc())
                return


    def log_start_finish(self, test_file, keys, start=False, finish=False):
        if not self.write_start_finish_log:
            return

        if (not start and not finish) or (start and finish):
            logging.warning("You need to pass either start=True or finish=True, but only one of them! You passed start={0}, finish={1}".format(start, finish))
            return

        string = "Started" if start else "Finished"

        with self.start_finish_lock:
            self.start_finish_log.write("{string} {test_file} {keys} at {time}\n".format(string=string, test_file=test_file, keys=keys, time=time()))
            self.start_finish_log.flush()


    def exec_test(self, test_file, test, clean_env=False):
        keys = '"--filter={test}"'.format(test=test)
        if clean_env:
            tmpdir = tempfile.mkdtemp()
            keys = '{old_key} "--data_path={tmpdir}"'.format(old_key=keys, tmpdir=tmpdir)
            logging.debug("Temp dir: {tmpdir}".format(tmpdir=tmpdir))
        else:
            keys = "{old_key}{resource_path}".format(old_key=keys, resource_path=self.key_postfix)
            logging.debug("Setting user_resource_path and data_path to {resource_path}".format(resource_path=self.key_postfix))

        self.log_start_finish(test_file, keys, start=True)
        out, err, result = self.get_tests_from_exec_file(test_file, keys)
        self.log_start_finish(test_file, keys, finish=True)

        if clean_env:
            try:
                shutil.rmtree(tmpdir)
            except:
                logging.error("Failed to remove tempdir {tmpdir}".format(tmpdir=tmpdir))

        with self.file_lock:
            self.file.write("BEGIN: {file}\n".format(file=test_file))
            self.file.write(str(err))
            self.file.write("\nEND: {file} | result: {res}\n\n".format(file=test_file, res=result))
            self.file.flush()


    def get_tests_from_exec_file(self, test, keys):
        spell = ["{test} {keys}".format(test=path.join(self.workspace_path, test), keys=keys)]
        logging.debug(">> {spell}".format(spell=spell))

        process = subprocess.Popen(spell,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   shell=True
                                   )

        out, err = process.communicate()
        result = process.returncode

        return filter(lambda x: x,  out.split("\n")), err, result


    def process_cli(self):
        parser = OptionParser()
        parser.add_option("-o", "--output", dest="output", default="testlog.log", help="resulting log file. Default testlog.log")
        parser.add_option("-f", "--folder", dest="folder", default="omim-build-release/out/release", help="specify the folder where the tests reside (absolute path or relative to the location of this script)")
        parser.add_option("-i", "--include", dest="runlist", action="append", default=[], help="Include test into execution, comma separated list with no spaces or individual tests, or both. E.g.: -i one -i two -i three,four,five")
        parser.add_option("-r", "--user_resource_path", dest="user_resource_path", default="", help="Path to user resources, such as MWMs")
        parser.add_option("-d", "--data_path", dest="data_path", default="", help="Path to the writable dir")
        parser.add_option("-l", "--log_start_finish", dest="log_start_finish", action="store_true", default=False,
                          help="Write to log each time a test starts or finishes. May be useful if you need to find out which of the tests runs for how long, and which test hang. May slow down the execution of tests.")

        (options, args) = parser.parse_args()

        if not options.runlist:
            logging.warn("You must provide the list of tests to run. This runner doesn't run all the tests it finds, only the ones you specify.")
            exit(2)

        self.workspace_path = options.folder
        interim_runlist = list()
        for opt in options.runlist:
            interim_runlist.extend(map(lambda x: x.strip(), opt.split(",")))

        self.runlist = filter(lambda x: x in tests_on_disk(self.workspace_path), interim_runlist)
        self.output = options.output
        self.user_resource_path = options.user_resource_path
        self.data_path = options.data_path
        self.write_start_finish_log = options.log_start_finish


if __name__ == "__main__":
    runner = IntegrationRunner()
    runner.run_tests()

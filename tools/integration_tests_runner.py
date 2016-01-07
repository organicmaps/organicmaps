from __future__ import print_function
from optparse import OptionParser
import subprocess
import multiprocessing
from threading import Lock
from threading import Thread
import traceback
import logging

from run_desktop_tests import tests_on_disk

__author__ = 't.danshin'


class IntegrationRunner:
    def __init__(self):
        self.process_cli()

        self.proc_count = multiprocessing.cpu_count()
        logging.info("Number of processors is: {}".format(self.proc_count))

        self.file_lock = Lock()
        self.queue_lock = Lock()

        self.tests = list()

    def run_tests(self):
        for exec_file in self.runlist:
            tests = list(self.get_tests_from_exec_file(exec_file, "--list_tests")[0])[::-1]
            self.tests.extend(map(lambda x: (exec_file, x),  tests))

        self.file = open(self.output, "w")
        self.run_parallel_tests()
        self.file.close()


    def run_parallel_tests(self):
        threads = list()

        for i in range(0, self.proc_count):
            thread = Thread(target=self.exec_tests_in_queue)
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()


    def exec_tests_in_queue(self):
        while True:
            try:
                self.queue_lock.acquire()
                if not len(self.tests):
                    return

                test_file, test = self.tests.pop()

                self.queue_lock.release()
                self.exec_test(test_file, test)
            except:
                logging.error(traceback.format_exc())

            finally:
                if self.queue_lock.locked():
                    self.queue_lock.release()


    def exec_test(self, test_file, test):
        out, err, result = self.get_tests_from_exec_file(test_file, '--filter={test}'.format(test=test))

        try:
            self.file_lock.acquire()
            self.file.write("BEGIN: {}\n".format(test_file))
            self.file.write(str(err))
            self.file.write("\nEND: {} | result: {}\n\n".format(test_file, result))
            self.file.flush()
        finally:
            self.file_lock.release()


    def get_tests_from_exec_file(self, test, keys):
        spell = "{tests_path}/{test} {keys}".format(tests_path=self.workspace_path, test=test, keys=keys)

        process = subprocess.Popen(spell.split(" "),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE
                                   )

        out, err = process.communicate()
        result = process.returncode

        return filter(lambda x: x,  out.split("\n")), err, result


    def process_cli(self):
        parser = OptionParser()
        parser.add_option("-o", "--output", dest="output", default="testlog.log", help="resulting log file. Default testlog.log")
        parser.add_option("-f", "--folder", dest="folder", default="omim-build-release/out/release", help="specify the folder where the tests reside (absolute path or relative to the location of this script)")
        parser.add_option("-i", "--include", dest="runlist", action="append", default=[], help="Include test into execution, comma separated list with no spaces or individual tests, or both. E.g.: -i one -i two -i three,four,five")

        (options, args) = parser.parse_args()

        if not options.runlist:
            logging.warn("You must provide the list of tests to run. This runner doesn't run all the tests it finds, only the ones you specify.")
            exit(2)

        self.workspace_path = options.folder
        self.runlist = filter(lambda x: x in tests_on_disk(self.workspace_path), options.runlist)
        self.output = options.output


def main():
    runner = IntegrationRunner()
    runner.run_tests()

if __name__ == "__main__":
    main()

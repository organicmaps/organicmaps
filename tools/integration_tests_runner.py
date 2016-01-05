from __future__ import print_function


__author__ = 't.danshin'

import subprocess
import multiprocessing
from threading import Lock
from threading import Thread
import traceback

#The idea is that we should run integration tests in parallel. To that end, we need to first get all tests, create a queue and feed tests to it as soon as executors in that queue become free.



class IntegrationRunner:
    def __init__(self, exec_file):
        self.exec_file = exec_file
        print("Exec file " + exec_file)
        self.workspace_path = "/Users/t.danshin/Documents/projects/omim-build-release/out/release"
        self.proc_count = multiprocessing.cpu_count()

        self.file_lock = Lock()
        self.queue_lock = Lock()

        self.tests = list(self.get_tests_from_exec_file(exec_file, "--list_tests")[0])
        self.tests.reverse()
        print("Self.tests are: ")
        print(self.tests)

        # self.buckets = self.split_tests_into_buckets()

        # print("Bucket 1 = ")
        # print(self.buckets[1])

        self.file = open("python_log.log", "w")


        # self.exec_tests_in_bucket(self.buckets[1])

        threads = list()

        for i in range(0, self.proc_count):
            thread = Thread(target=self.exec_tests_in_bucket)
            thread.start()
            threads.append(thread)

        # for bucket in self.buckets:
        #     thread = Thread(target=self.exec_tests_in_bucket, args=(bucket,))
        #     thread.start()
        #     threads.append(thread)

        for thread in threads:
            thread.join()


        self.file.close()

    # def split_tests_into_buckets(self):
    #     test_buckets = list()
    #     for i in range(0, self.proc_count):
    #         test_buckets.append(list())
    #     i = 0
    #     for test in self.tests:
    #         test_bucket = i % len(test_buckets)
    #         print(">> " + str(test_bucket))
    #         test_buckets[test_bucket].append(test)
    #         i += 1
    #
    #
    #     return test_buckets

    def exec_tests_in_bucket(self):
        while True:
            try:
                self.queue_lock.acquire()
                if not len(self.tests):
                    print("Len of tests is: " + str(len(self.tests)))
                    print("Returning because nothing is left in the queue")
                    return

                test = self.tests.pop()
                self.queue_lock.release()
                print("Added test: " + test)

                out, err = self.get_tests_from_exec_file(self.exec_file, '--filter={test}'.format(test=test))

                print("Finished " + test)
                print("Err: >> " + str(err))

                print("Out: >> " + str(out))

                try:
                    self.file_lock.acquire()
                    self.file.write(str(err))
                finally:
                    self.file_lock.release()

            except:
                traceback.print_exc()

            finally:
                if self.queue_lock.locked():
                    self.queue_lock.release()
                # return






    def get_tests_from_exec_file(self, test, keys):
        spell = "{tests_path}/{test} {keys}".format(tests_path=self.workspace_path, test=test, keys=keys)

        print("Spell = " + spell)
        process = subprocess.Popen(spell.split(" "),
                                   # shell=True,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE
                                   )

        out, err = process.communicate()
#        process.wait()

        print("out = " + str(out))
        print("err = " + str(err))

        return (filter(lambda x: x,  out.split("\n")), err)


def main():
    exec_file =  "pedestrian_routing_tests"
    runner = IntegrationRunner(exec_file)


if __name__ == "__main__":
    main()

"""
recalculate_geom_index.py <data_path> <generator_tool> [<designer_tool> <designer_param>]

Calculates geometry index for all mwms found inside the resource and the writable directories.
Uses generator_tool for index calculation. After all it runs designer_tool if has one.
"""

import os
import subprocess
import sys
from Queue import Queue, Empty
from threading import Thread

WORKERS = 8

exclude_names = ("WorldCoasts.mwm", "WorldCoasts_migrate.mwm")

def FindAllMwms(data_path):
    result = []
    for file in os.listdir(data_path):
        new_path = os.path.join(data_path, file)
        if os.path.isdir(new_path):
            result.extend(FindAllMwms(new_path))
            continue
        if file.endswith(".mwm") and file not in exclude_names:
            result.append((file, data_path))
    return result

def ProcessMwm(generator_tool, task, error_queue):
    print "Processing ", task[0]
    try:
        subprocess.call((generator_tool, '--data_path={0}'.format(task[1]), '--output={0}'.format(task[0][:-4]), "--generate_index=true", "--intermediate_data_path=/tmp/"))
    except subprocess.CalledProcessError as e:
        error_queue.put(str(error_queue))

def parallel_worker(tasks, generator_tool, error_queue):
    while True:
        try:
            task = tasks.get_nowait()
        except Empty:
            print "Process done!"
            return
        ProcessMwm(generator_tool, task, error_queue)
        tasks.task_done()

if __name__ == "__main__":

    if len(sys.argv) < 3:
        print "{0} <resources_dir> <writable_dir> <generator_tool> [<designer_tool> <designer_params>]".format(sys.argv[0])
        exit(1)

    mwms = FindAllMwms(sys.argv[1])
    if sys.argv[2] != sys.argv[1]:
        mwms.extend(FindAllMwms(sys.argv[2]))
    tasks = Queue()
    error_queue = Queue()
    for task in mwms:
        tasks.put(task)

    for i in range(WORKERS):
        t=Thread(target=parallel_worker, args=(tasks, sys.argv[3], error_queue))
        t.daemon = True
        t.start()

    tasks.join()
    print "Processing done."

    if len(sys.argv) > 4:
        print "Starting app"
        subprocess.Popen(sys.argv[4:])

    if not error_queue.qsize() == 0:
        while error_queue.qsize():
            error = error_queue.get()
            print error
        exit(1)

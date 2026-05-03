#!/usr/bin/env python3
"""
recalculate_geom_index.py <resources_dir> <writable_dir> <generator_tool> [<designer_tool> <designer_param>...]

Recomputes the geometry index for every .mwm found under <resources_dir> and
<writable_dir>, using <generator_tool>.  When extra arguments are provided
they are launched as a child process once index generation completes
(typically the Designer .app, so it reopens with fresh indices).
"""

import os
import subprocess
import sys
from queue import Queue, Empty
from threading import Thread

WORKERS = 8

EXCLUDE_NAMES = ("WorldCoasts.mwm", "WorldCoasts_migrate.mwm")


def find_all_mwms(data_path):
    result = []
    for entry in os.listdir(data_path):
        new_path = os.path.join(data_path, entry)
        if os.path.isdir(new_path):
            result.extend(find_all_mwms(new_path))
            continue
        if entry.endswith(".mwm") and entry not in EXCLUDE_NAMES:
            result.append((entry, data_path))
    return result


def process_mwm(generator_tool, task, error_queue):
    name, data_path = task
    print(f"Processing {name}")
    try:
        subprocess.run(
            (
                generator_tool,
                f"--data_path={data_path}",
                f"--output={name[:-4]}",
                "--generate_index=true",
                "--intermediate_data_path=/tmp/",
            ),
            check=True,
        )
    except subprocess.CalledProcessError as e:
        error_queue.put(str(e))


def parallel_worker(tasks, generator_tool, error_queue):
    while True:
        try:
            task = tasks.get_nowait()
        except Empty:
            return
        process_mwm(generator_tool, task, error_queue)
        tasks.task_done()


def main():
    if len(sys.argv) < 4:
        print(
            f"{sys.argv[0]} <resources_dir> <writable_dir> <generator_tool> "
            "[<designer_tool> <designer_params>...]"
        )
        sys.exit(1)

    resources_dir, writable_dir, generator_tool = sys.argv[1], sys.argv[2], sys.argv[3]

    mwms = find_all_mwms(resources_dir)
    if writable_dir != resources_dir:
        mwms.extend(find_all_mwms(writable_dir))

    tasks = Queue()
    error_queue = Queue()
    for task in mwms:
        tasks.put(task)

    for _ in range(WORKERS):
        t = Thread(target=parallel_worker, args=(tasks, generator_tool, error_queue))
        t.daemon = True
        t.start()

    tasks.join()
    print("Processing done.")

    if len(sys.argv) > 4:
        print("Starting app")
        subprocess.Popen(sys.argv[4:])

    if error_queue.qsize() != 0:
        while error_queue.qsize():
            print(error_queue.get())
        sys.exit(1)


if __name__ == "__main__":
    main()

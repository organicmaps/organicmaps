import os
import sys


def total_virtual_memory():
    if not sys.platform.startswith("linux"):
        return 0

    memory = os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
    for path in ("/sys/fs/cgroup/memory.max", "/sys/fs/cgroup/memory/memory.limit_in_bytes"):
        try:
            with open(path, "rb") as file:
                limit = file.read().strip()
        except OSError:
            continue
        return memory if limit == b"max" else min(memory, int(limit))
    return memory

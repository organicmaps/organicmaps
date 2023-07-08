import os
import sys


def total_virtual_memory():
    if sys.platform.startswith("linux"):
        return os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
    else:
        return 0

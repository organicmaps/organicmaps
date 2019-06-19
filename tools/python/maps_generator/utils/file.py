import errno
import functools
import glob
import logging
import os
import shutil
import urllib.request

from .md5 import md5, check_md5

logger = logging.getLogger("maps_generator")


def is_executable(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)


@functools.lru_cache()
def find_executable(path, exe=None):
    if exe is None:
        if is_executable(path):
            return path
        else:
            raise FileNotFoundError(path)
    find_pattern = f"{path}/**/{exe}"
    for name in glob.iglob(find_pattern, recursive=True):
        if is_executable(name):
            return name
    raise FileNotFoundError(f"{exe} not found in {path}")


def download_file(url, name):
    logger.info(f"Trying to download {name} from {url}.")
    urllib.request.urlretrieve(url, name)
    logger.info(f"File {name} was downloaded from {url}.")


def is_exists_file_and_md5(name):
    return os.path.isfile(name) and os.path.isfile(md5(name))


def is_verified(name):
    return is_exists_file_and_md5(name) and check_md5(name, md5(name))


def copy_overwrite(from_path, to_path):
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)


def symlink_force(target, link_name):
    try:
        os.symlink(target, link_name)
    except OSError as e:
        if e.errno == errno.EEXIST:
            os.remove(link_name)
            os.symlink(target, link_name)
        else:
            raise e

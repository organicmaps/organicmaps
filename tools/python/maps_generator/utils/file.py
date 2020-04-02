import errno
import functools
import glob
import logging
import os
import shutil
import urllib.request
from typing import AnyStr
from typing import Dict
from typing import Optional

from maps_generator.utils.md5 import check_md5
from maps_generator.utils.md5 import md5

logger = logging.getLogger("maps_generator")


def is_executable(fpath: AnyStr) -> bool:
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)


@functools.lru_cache()
def find_executable(path: AnyStr, exe: Optional[AnyStr] = None) -> AnyStr:
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


def download_file(url: AnyStr, name: AnyStr):
    logger.info(f"Trying to download {name} from {url}.")
    urllib.request.urlretrieve(url, name)
    logger.info(f"File {name} was downloaded from {url}.")


def download_files(url_to_path: Dict[AnyStr, AnyStr]):
    for k, v in url_to_path.items():
        download_file(k, v)


def is_exists_file_and_md5(name: AnyStr) -> bool:
    return os.path.isfile(name) and os.path.isfile(md5(name))


def is_verified(name: AnyStr) -> bool:
    return is_exists_file_and_md5(name) and check_md5(name, md5(name))


def copy_overwrite(from_path: AnyStr, to_path: AnyStr):
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)


def symlink_force(target: AnyStr, link_name: AnyStr):
    try:
        os.symlink(target, link_name)
    except OSError as e:
        if e.errno == errno.EEXIST:
            os.remove(link_name)
            os.symlink(target, link_name)
        else:
            raise e

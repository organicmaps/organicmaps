import errno
import functools
import glob
import logging
import os
import shutil
from functools import partial
from multiprocessing.pool import ThreadPool
from typing import AnyStr
from typing import Dict
from typing import List
from typing import Optional
from urllib.parse import unquote
from urllib.parse import urljoin
from urllib.parse import urlparse
from urllib.request import url2pathname

import requests
from bs4 import BeautifulSoup
from requests_file import FileAdapter

from maps_generator.utils.md5 import check_md5
from maps_generator.utils.md5 import md5_ext

logger = logging.getLogger("maps_generator")


def is_file_uri(url: AnyStr) -> bool:
    return urlparse(url).scheme == "file"

def file_uri_to_path(url : AnyStr) -> AnyStr:
    file_uri = urlparse(url)
    file_path = file_uri.path

    # URI is something like "file://~/..."
    if file_uri.netloc == '~':
        file_path = f'~{file_uri.path}'
        return os.path.expanduser(file_path)
    
    return file_path

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


def download_file(url: AnyStr, name: AnyStr, download_if_exists: bool = True):
    logger.info(f"Trying to download {name} from {url}.")
    if not download_if_exists and os.path.exists(name):
        logger.info(f"File {name} already exists.")
        return

    if is_file_uri(url):
        # url uses 'file://' scheme
        copy_overwrite(file_uri_to_path(url), name)
        logger.info(f"File {name} was copied from {url}.")
        return

    tmp_name = f"{name}__"
    os.makedirs(os.path.dirname(tmp_name), exist_ok=True)
    with requests.Session() as session:
        session.mount("file://", FileAdapter())
        with open(tmp_name, "wb") as handle:
            response = session.get(url, stream=True)
            file_length = None
            try:
                file_length = int(response.headers["Content-Length"])
            except KeyError:
                logger.warning(
                    f"There is no attribute Content-Length in headers [{url}]: {response.headers}"
                )

            current = 0
            max_attempts = 32
            attempts = max_attempts
            while attempts:
                for data in response.iter_content(chunk_size=4096):
                    current += len(data)
                    handle.write(data)

                if file_length is None or file_length == current:
                    break

                logger.warning(
                    f"Download interrupted. Resuming download from {url}: {current}/{file_length}."
                )
                headers = {"Range": f"bytes={current}-"}
                response = session.get(url, headers=headers, stream=True)
                attempts -= 1

            assert (
                attempts > 0
            ), f"Maximum failed resuming download attempts of {max_attempts} is exceeded."

    shutil.move(tmp_name, name)
    logger.info(f"File {name} was downloaded from {url}.")


def is_dir(url) -> bool:
    return url.endswith("/")


def find_files(url) -> List[AnyStr]:
    def files_list_file_scheme(path, results=None):
        if results is None:
            results = []

        for p in os.listdir(path):
            new_path = os.path.join(path, p)
            if os.path.isdir(new_path):
                files_list_file_scheme(new_path, results)
            else:
                results.append(new_path)
        return results

    def files_list_http_scheme(url, results=None):
        if results is None:
            results = []

        page = requests.get(url).content
        bs = BeautifulSoup(page, "html.parser")
        links = bs.findAll("a", href=True)
        for link in links:
            href = link["href"]
            if href == "./" or href == "../":
                continue

            new_url = urljoin(url, href)
            if is_dir(new_url):
                files_list_http_scheme(new_url, results)
            else:
                results.append(new_url)
        return results

    parse_result = urlparse(url)
    if parse_result.scheme == "file":
        return [
            f.replace(parse_result.path, "")
            for f in files_list_file_scheme(parse_result.path)
        ]
    if parse_result.scheme == "http" or parse_result.scheme == "https":
        return [f.replace(url, "") for f in files_list_http_scheme(url)]

    assert False, parse_result


def normalize_url_to_path_dict(
    url_to_path: Dict[AnyStr, AnyStr]
) -> Dict[AnyStr, AnyStr]:
    for url in list(url_to_path.keys()):
        if is_dir(url):
            path = url_to_path[url]
            del url_to_path[url]
            for rel_path in find_files(url):
                abs_url = urljoin(url, rel_path)
                url_to_path[abs_url] = unquote(os.path.join(path, rel_path))
    return url_to_path


def download_files(url_to_path: Dict[AnyStr, AnyStr], download_if_exists: bool = True):
    with ThreadPool() as pool:
        pool.starmap(
            partial(download_file, download_if_exists=download_if_exists),
            url_to_path.items(),
        )


def is_exists_file_and_md5(name: AnyStr) -> bool:
    return os.path.isfile(name) and os.path.isfile(md5_ext(name))


def is_verified(name: AnyStr) -> bool:
    return is_exists_file_and_md5(name) and check_md5(name, md5_ext(name))


def copy_overwrite(from_path: AnyStr, to_path: AnyStr):
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)


def make_symlink(target: AnyStr, link_name: AnyStr):
    try:
        os.symlink(target, link_name)
    except OSError as e:
        if e.errno == errno.EEXIST:
            if os.path.islink(link_name):
                link = os.readlink(link_name)
                if os.path.abspath(target) != os.path.abspath(link):
                    raise e
            else:
                raise e
        else:
            raise e

#!/usr/bin/env python3

import argparse
import os
import re
import time
import sys
import urllib.request
import urllib.parse
import urllib.error
import logging
from multiprocessing.pool import ThreadPool


DIRECT_MAP_URL = 'http://direct.mapswithme.com/direct/'
ROOT = os.path.dirname(os.path.realpath(__file__))
DEFAULT_DOWNLOAD_DIRECTORY = os.path.join(os.path.dirname(os.path.dirname(ROOT)), 'data')
MAP_REVISION_REGEXP = re.compile(r'(\d{6})')
MWM_NAME_REGEXP = re.compile(r'"(\S+\.mwm)"')

logger = logging.getLogger(__name__)


def parse_html_by_regexp(url, regexp):
    def parse_line(line):
        return regexp.search(line.decode('utf-8'))

    with urllib.request.urlopen(url) as http_response_lines:
        filtered_lines = filter(None, map(parse_line, http_response_lines))
        for line in filtered_lines:
            yield line.group(1)


def get_mwm_versions():
    return map(int, parse_html_by_regexp(DIRECT_MAP_URL, MAP_REVISION_REGEXP))


def get_mwm_names(version_num):
    version_url = '{base}{version}'.format(base=DIRECT_MAP_URL, version=version_num)
    return map(urllib.parse.unquote, parse_html_by_regexp(version_url, MWM_NAME_REGEXP))


def download_file(arg_tuple, attempts=3):
    url, filename = arg_tuple
    try:
        urllib.request.urlretrieve(url, filename=filename)
    except urllib.error.HTTPError as err:
        if 400 <= err.code < 500:
            logger.error('URL \'{url}\' is not found'.format(url=url), exc_info=True)
            return False
    except PermissionError:
        logger.error('Can\'t write file {filename}: permission denied'.format(filename=filename), exc_info=True)
        return False
    except urllib.error.URLError:
        if attempts:
            time.sleep(3)
            return download_file(arg_tuple, attempts - 1)
        logger.error('File {filename} is not loaded'.format(filename=filename), exc_info=True)
        return False
    return True


def progress_bar(total, progress):
    """
    Displays or updates a console progress bar.

    Original source: https://stackoverflow.com/a/15860757/1391441
    """
    initial_progress = progress
    bar_length = 20
    progress = progress / total
    block = int(round(bar_length * progress))
    progress_bar_str = '#' * block + '-' * (bar_length - block)
    text = '\r[{progress_bar}] {percent:.0f}% {item}/{total}'.format(
        progress_bar=progress_bar_str,
        percent=round(progress * 100, 0),
        item=initial_progress,
        total=total)
    sys.stderr.write(text)
    if progress == 1:
        sys.stderr.write('\n')
    sys.stderr.flush()


def download_mwm_list(version=None, threads=8, folder=None, mwm_prefix_list=None, quiet=True):
    versions = list(get_mwm_versions())

    if version is None:
        # Get latest version by default
        version = max(versions)
    elif version not in versions:
        logger.error('Maps with version {version} is not found in {url}'.format(version=version, url=DIRECT_MAP_URL))
        return

    mwm_names = get_mwm_names(version)
    if mwm_prefix_list is not None:
        mwm_regexp = re.compile('|'.join(mwm_prefix_list))
        mwm_to_download = filter(lambda m: mwm_regexp.match(m), mwm_names)
    else:
        mwm_to_download = mwm_names

    if folder is None:
        folder = '{base}/{version}'.format(base=DEFAULT_DOWNLOAD_DIRECTORY, version=version)

    os.makedirs(folder, exist_ok=True)

    def generate_args(mwm_name):
        unescaped_name = urllib.parse.quote(mwm_name)
        url = '{base}{version}/{mwm_name}'.format(base=DIRECT_MAP_URL, version=version, mwm_name=unescaped_name)
        filename = '{folder}/{mwm_name}'.format(folder=folder, mwm_name=mwm_name)
        return url, filename

    mwm_args = list(map(generate_args, mwm_to_download))

    # No sense to show progress bar for one map
    if len(mwm_args) < 2:
        quiet = True

    pool = ThreadPool(processes=threads)
    for num, _ in enumerate(pool.imap_unordered(download_file, mwm_args)):
        if not quiet:
            progress_bar(len(mwm_args), num + 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Script to download \'.mwm\' files in multiple threads')
    parser.add_argument('-v', '--version', help='Map version number e.g. 180126', type=int, default=None)
    parser.add_argument('-t', '--threads', help='Threads count', type=int, default=8)
    parser.add_argument('-f', '--folder', help='Directory to save maps', type=str, default=None)
    parser.add_argument('-m', '--mwm_prefix_list', help='Mwm prefix list (exmp. \'Russia\', \'Russia_Moscow.mwm\')',
                        type=str, default=None, nargs='+')
    parser.add_argument('-q', '--quiet', help='Prevent progress bar showing', action='store_true', default=False)

    args = parser.parse_args()

    if not args.quiet:
        start_time = time.time()
        print('Start downloading maps with {threads} threads'.format(threads=args.threads))

    download_mwm_list(**vars(args))

    if not args.quiet:
        end_time = time.time()
        print('Finished in {} s.'.format(round(end_time - start_time)))

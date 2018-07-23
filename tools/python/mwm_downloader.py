#!/usr/bin/env python3

import argparse
import itertools
import json
import logging
import os
import re
import random
import socket
import sys
import time
import urllib.request
import urllib.parse
import urllib.error

from multiprocessing.pool import ThreadPool
from pathlib import Path


ROOT = Path(__file__).parent.absolute()
OMIM_ROOT = ROOT / '..' / '..'
DEFAULT_DOWNLOAD_DIRECTORY = OMIM_ROOT / 'data'
COUNTRIES_TXT = OMIM_ROOT / 'data' / 'countries.txt'
TIMEOUT = 3

URL_PATTERN = 'http://{prefix}.mapswithme.com/direct/{version}/{name}.mwm'
MAP_SERVERS = ('maps-dl-ru1', 'maps-dl-ru2', 'maps-dl-ru3', 'maps-dl-ams1',
               'maps-dl-eu2', 'maps-dl-us1')
MWM_NAME_REGEXP = re.compile(r'"(\S+\.mwm)"')

logger = logging.getLogger(__name__)


def country_names_generator(country_obj):
    """
    See more info about 'countries.txt' file format in 'omim/storage/country.cpp'.
    """
    if 'g' in country_obj:
        yield from country_names_generator(country_obj['g'])
    elif 's' in country_obj:
        yield country_obj['id']
    elif type(country_obj) is list:
        for country in country_obj:
            yield from country_names_generator(country)


def is_redirected(url):
    response = urllib.request.urlopen(url)
    return response.url != url


def download_file(url, filename, network_attempts=3):
    try:
        if is_redirected(url):
            logging.error('Maps server not found in \'{url}\''.format(url=url))
            return False

        with urllib.request.urlopen(url, timeout=TIMEOUT) as response:
            content = response.read()
            with open(filename, 'wb') as f:
                f.write(content)

    except urllib.error.HTTPError as err:
        if 400 <= err.code < 500:
            logger.error('URL \'{url}\' is not found'.format(url=url))
            return False
    except (PermissionError, FileNotFoundError) as e:
        logger.error('Can\'t write file {filename}: {error}'.format(filename=filename, error=e))
        return False
    except (urllib.error.URLError, socket.timeout):
        if network_attempts > 0:
            time.sleep(TIMEOUT)
            return download_file(url, filename, network_attempts - 1)
        logger.error('File {filename} is not loaded'.format(filename=filename))
        return False

    return True


def download_map(arg_tuple):
    version, mwm_name, filename = arg_tuple
    mwm_name = urllib.parse.quote(mwm_name)
    random_prefix_generator = (random.choice(MAP_SERVERS) for _ in range(0, len(MAP_SERVERS)))

    for server_prefix in random_prefix_generator:
        url = URL_PATTERN.format(prefix=server_prefix, version=version, name=mwm_name)
        if download_file(url, filename):
            return True
        else:
            logging.error('Failed to load mwm \'{}\' from server \'{}\''.format(mwm_name, server_prefix))
    return False


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
    try:
        countries = json.load(open(str(COUNTRIES_TXT), 'r'))
    except (OSError, json.decoder.JSONDecodeError) as e:
        logging.error('File \'omim/data/countries.txt\' is corrupted.', exc_info=True)
        exit(1)

    mwm_names = country_names_generator(countries)

    if mwm_prefix_list is not None:
        mwm_regexp = re.compile('|'.join(mwm_prefix_list))
        mwm_to_download = filter(lambda m: mwm_regexp.match(m), mwm_names)
    else:
        mwm_to_download = mwm_names

    if version is None:
        version = countries['v']

    if folder is None:
        folder = '{base}/{version}'.format(base=DEFAULT_DOWNLOAD_DIRECTORY, version=version)

    os.makedirs(folder, exist_ok=True)

    def filename(mwm_name):
        return '{folder}/{mwm_name}.mwm'.format(folder=folder, mwm_name=mwm_name)

    mwm_args = [(version, mwm_name, filename(mwm_name)) for mwm_name in mwm_to_download]

    # No sense to show progress bar for one map
    if len(mwm_args) < 2:
        quiet = True

    pool = ThreadPool(processes=threads)
    for num, _ in enumerate(pool.imap_unordered(download_map, mwm_args)):
        if not quiet:
            progress_bar(len(mwm_args), num + 1)


if __name__ == '__main__':
    if sys.version_info < (3, 4):
        raise RuntimeError('This script requires Python 3.4+')

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

#!/usr/bin/env python2.7

from __future__ import print_function

import os
import sys

# TODO(mgsergio, zveric, yershov): Make mwm an installable module.
sys.path.append(
    os.path.join(
        os.path.dirname(__file__), '..', 'mwm'
    )
)

import argparse
import csv
# c_long is used to get signed int64. Postgres can't handle uint64.
import ctypes
import logging
import mwm

from itertools import islice
from zlib import adler32


def get_args():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        '--mapping_names',
        nargs='+',
        help='osm2ft files to handle.'
    )
    group.add_argument(
        '--mapping_path',
        nargs=1,
        action=AppendOsm2FidAction,
        dest='mapping_names',
        help='Path to folder with .osm2ft. Each file whould be handled.'
    )

    parser.add_argument(
        '--version',
        required=True,
        type=int,
        help='The version of mwm for which a mapping is generated.'
    )
    parser.add_argument(
        '--head',
        type=int,
        help='Write that much lines of osmid <-> fid to stdout.'
    )

    return parser.parse_args();


def get_mapping(mapping_name):
    with open(mapping_name, 'rb') as f:
        osm2ft = mwm.read_osm2ft(f, tuples=False)

    for osmid, fid in osm2ft.iteritems():
        yield ctypes.c_long(osmid).value, fid


def print_mapping(mapping, count):
    for osmid, fid in islice(mapping, count):
        print('{}\t{}'.format(osmid, fid))

def generate_id_from_name_and_version(name, version):
    return ctypes.c_long((adler32(name) << 32) | version).value


def generate_csvs(mapping, mapping_name, version):
    mwm_id = generate_id_from_name_and_version(
        mapping_name,
        version
    )

    with open('mwm.csv', 'wb') as f:
        w = csv.writer(f)
        w.writerow(['id', 'name', 'version'])
        w.writerow([
            mwm_id,
            mapping_name,
            version,
        ])
    with open('mapping.csv', 'wb') as f:
        w = csv.writer(f)
        w.writerow(['osmid', 'fid', 'mwm_id'])
        for row in mapping:
            w.writerow(row + (mwm_id, ))


def main():
    args = get_args()
    for mapping_name in args.mapping_names:
        mapping = get_mapping(mapping_name)
        if args.head:
            print('{}:'.format(mapping_name))
            print_mapping(mapping, args.head)
            exit(0)
        mwm_name = (
            os.path.basename(mapping_name)
            .split('.', 1)
        )[0]
        generate_csvs(
            mapping,
            mwm_name,
            args.version
        )


class AppendOsm2FidAction(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        assert nargs == 1, 'nargs should equals to 1.'
        super(AppendOsm2FidAction, self).__init__(
            option_strings,
            dest,
            nargs=1,
            **kwargs
        )

    def __call__(self, parser, namespace, values, option_string=None):
        values = [
            os.path.join(values[0], mapping_name)
            for mapping_name in os.listdir(values[0])
            if mapping_name.endswith('.osm2ft')
        ]
        setattr(namespace, self.dest, values)


if __name__ == '__main__':
    main()

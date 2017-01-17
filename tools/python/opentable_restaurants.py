#! /usr/bin/env python2.7
# coding: utf-8

from __future__ import print_function

import argparse
import base64
import copy
import json
import logging
import os
import re
import sys
import urllib2

from datetime import datetime

# Initialize logging.
logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(levelname)s: %(message)s')

class OpentableDownloaderError(Exception):
    pass

class OpentableDownloader(object):
    def __init__(self, login, password, opentable_filename, tsv_filename=None):
        self.login = login
        self.password = password
        self.token = None
        self.opentable_filename  = opentable_filename
        self.tsv_filename = tsv_filename

        # TODO(mgsergio): Check if token is actual in functions.
        self._get_token()

    def download(self):
        headers = self._add_auth_header({'Content-Type': 'application/json'})
        url = 'https://platform.opentable.com/sync/listings'

        with open(self.opentable_filename, 'w') as f:
            offset = 0
            while True:
                request = urllib2.Request(url + '?offset={}'.format(offset), headers=headers)
                logging.debug('Fetching data with headers %s from %s',
                              str(headers), request.get_full_url())
                resp = urllib2.urlopen(request)
                # TODO(mgsergio): Handle exceptions
                data = json.loads(resp.read())
                for rest in data['items']:
                    print(json.dumps(rest), file=f)

                total_items = int(data['total_items'])
                offset = int(data['offset'])
                items_count = len(data['items'])

                if total_items <= offset + items_count:
                    break

                offset += items_count

    def _get_token(self):
        url = 'https://oauth.opentable.com/api/v2/oauth/token?grant_type=client_credentials'
        headers = self._add_auth_header({})
        request = urllib2.Request(url, headers=headers)
        logging.debug('Fetching token with headers %s', str(headers))
        resp = urllib2.urlopen(request)
        # TODO(mgsergio): Handle exceptions
        if resp.getcode() != 200:
            raise OpentableDownloaderError("Cant't get token. Response: {}".format(resp.read()))
        self.token = json.loads(resp.read())
        logging.debug('Token is %s', self.token)

    def _add_auth_header(self, headers):
        if self.token is None:
            key = base64.b64encode('{}:{}'.format(self.login, self.password))
            headers['Authorization'] = 'Basic {}'.format(key)
        else:
            headers['Authorization'] = '{} {}'.format(self.token['token_type'],
                                                      self.token['access_token'])
        return headers


def make_tsv(data_file, output_file):
    for rest in data_file:
        rest = json.loads(rest)
        try:
            address = ' '.join([rest['address'], rest['city'], rest['country']])
            # Some addresses contain \t and maybe other spaces.
            address = re.sub(r'\s', ' ', address)
        except TypeError:
            address = ''
        row = '\t'.join(map(unicode, [rest['rid'], rest['latitude'], rest['longitude'],
                                      rest['name'], address, rest['reservation_url'],
                                      rest['phone_number']]))
        print(row.encode('utf-8'), file=output_file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Downloads opentable data.')
    parser.add_argument('-d', '--download', action='store_true', help='Download data')
    parser.add_argument('--tsv', type=str, nargs='?', const='',
                        help='A file to put data into, stdout if value is empty '
                        'If ommited, no tsv data is generated')
    parser.add_argument('--opentable_data', type=str, help='Path to opentable data file')

    # TODO(mgsergio): Allow config instead.
    parser.add_argument('--client', required=True, help='Opentable client id')
    parser.add_argument('--secret', required=True, help="Opentable client's secret")

    args = parser.parse_args(sys.argv[1:])

    if args.download:
        logging.info('Downloading')
        loader = OpentableDownloader(args.client, args.secret, args.opentable_data)
        loader.download()
    if args.tsv is not None:
        data = open(args.opentable_data)
        tsv = open(args.tsv, 'w') if args.tsv else sys.stdout
        make_tsv(data, tsv)

from __future__ import print_function

import argparse
import json
import logging
import os
import urllib2

logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(levelname)s: %(message)s')


class ViatorApi(object):
    def __init__(self, apikey):
        self.apikey = apikey

    def get_locations(self):
        url = 'https://viatorapi.viator.com/service/taxonomy/locations?apiKey=' + self.apikey
        stream = urllib2.urlopen(url)
        return json.load(stream)


def check_errors(locations):
    if not locations['success']:
        raise Exception('Viator error, error codes:{} error text:{}'
                        .format(locations['errorCodes'], locations['errorMessageText']))


def save_cities(locations, output_file_name):
    with open(output_file_name, 'w') as output_file:
        for l in locations['data']:
            is_object_supported = (l['destinationType'] == 'CITY' and l['destinationId'] and
                                   l['destinationName'] and l['latitude'] and l['longitude'])
            if is_object_supported:
                city = '\t'.join([
                    str(l['destinationId']),
                    l['destinationName'],
                    str(l['latitude']),
                    str(l['longitude'])
                ])
                print(city.encode('utf-8'), file=output_file)


def run(options):
    try:
        api = ViatorApi(options.apikey)
        locations = api.get_locations()
        check_errors(locations)
        save_cities(locations, options.output)
    except Exception as e:
        logging.exception(e)


def process_options():
    parser = argparse.ArgumentParser(description='Download and process viator cities.')

    parser.add_argument('--apikey', dest='apikey', help='Viator apikey', required=True)
    parser.add_argument('--output', dest='output', help='Destination file', required=True)

    options = parser.parse_args()

    return options


def main():
    options = process_options()
    run(options)


if __name__ == '__main__':
    main()

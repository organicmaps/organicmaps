#!/usr/bin/env python
# coding: utf8
from __future__ import print_function

from collections import defaultdict
from datetime import datetime
import argparse
import base64
import eviltransform
import json
import logging
import os
import pickle
import time
import urllib2

# Initialize logging.
logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(levelname)s: %(message)s')

# Names starting with '.' are calculated in get_hotel_field() below.
HOTEL_FIELDS = ('hotel_id', '.lat', '.lon', 'name', 'address', 'class', '.rate', 'ranking', 'review_score', 'url', 'hoteltype_id', '.trans')


class BookingApi:
    def __init__(self, login, password):
        self.login = login
        self.password = password
        self.baseConfig = {
            "headers": {
                "Content-Type": "application/json",
                "Authorization": "Basic " + base64.encodestring(
                    "{login}:{password}".format(login=self.login, password=self.password)).replace('\n', '')
            },
            "url": 'https://distribution-xml.booking.com/json/bookings'}
        self.checkMinute = 0
        self.requestPerMinute = 0
        self.requestLimit = 15  # request per minute

    def call(self, function, params=None):
        self.requestPerMinute += 1
        now = datetime.utcnow()

        if self.requestPerMinute >= self.requestLimit:
            waittime = 60 - now.second
            logging.info("Limit for request per minute exceeded. Waiting for: {0} sec.".format(waittime))
            time.sleep(waittime)
            now = datetime.utcnow()

        if self.checkMinute != now.minute:
            self.requestPerMinute = 0
            self.checkMinute = now.minute

        payload = ''
        try:
            p = "" if not params else '?' + "&".join(
                ["{key}={value}".format(key=k, value=v) for (k, v) in params.iteritems()])
            url = "{base}.{func}{params}".format(base=self.baseConfig["url"], func=function, params=p)
            logging.debug("{0} {1} API call:{2}".format(self.checkMinute, self.requestPerMinute, url))
            request = urllib2.Request(url, None, self.baseConfig["headers"])
            stream = urllib2.urlopen(request)
            payload = stream.read()
            data = json.loads(payload)
            if isinstance(data, dict) and 'message' in data and 'code' in data:
                logging.error('Api call failed with error: {0} Code: {1}'.format(data['message'], data['code']))
                return None
            return data

        except Exception as e:
            logging.error('Error: {0} Context: {1}'.format(e, payload))
            return None


def download(user, password, path):
    '''
    Downloads all hotels from booking.com and stores them in a bunch of .pkl files.
    '''
    api = BookingApi(user, password)

    maxrows = 1000
    countries = api.call("getCountries", dict(languagecodes='en'))
    for country in countries:
        countrycode = country['countrycode']
        logging.info(u'Download[{0}]: {1}'.format(countrycode, country['name']))

        allhotels = {}
        while True:
            hotels = api.call('getHotels',
                              dict(new_hotel_type=1, offset=len(allhotels), rows=maxrows, countrycodes=countrycode))

            # Check for error.
            if hotels is None:
                logging.critical('No hotels downloaded for country {0}'.format(country['name']))
                break

            for h in hotels:
                allhotels[h['hotel_id']] = h

            # If hotels in answer less then maxrows, we reach end of data.
            if len(hotels) < maxrows:
                break

        if not hotels:
            continue

        # Now the same for hotel translations
        offset = 0
        while True:
            hotels = api.call('getHotelTranslations', dict(offset=offset, rows=maxrows, countrycodes=countrycode))
            if hotels is None:
                exit(1)

            # Add translations for each hotel
            for h in hotels:
                if h['hotel_id'] in allhotels:
                    if 'translations' not in allhotels[h['hotel_id']]:
                        allhotels[h['hotel_id']]['translations'] = {}
                    allhotels[h['hotel_id']]['translations'][h['languagecode']] = {'name': h['name'], 'address': h['address']}

            offset += len(hotels)
            if len(hotels) < maxrows:
                break

        logging.info('Num of hotels: {0}, translations: {1}'.format(len(allhotels), offset))
        filename = os.path.join(path,
                                '{0} - {1}.pkl'.format(country['area'].encode('utf8'), country['name'].encode('utf8')))
        with open(filename, 'wb') as fd:
            pickle.dump(allhotels.values(), fd, pickle.HIGHEST_PROTOCOL)


def translate(source, output):
    '''
    Reads *.pkl files and produces a single list of hotels as tab separated values.
    '''
    files = [os.path.join(source, filename)
             for filename in os.listdir(source) if filename.endswith('.pkl')]

    data = []
    for filename in sorted(files):
        logging.info('Processing {0}'.format(filename))
        with open(filename, 'rb') as fd:
            data += pickle.load(fd)

    # Fix chinese coordinates
    for hotel in data:
        if hotel['countrycode'] == 'cn' and 'location' in hotel:
            try:
                hotel['location']['latitude'], hotel['location']['longitude'] = eviltransform.gcj2wgs_exact(
                    float(hotel['location']['latitude']), float(hotel['location']['longitude']))
            except ValueError:
                # We don't care if there were errors converting coordinates to float
                pass

    # Dict of dicts city_id -> { currency -> [prices] }
    cities = defaultdict(lambda: defaultdict(list))

    def valid(hotel):
        return 'city_id' in hotel and 'currencycode' in hotel and 'minrate' in hotel and hotel['minrate'] is not None

    # Collect prices
    for hotel in data:
        if valid(hotel):
            cities[hotel['city_id']][hotel['currencycode']].append(float(hotel['minrate']))

    # Replaces list of prices by a median price.
    for city in cities:
        for cur in cities[city]:
            cities[city][cur] = sorted(cities[city][cur])[len(cities[city][cur]) / 2]

    # Price rate ranges, relative to the median price for a city
    rates = (0.7, 1.3)

    def get_hotel_field(hotel, field, rate):
        if field == '.lat':
            return hotel['location']['latitude']
        elif field == '.lon':
            return hotel['location']['longitude']
        elif field == '.rate':
            return rate
        elif field == '.trans':
            # Translations are packed into a single column: lang1|name1|address1|lang2|name2|address2|...
            if 'translations' in hotel:
                tr_list = []
                for tr_lang, tr_values in hotel['translations'].items():
                    tr_list.append(tr_lang)
                    tr_list.extend([tr_values[e] for e in ('name', 'address')])
                return '|'.join([s.replace('|', ';') for s in tr_list])
            else:
                return ''
        elif field in hotel:
            return hotel[field]
        elif field == 'ranking':
            # This field is not used yet, and booking.com sometimes blocks it.
            return ''
        logging.error('Unknown hotel field: {0}, URL: {1}'.format(field, hotel['url']))
        return ''

    with open(output, 'w') as fd:
        for hotel in data:
            rate = 0
            if valid(hotel):
                avg = cities[hotel['city_id']][hotel['currencycode']]
                price = float(hotel['minrate'])
                rate = 1
                # Find a range that contains the price
                while rate <= len(rates) and price > avg * rates[rate - 1]:
                    rate += 1
            l = [get_hotel_field(hotel, e, rate) for e in HOTEL_FIELDS]
            print('\t'.join([unicode(f).encode('utf8').replace('\t', ' ').replace('\n', ' ').replace('\r', '') for f in l]), file=fd)


def process_options():
    parser = argparse.ArgumentParser(description='Download and process booking hotels.')
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose")
    parser.add_argument("-q", "--quiet", action="store_false", dest="verbose")

    parser.add_argument("--password", dest="password", help="Booking.com account password")
    parser.add_argument("--user", dest="user", help="Booking.com account user name")

    parser.add_argument("--path", dest="path", help="Path to data files")
    parser.add_argument("--output", dest="output", help="Name and destination for output file")

    parser.add_argument("--download", action="store_true", dest="download", default=False)
    parser.add_argument("--translate", action="store_true", dest="translate", default=False)

    options = parser.parse_args()

    if not options.download and not options.translate:
        parser.print_help()

    # TODO(mgsergio): implpement it with argparse facilities.
    if options.translate and not options.output:
        print("--output isn't set")
        parser.print_help()
        exit()

    return options


def main():
    options = process_options()
    if options.download:
        download(options.user, options.password, options.path)
    if options.translate:
        translate(options.path, options.output)


if __name__ == "__main__":
    main()

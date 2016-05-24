#!/usr/bin/python
# coding: utf8
from __future__ import print_function

import json
import urllib2
import base64
from datetime import datetime
import time
import logging
import pickle
import os
import argparse
from collections import namedtuple, defaultdict

# init logging
logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(levelname)s: %(message)s')

Hotel = namedtuple('Hotel',
                   ['id', 'lat', 'lon', 'name', 'address',
                    'stars', 'priceCategory', 'ratingBooking',
                    'ratingUser', 'descUrl'])


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
            logging.warning("Limit for request per minute exceeded. Wait for: {0} sec.".format(waittime))
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
            return json.loads(payload)

        except Exception as e:
            logging.error('Error: {0} Context: {1}'.format(e, payload))
            return None


def make_record(src, rate):
    return Hotel(
        int(src['hotel_id']),
        float(src['location']['latitude']),
        float(src['location']['longitude']),
        src['name'],
        src['address'],
        int(src['class']),
        rate,
        src['ranking'],
        src['review_score'],
        src['url']
    )


def download(user, password, path):
    api = BookingApi(user, password)

    maxrows = 1000
    countries = api.call("getCountries", dict(languagecodes='en'))
    for country in countries:
        countrycode = country['countrycode']
        logging.info(u'{0} {1}'.format(countrycode, country['name']))

        counter = 0
        allhotels = []
        while True:
            hotels = api.call('getHotels',
                              dict(new_hotel_type=1, offset=counter, rows=maxrows, countrycodes=countrycode))
            if isinstance(hotels, dict) and 'ruid' in hotels:
                logging.error('{0} Code: {1}'.format(hotels['message'], hotels['code']))
                exit(1)

            for hotel in hotels:
                allhotels.append(hotel)

            counter += len(hotels)

            if len(hotels) < maxrows:
                break

        logging.info('Total hotels: {0}'.format(len(allhotels)))
        filename = os.path.join(path,
                                '{0} - {1}.pkl'.format(country['area'].encode('utf8'), country['name'].encode('utf8')))
        with open(filename, 'wb') as fd:
            pickle.dump(allhotels, fd, pickle.HIGHEST_PROTOCOL)


def translate(source, output):
    files = []
    data = []

    for filename in os.listdir(source):
        if filename.endswith(".pkl"):
            files.append(filename)

    for filename in files:
        logging.info('Processing {0}'.format(filename))
        with open(filename, 'rb') as fd:
            data += pickle.load(fd)

    # Dict of dicts city_id -> { currency -> [prices] }
    cities = defaultdict(lambda: defaultdict(list))

    # Collect prices
    for hotel in data:
        if 'city_id' in hotel and 'currencycode' in hotel and 'minrate' in hotel and hotel['minrate'] is not None:
            cities[hotel['city_id']][hotel['currencycode']].append(float(hotel['minrate']))

    # Find median prices
    for city in cities:
        for cur in cities[city]:
            cities[city][cur] = sorted(cities[city][cur])[len(cities[city][cur]) / 2]

    # Price rate ranges, relative to the median price for a city
    rates = (0.7, 1.3)

    with open(output, 'w') as fd:
        for hotel in data:
            rate = 0
            if 'city_id' in hotel and 'currencycode' in hotel and 'minrate' in hotel and hotel['minrate'] is not None:
                avg = cities[hotel['city_id']][hotel['currencycode']]
                price = float(hotel['minrate'])
                rate = 1
                while rate <= len(rates) and price > avg * rates[rate - 1]:
                    rate += 1
            cur = make_record(hotel, rate)
            l = [(str(e) if e else '') if not isinstance(e, unicode) else e.encode('utf8') for e in cur]
            print('\t'.join(l), file=fd)


def process_options():
    parser = argparse.ArgumentParser(description='Download and process booking hotels.')
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose")
    parser.add_argument("-q", "--quiet", action="store_false", dest="verbose")

    parser.add_argument("--password", dest="password", help="Booking.com account password")
    parser.add_argument("--user", dest="user", help="Booking.com account user name")

    parser.add_argument("--path", dest="path", help="path to data files")
    parser.add_argument("--output", dest="output", help="Name and destination for output file")

    parser.add_argument("--download", action="store_true", dest="download", default=False)
    parser.add_argument("--translate", action="store_true", dest="translate", default=False)

    options = parser.parse_args()

    if not options.download and not options.translate:
        parser.print_help()

    if options.translate and not options.output:
        print("--output isn't set")
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

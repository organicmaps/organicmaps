#!/usr/bin/env python3
"""Converts the TRIP lines of a NoGPS Maps log to CSV to analyse a drive.

The navigation core writes a TRIP line every second while the location is on: the shown position, the positions
of the platform (gps= satellites only, Android; fused= CoreLocation on iOS, Google fused on Android) and the state
of the inertial navigation. The error of the shown position is measured against the platform position, gps= if
there is one, at the same moment (the two have different ages): along the course of the car (+ the mark is ahead)
and across it (+ the mark is to the right). With "Disable GPS" on, the platform position is the car itself.

Logs:
  iOS: Documents/log.txt, written with "Enable logging" on in the settings. Times are UTC.
  Android: Android/data/<app id>/files/logs/, or the zip of "Report a bug". Times are the local time of the phone,
  the log has no year: --year.

Usage:
  nogps_trip_log.py log.txt [more logs] > trip.csv
  nogps_trip_log.py log.txt --gpx trip.gpx > trip.csv    # the shown and the platform tracks to see on a map
"""

import argparse
import csv
import math
import re
import sys
from datetime import datetime, timedelta, timezone

# "I(1) 12.34567 nogps/engine.cpp:1010 LogTrip(): TRIP ...": seconds since the launch of the app.
IOS_PREFIX = re.compile(r'\w\(\d+\) (\d+\.\d+) ')
IOS_START = re.compile(r'Local time: (\d{4}-\d\d-\d\d \d\d:\d\d:\d\d) \+0000')
# "10-08 18:51:32.123 (main) I/OM: TRIP ...".
ANDROID_PREFIX = re.compile(r'(\d\d-\d\d \d\d:\d\d:\d\d\.\d{3}) ')
# gps= and fused= are "lat,lon/accuracy/km/h/course/age".
PLATFORM_FIELDS = ('acc', 'kmh', 'bear', 'age')
EARTH_RADIUS_M = 6371000
FIRST_COLUMNS = ['time', 'src', 'lat', 'lon', 'acc', 'bear', 'age', 'ref', 'err_m', 'along_m', 'across_m']


def parse_time(line, launch, year):
    """Returns the time of a log line, None if unknown. Keeps the time of the iOS app launch in |launch|."""
    m = ANDROID_PREFIX.match(line)
    if m:
        return datetime.strptime(f'{year}-{m.group(1)}', '%Y-%m-%d %H:%M:%S.%f')
    m = IOS_PREFIX.match(line)
    if not m:
        return None
    since_launch = timedelta(seconds=float(m.group(1)))
    start = IOS_START.search(line, m.end())
    if start:
        launch[0] = datetime.strptime(start.group(1), '%Y-%m-%d %H:%M:%S').replace(tzinfo=timezone.utc) - since_launch
    return launch[0] + since_launch if launch[0] else None


def parse_trip(text):
    row = {}
    after_net = False
    for token in text.split():
        key, _, value = token.partition('=')
        if key == 'net':
            after_net = True
        elif after_net and key in ('acc', 'age'):
            key = 'net_' + key
        if key in ('pos', 'net'):
            prefix = '' if key == 'pos' else 'net_'
            row[prefix + 'lat'], row[prefix + 'lon'] = value.split(',')
        elif key in ('gps', 'fused'):
            lat_lon, *rest = value.split('/')
            row[key + '_lat'], row[key + '_lon'] = lat_lon.split(',')
            row.update((f'{key}_{name}', v) for name, v in zip(PLATFORM_FIELDS, rest))
        else:
            row[key] = value
    return row


def number(row, key):
    try:
        return float(row[key])
    except (KeyError, ValueError):
        return None


def add_error(row):
    ref = 'gps' if 'gps_lat' in row else 'fused' if 'fused_lat' in row else None
    if not ref or 'lat' not in row:
        return
    ref_lat = float(row[ref + '_lat'])
    dx = math.radians(float(row['lon']) - float(row[ref + '_lon'])) * math.cos(math.radians(ref_lat)) * EARTH_RADIUS_M
    dy = math.radians(float(row['lat']) - ref_lat) * EARTH_RADIUS_M
    row['ref'] = ref
    kmh, course = number(row, ref + '_kmh'), number(row, ref + '_bear')
    if kmh is None or course is None:
        row['err_m'] = round(math.hypot(dx, dy), 1)
        return
    c = math.radians(course)
    # The car has moved on by the difference of the ages.
    along = dx * math.sin(c) + dy * math.cos(c) + kmh / 3.6 * (number(row, 'age') - number(row, ref + '_age'))
    across = dx * math.cos(c) - dy * math.sin(c)
    row.update(err_m=round(math.hypot(along, across), 1), along_m=round(along, 1), across_m=round(across, 1))


def read_rows(paths, year):
    rows = []
    for path in paths:
        launch = [None]
        with open(path, encoding='utf-8', errors='replace') as log:
            for line in log:
                time = parse_time(line, launch, year)
                trip = line.find(' TRIP src=')
                if time is None or trip < 0:
                    continue
                row = parse_trip(line[trip + len(' TRIP '):])
                row['time'] = time
                add_error(row)
                rows.append(row)
    rows.sort(key=lambda row: row['time'])
    for row in rows:
        row['time'] = row['time'].isoformat(timespec='milliseconds')
    return rows


def write_gpx(path, rows):
    def track(name, points):
        trkpts = ''.join(f'<trkpt lat="{lat}" lon="{lon}"><time>{time}</time></trkpt>\n' for lat, lon, time in points)
        return f'<trk><name>{name}</name><trkseg>\n{trkpts}</trkseg></trk>\n'

    shown = [(r['lat'], r['lon'], r['time']) for r in rows if 'lat' in r]
    platform = [(r[r['ref'] + '_lat'], r[r['ref'] + '_lon'], r['time']) for r in rows if 'ref' in r]
    with open(path, 'w', encoding='utf-8') as gpx:
        gpx.write('<?xml version="1.0" encoding="UTF-8"?>\n'
                  '<gpx version="1.1" creator="nogps_trip_log" xmlns="http://www.topografix.com/GPX/1/1">\n')
        gpx.write(track('Shown position', shown) + track('Platform position', platform) + '</gpx>\n')


def print_summary(rows):
    by_source = {}
    for row in rows:
        by_source.setdefault(row.get('src'), []).append(row)
    for source, source_rows in by_source.items():
        text = f'{source}: {len(source_rows)} lines'
        errors = sorted(float(r['err_m']) for r in source_rows if 'err_m' in r)
        if errors:
            text += (f', error median {errors[len(errors) // 2]} m, 90% {errors[len(errors) * 9 // 10]} m,'
                     f' max {errors[-1]} m')
        along = sorted(float(r['along_m']) for r in source_rows if 'along_m' in r)
        if along:
            text += f', along median {along[len(along) // 2]:+} m'
        print(text, file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('logs', nargs='+')
    parser.add_argument('--year', type=int, default=datetime.now().year, help='the year of an Android log')
    parser.add_argument('--gpx', help='also write the shown and the platform positions as GPX tracks')
    args = parser.parse_args()

    rows = read_rows(args.logs, args.year)
    columns = FIRST_COLUMNS + list(dict.fromkeys(k for row in rows for k in row if k not in FIRST_COLUMNS))
    out = csv.DictWriter(sys.stdout, columns)
    out.writeheader()
    out.writerows(rows)
    if args.gpx:
        write_gpx(args.gpx, rows)
    print_summary(rows)


if __name__ == '__main__':
    main()

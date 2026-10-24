#!/usr/bin/env python3
"""Drives a recorded trip of NonGPS Maps again in a debug build and tells what has changed.

A debug build of the Android application drives the file nogps-replay-in.txt put to its folder
(/sdcard/Android/data/<package>/files/) through a navigation of its own when it starts, and writes the trip log of
that drive to nogps-replay-out.txt, and nogps-replay-out.txt.done when it is over. See NoGpsReplay.cpp.

  nogps_replay.py input app*.log --day 10-11 -o nogps-replay-in.txt [--no-taps] [--no-route] [--from HH:MM]
      [--to HH:MM]
      makes the file to drive from the trip logs of a day: the rotation, the speed and the acceleration of the
      sensor box, the marks and the buttons of the user, the routes the user asked for. --no-taps leaves only the
      mark each trip starts with, --no-route drives without the routes. Where the application started anew has
      shown the car it kept, or the file begins with the car off the roads, the file gives that kept car instead of
      a tap: a tap would put it on a road.

  nogps_replay.py compare app*.log --day 10-11 --replay nogps-replay-out.txt
      how far the mark was from GPS on the trip and how far it is driven again, where GPS was seen.

The files made here keep the places of the trip: they are not for git. Nothing printed has coordinates.
"""
import argparse
import bisect
import collections
import math
import re

LINE = re.compile(r'^(\d\d-\d\d) (\d\d):(\d\d):(\d\d)\.(\d\d\d) \(([^)]*)\) \w/(\w+): (.*)$')
FIELD = re.compile(r' (\w+)=(\S+)')
SOURCE = re.compile(r'^\S+:\d+ (\w+)\(\): (.*)$')
TAP = re.compile(r'tap = ms::LatLon\((-?[\d.]+), (-?[\d.]+)\) location = ms::LatLon\((-?[\d.]+), (-?[\d.]+)\) '
                 r'bearing = (-?[\d.]+)')
CHECKPOINT = re.compile(r'ms::LatLon\((-?[\d.]+), (-?[\d.]+)\)')
SLICE_MS = 200
# A break of the box data longer than this is another trip.
MAX_STAND_FILL_MS = 600_000
# The car standing before the first data of the box is driven again for this long at most.
MAX_STAND_BEFORE_MS = 15_000


def read(paths, day):
    """The lines of the core of the day: the TRIP lines as dictionaries, the others as (ms of the day, text, the
    function that has written it), and when the application was started."""
    seen = set()
    trips = []
    others = []
    started = []
    for path in paths:
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                m = LINE.match(line.rstrip('\n'))
                if not m or m.group(1) != day or line in seen:
                    continue
                seen.add(line)
                ms = ((int(m.group(2)) * 60 + int(m.group(3))) * 60 + int(m.group(4))) * 1000 + int(m.group(5))
                body = m.group(8)
                if m.group(7) != 'OMcore':
                    if 'initNativePlatform()' in body:
                        started.append(ms)
                    continue
                source = SOURCE.match(body)
                function, text = (source.group(1), source.group(2)) if source else ('', body)
                if text.startswith('TRIP '):
                    fields = dict(FIELD.findall(' ' + text))
                    fields['ms'] = ms
                    trips.append(fields)
                else:
                    others.append((ms, text, function))
    trips.sort(key=lambda f: f['ms'])
    others.sort(key=lambda x: x[0])
    return trips, others, sorted(set(started))


def number(fields, name, default=None):
    value = fields.get(name)
    if value is None:
        return default
    try:
        return float(value.split('/')[0])
    except ValueError:
        return default


def lat_lon(value):
    try:
        lat, lon = value.split('/')[0].split(',')
        return float(lat), float(lon)
    except (ValueError, AttributeError):
        return None


def clock(ms):
    return '%02d:%02d:%02d' % (ms // 3600000, ms // 60000 % 60, ms // 1000 % 60)


def parse_clock(text):
    parts = [int(p) for p in text.split(':')]
    while len(parts) < 3:
        parts.append(0)
    return (parts[0] * 3600 + parts[1] * 60 + parts[2]) * 1000


def slices_of(lines, parse):
    """The parts of the seconds of GYRO or ACCB lines: (the end of the part in ms, its duration, the values)."""
    result = []
    previous_end = None
    for end, values in lines:
        duration = SLICE_MS
        # The box counts the parts by its own samples: they are a bit longer than 200 ms of the phone.
        if previous_end is not None and 900 <= end - previous_end <= 1300:
            duration = (end - previous_end) / len(values)
        previous_end = end
        for i, value in enumerate(values):
            parsed = parse(value)
            if parsed is not None:
                result.append((end - (len(values) - 1 - i) * duration, duration, parsed))
    return result


def make_input(args):
    trips, others, started = read(args.logs, args.day)
    start = parse_clock(args.start) if args.start else 0
    end = parse_clock(args.end) if args.end else 86_400_000

    gyro_lines = []
    accel_lines = []
    # What the application and the user did: (ms, order at the same time, the record, done by the user).
    actions = [(ms, 0, 'S %d 2' % ms, False) for ms in started]
    total_before = None
    by_itself_ms = None
    # The routes: (ms, the points the user asked a route through or None, the route is followed or closed).
    routes = []
    asked_ms = None
    # Where the application started anew has found the car it kept: (ms, the kept car as it was written).
    restores = []
    for ms, text, function in others:
        if text.startswith('GYRO '):
            parts = text.split()
            deltas = [float(p) for p in parts[2:7]]
            speeds = [int(p) for p in parts[7:12]]
            if len(deltas) != 5 or len(speeds) != 5:
                continue
            # The parts are rounded to 0.01 degrees and the total is not: what the rounding has lost in a second
            # is given back, so the heading doesn't drift over an hour.
            total = float(parts[1])
            if gyro_lines and 900 <= ms - gyro_lines[-1][0] <= 1300 and total_before is not None:
                lost = total - total_before - sum(deltas)
                if abs(lost) <= 0.2:
                    deltas = [d + lost / 5 for d in deltas]
            # What the car has rotated since the previous line, if a line between them is lost.
            between = total - sum(deltas) - total_before if total_before is not None else 0
            total_before = total
            gyro_lines.append((ms, [(d, v, between if i == 0 else 0) for i, (d, v) in enumerate(zip(deltas, speeds))]))
        elif text.startswith('ACCB_BEFORE '):
            parts = text.split()
            accel_lines.append((ms - int(parts[1]) * 1000, parts[2:7]))
        elif text.startswith('ACCB '):
            accel_lines.append((ms, text.split()[1:6]))
        elif function == 'Start' and text == 'Start':
            actions.append((ms, 1, 'S %d 1' % ms, False))
        elif function == 'Stop' and text == 'Stop':
            actions.append((ms, 1, 'S %d 0' % ms, False))
        elif function == 'OnBleState' and text.startswith('Bluetooth = '):
            actions.append((ms, 2, 'L %d %s' % (ms, text.split()[2]), False))
        elif function == 'SetGpsDisabled' and text.startswith('disabled = '):
            actions.append((ms, 3, 'X %d %s' % (ms, text.split()[2]), False))
        elif text.startswith('Entering the manual mode') or text.startswith('GPS is trusted again'):
            by_itself_ms = ms
        elif function == 'SetManualMode' and text.startswith('enabled = '):
            # The navigation turns the manual mode on and off by itself too, and does it again here.
            if by_itself_ms is None or ms - by_itself_ms > 5:
                actions.append((ms, 3, 'A %d %s' % (ms, text.split()[2]), False))
        elif text.startswith('tap = '):
            m = TAP.search(text)
            if m:
                actions.append((ms, 6, 'T %d %s %s' % (ms, m.group(1), m.group(2)), True))
                actions.append((ms, 6, 'H %d %s' % (ms, m.group(5)), True))
        elif function == 'RestoreCarPosition' and text.startswith('The car is where it was left = '):
            restores.append((ms, text.split(' = ', 1)[1].split(',')))
        elif text.startswith('Reversed the car'):
            actions.append((ms, 6, 'R %d' % ms, True))
        elif function == 'SwitchMark' and text == 'Switching the mark':
            actions.append((ms, 6, 'W %d' % ms, True))
        elif text.startswith('Moved the position by'):
            m = re.match(r'Moved the position by (-?[\d.]+)', text)
            if m:
                actions.append((ms, 6, 'M %d %s' % (ms, m.group(1)), True))
        elif text.startswith('paused = '):
            actions.append((ms, 6, 'P %d %s' % (ms, text.split()[2]), True))
        elif function == 'SetState' and text.startswith('Routing session state: '):
            # A route the user asks for is built, a route the car has left is rebuilt: the latter is done again by
            # the route of the trip driven again.
            state = text.split()[-1]
            if state == 'RouteBuilding':
                asked_ms = ms
            elif state == 'NoValidRoute':
                routes.append((ms, None, False))
        elif function == 'CalculateRoute' and text.startswith('Calculating the route') and asked_ms is not None:
            points = CHECKPOINT.findall(text.split('startDirection')[0])
            if len(points) >= 2 and ms - asked_ms <= 5000:
                routes.append((asked_ms, points, False))
            asked_ms = None
        elif function == 'EnableFollowMode' and text.split()[-1] in ('OnRoute', 'RouteNotStarted'):
            routes.append((ms, None, True))
    if args.no_route:
        routes = []
    for ms, points, follow in routes:
        if points:
            actions.append((ms, 7, 'N %d %s' % (ms, ' '.join('%s %s' % p for p in points)), False))
        else:
            actions.append((ms, 7, 'F %d %d' % (ms, follow), False))

    # GPS as the application has seen it, a position a second. What it has told to be spoofed is left out: it is
    # told so by the network positions, which are not driven again.
    def optional(text, scale):
        try:
            return '%.2f' % (float(text) * scale)
        except ValueError:
            return '-'

    fix_ms = None
    for f in trips:
        value = f.get('gps')
        if not value or f.get('spoofed') == '1':
            continue
        parts = value.split('/')
        position = lat_lon(value)
        try:
            accuracy = float(parts[1])
            age = float(parts[4])
        except (ValueError, IndexError):
            continue
        at = f['ms'] - age * 1000
        if position is None or age > 1.05 or (fix_ms is not None and at - fix_ms < 300):
            continue
        fix_ms = at
        # The accuracy of the bearing tells if a slow car takes its heading from GPS. Older logs don't have it:
        # the car looking where GPS did has taken it.
        if len(parts) > 5:
            bearing_accuracy = optional(parts[5], 1)
        else:
            bearing_accuracy = '-'
            try:
                took = abs((float(parts[3]) - number(f, 'hdg', 1e9) + 180) % 360 - 180) <= 6
                if took and f.get('hdg', '').endswith('/GPS'):
                    bearing_accuracy = '5.00'
            except ValueError:
                pass
        actions.append((at, 4, 'G %d %.6f %.6f %.1f %s %s %s' % (
            round(at), position[0], position[1], accuracy, optional(parts[2], 1 / 3.6), optional(parts[3], 1),
            bearing_accuracy), False))
    accel_lines.sort(key=lambda x: x[0])

    def accel_value(text):
        try:
            h1, h2, up, _ = [float(v) for v in text.split(',')]
        except ValueError:
            return None
        return round(h1 * 1000), round(h2 * 1000), round(up * 1000)

    gyro = slices_of(gyro_lines, lambda v: v)
    accel = slices_of(accel_lines, accel_value)
    accel_times = [a[0] for a in accel]
    trip_times = [f['ms'] for f in trips]
    # The car kept is given with the start that has found it, as old as the first trip line after it tells.
    for ms, kept in restores:
        opened = max((s for s in started if ms - 10_000 <= s <= ms), default=None)
        if opened is None or len(kept) != 5:
            continue
        i = bisect.bisect_left(trip_times, ms)
        age = next((number(f, 'markAge') * 1000 - (f['ms'] - ms) for f in trips[i:i + 5] if 'markAge' in f), 0)
        actions.append((opened, 0, 'C %d %s %s %s %s %d' % (opened, kept[0], kept[1], kept[2], kept[3], max(0, age)),
                        False))

    def accel_at(ms, window):
        i = bisect.bisect_left(accel_times, ms)
        best = None
        for j in (i - 1, i):
            if 0 <= j < len(accel) and (best is None or abs(accel[j][0] - ms) < abs(best[0] - ms)):
                best = accel[j]
        return best[2] if best and abs(best[0] - ms) <= window else None

    def flags_at(ms):
        i = bisect.bisect_right(trip_times, ms) - 1
        return trips[i].get('bf', '17') if i >= 0 else '17'

    def stands(from_ms, to_ms):
        """The application was running and the box was heard all the time: the car stood, a second it moves or
        rotates in is written."""
        rows = trips[bisect.bisect_left(trip_times, from_ms):bisect.bisect_right(trip_times, to_ms)]
        return (len(rows) >= (to_ms - from_ms) / 1000 * 0.8 - 1 and
                all(number(f, 'speed', -1) >= 0 and f.get('src') != 'GPS' for f in rows))

    def speed_at(ms):
        """The speed of the nearest trip line. A second of the box is not written when the car reports 0 at its end:
        the faster of the lines around drove on a car that stood, on October 18 at 21:23:30 a stand of 1.5 s was
        driven again as 0.4 s."""
        i = bisect.bisect_right(trip_times, ms)
        rows = [f for f in trips[max(0, i - 1):i + 1] if abs(f['ms'] - ms) <= 1500]
        nearest = min(rows, key=lambda f: abs(f['ms'] - ms), default=None)
        return int(number(nearest, 'speed', 0)) if nearest else 0

    records = []  # (ms, order at the same time, text)
    previous = None
    for at, duration, (delta, speed, between) in gyro:
        if not (start <= at <= end):
            continue
        gap = at - duration - previous if previous is not None else None
        if gap is None or gap > MAX_STAND_FILL_MS or (gap > 2 * SLICE_MS and not stands(previous, at - duration)):
            # Nothing has come from the box meanwhile. Then the car stood for some seconds before it moved: a
            # standing car is not written, but how long it stood tells which way it starts.
            begin = at - duration
            i = bisect.bisect_right(trip_times, begin) - 1
            while (i >= 0 and begin - trips[i]['ms'] <= MAX_STAND_BEFORE_MS and trips[i].get('speed') == '0' and
                   (previous is None or trips[i]['ms'] > previous)):
                i -= 1
            stood_from = trips[i + 1]['ms'] if i + 1 < len(trips) and trips[i + 1]['ms'] < begin else begin
            records.append((stood_from, 5, 'J %d' % round(stood_from)))
            count = round((begin - stood_from) / SLICE_MS)
            for k in range(count):
                t = stood_from + (begin - stood_from) * (k + 1) / count
                a = accel_at(t, 3000)
                records.append((t, 5, 'B %d %d 0 0 %s %s' % (round(t), round((begin - stood_from) / count),
                                                             flags_at(t), '%d %d %d' % a if a else '-')))
        elif gap > SLICE_MS / 2:
            # Nothing is written about the box while the car stands. A second of a driving car is lost from the
            # log now and then: the car has driven on with the speed of the trip line of that second, and has
            # rotated as much as the total rotation tells.
            count = max(1, round(gap / SLICE_MS))
            moves = gap <= 10_000 and any(speed_at(previous + gap * (i + 1) / count) > 0 for i in range(count))
            for i in range(count):
                t = previous + gap * (i + 1) / count
                a = accel_at(t, 3000)
                records.append((t, 5, 'B %d %d %d %d %s %s' % (
                    round(t), round(gap / count), round(between * 1000 / count) if moves else 0,
                    speed_at(t) if moves else 0, flags_at(t), '%d %d %d' % a if a else '-')))
        a = accel_at(at, 150) or accel_at(at, 3000)
        records.append((at, 5, 'B %d %d %d %d %s %s' % (round(at), round(duration), round(delta * 1000), speed,
                                                        flags_at(at), '%d %d %d' % a if a else '-')))
        previous = at
    if not records:
        print('no box data in the logs of', args.day)
        return
    first = round(records[0][0]) - 1000
    # The file begins when the application was started before that, if it was that day: with all it was given since.
    if any(start <= ms <= first for ms in started):
        first = max(ms for ms in started if start <= ms <= first)

    for ms, order, text, by_user in actions:
        if first <= ms <= end and not (by_user and args.no_taps):
            records.append((ms, order, text))

    # What the navigation was like when the file begins, unless the application is started right then.
    if not any(first <= ms <= first + 10_000 for ms in started):
        i = min(bisect.bisect_left(trip_times, first), len(trips) - 1)
        state = trips[i] if trips else {}
        position = lat_lon(state.get('pos'))
        heading = number(state, 'hdg')
        # A tap would put the car standing off the roads on a road, looking along it: on October 19 and 23 it stood
        # across its road in the yard, and the trip driven again went the other way. It is started as the
        # application keeps it then.
        kept = state.get('src') in ('MANUAL', 'INERTIAL') and position and state.get('road') == '0'
        records.append((first, -4, 'S %d 2' % first))
        if kept:
            records.append((first, -4, 'C %d %.6f %.6f %s 1 %d' % (
                first, position[0], position[1], '-' if heading is None else '%.1f' % heading,
                round(number(state, 'markAge', 0) * 1000))))
        records.append((first, -3, 'S %d 1' % first))
        if state.get('link', '').endswith('/CONNECTED'):
            records.append((first, -3, 'L %d CONNECTED' % first))
        # GPS switched off in the sheet stays so until the application is closed: on October 19 it was switched off
        # the day before, and the first GPS position came minutes after the file begins.
        disabled = None
        for ms, text, function in others:
            if function == 'SetGpsDisabled' and ms <= first:
                disabled = text.split()[2] == '1'
            elif disabled is None and ms >= first and function == 'SetGpsDisabled':
                break
            elif disabled is None and ms >= first and text.startswith('GPS is disabled, ignoring'):
                disabled = True
        if disabled:
            records.append((first, -2, 'X %d 1' % first))
        if state.get('src') in ('MANUAL', 'INERTIAL') and position and not kept:
            records.append((first, -1, 'A %d 1' % first))
            records.append((first, -1, 'T %d %.6f %.6f' % (first, position[0], position[1])))
            if heading is not None:
                records.append((first, -1, 'H %d %.0f' % (first, heading)))
        # The route the car follows when the file begins is asked for from where the car is.
        asked = None
        followed = False
        for ms, points, follow in routes:
            if ms >= first:
                break
            if points:
                asked, followed = points, False
            elif follow:
                followed = asked is not None
            else:
                asked = None
        if asked and position:
            there = ' '.join('%s %s' % p for p in asked[1:])
            records.append((first, 0, 'N %d %.6f %.6f %s' % (first, position[0], position[1], there)))
            if followed:
                records.append((first, 1, 'F %d 1' % first))

    records.sort(key=lambda r: (r[0], r[1]))
    with open(args.output, 'w', encoding='ascii', newline='\n') as f:
        f.write('D %s\n' % args.day)
        for _, _, text in records:
            f.write(text + '\n')
    kinds = collections.Counter(r[2][0] for r in records)
    print('written %s: %.1f h of the box in %d pieces, GPS %d, taps %d, reverse %d, pause %d, moved %d, '
          'GPS switched %d, manual mode %d, starts %d, routes asked for %d' % (
              args.output, kinds['B'] * SLICE_MS / 3.6e6, kinds['J'], kinds['G'], kinds['T'], kinds['R'], kinds['P'],
              kinds['M'], kinds['X'], kinds['A'], kinds['S'], kinds['N']))


def meters(a, b):
    north = (a[0] - b[0]) * 111320.0
    east = (a[1] - b[1]) * 111320.0 * math.cos(math.radians(a[0]))
    return math.hypot(north, east)


class GpsTrack:
    """GPS of the trip, to tell where the car was at any moment."""

    def __init__(self, trips):
        points = {}
        for f in trips:
            value = f.get('gps')
            if not value or f.get('spoofed') == '1':
                continue
            parts = value.split('/')
            position = lat_lon(value)
            try:
                accuracy = float(parts[1])
                age = float(parts[4])
            except (ValueError, IndexError):
                continue
            if position is None or accuracy > 12 or age > 2.5:
                continue
            points[round(f['ms'] - age * 1000)] = position
        self.times = sorted(points)
        self.points = [points[t] for t in self.times]

    def at(self, ms):
        i = bisect.bisect_left(self.times, ms)
        if i == 0 or i == len(self.times):
            return None
        t0, t1 = self.times[i - 1], self.times[i]
        if t1 - t0 > 2500:
            return None
        k = (ms - t0) / (t1 - t0)
        a, b = self.points[i - 1], self.points[i]
        return a[0] + (b[0] - a[0]) * k, a[1] + (b[1] - a[1]) * k


def summary(name, errors):
    if not errors:
        print('%-22s no seconds with GPS' % name)
        return
    errors = sorted(errors)
    n = len(errors)
    print('%-22s %5d s  median %4.0f m  90%% %4.0f m  over 30 m %4.1f%%  over 50 m %4.1f%%  over 100 m %4.1f%%' % (
        name, n, errors[n // 2], errors[n * 9 // 10], 100 * sum(e > 30 for e in errors) / n,
        100 * sum(e > 50 for e in errors) / n, 100 * sum(e > 100 for e in errors) / n))


EVENTS = [
    # Before October 16 the car followed apart was "paused by itself".
    ('road lost', r'road is lost after'), ('followed apart off the roads', r'paused by itself|it is followed apart'),
    ('turn moved to a crossing', r'is moved to the crossing'), ('turn not at a crossing', r'is not at a crossing'),
    ('roundabout or winding road', r'roundabout or a winding'), ('off the roads at a roundabout', r'at a roundabout'),
    ('started backward', r'has started backward'), ('way of a start changed', r'by the accelerometer, it has felt'),
    ('mark moved to the road of the car', r'the mark is put on the road going its way'),
    ('no road at the tap', r'^No road at the tap'), ('bend moved', r'^Bend: moved'),
    ('seconds on another branch of a fork', r'^FORK '),
    ('route asked for', r'Routing session state: \w+ -> RouteBuilding'),
    ('route rebuilt: the car left it', r'Routing session state: RouteNeedRebuild -> RouteRebuilding'),
    ('route rebuilt: the mark put off it', r'^Manual position is off the route'),
    ('route left: the car on a road beside it', r'beside the route, the route is left'),
    ('alternative route followed', r'is on the alternative route'),
    # A turn that fits several roads splits the mark, since October 17.
    ('other mark added', r'^Other mark added'), ('other mark dropped', r'^Other mark dropped'),
    ('mark switched to another road', r'^The car is shown on another road'),
    ('turn place told by the way driven', r'^Turn from .* WAY misses'),
]


def compare(args):
    trips, others, _ = read(args.logs, args.day)
    replayed, replayed_others, _ = read([args.replay], args.day)
    start = parse_clock(args.start) if args.start else 0
    end = parse_clock(args.end) if args.end else 86_400_000
    gps = GpsTrack(trips)
    if not replayed:
        print('nothing is driven again in', args.replay)
        return

    def errors_of(rows):
        result = {}
        for f in rows:
            if not (start <= f['ms'] <= end) or f.get('src') != 'INERTIAL' or number(f, 'speed', 0) <= 0:
                continue
            position = lat_lon(f.get('pos'))
            truth = gps.at(f['ms'])
            if position and truth:
                result[f['ms'] // 1000] = meters(position, truth)
        return result

    was = errors_of(trips)
    now = errors_of(replayed)
    both = sorted(set(was) & set(now))
    print('the day %s, driven again: %d s of TRIP lines, the trip had %d' % (args.day, len(replayed), len(trips)))
    summary('the trip, all', list(was.values()))
    summary('the trip, same seconds', [was[s] for s in both])
    summary('driven again', [now[s] for s in both])

    # The mark of the trip against the mark driven again, GPS or not.
    original = {f['ms'] // 1000: lat_lon(f.get('pos')) for f in trips if f.get('src') == 'INERTIAL'}
    apart = []
    for f in replayed:
        a, b = original.get(f['ms'] // 1000), lat_lon(f.get('pos'))
        if a and b and number(f, 'speed', 0) > 0 and start <= f['ms'] <= end:
            apart.append(meters(a, b))
    if apart:
        apart.sort()
        print('the two marks apart:   median %.0f m, 90%% %.0f m, over 30 m %.1f%% of %d s' % (
            apart[len(apart) // 2], apart[len(apart) * 9 // 10], 100 * sum(d > 30 for d in apart) / len(apart),
            len(apart)))

    print('%-34s %8s %8s' % ('', 'the trip', 'again'))
    for name, pattern in EVENTS:
        rx = re.compile(pattern)
        a = sum(1 for ms, text, _ in others if start <= ms <= end and rx.search(text))
        b = sum(1 for ms, text, _ in replayed_others if start <= ms <= end and rx.search(text))
        print('%-34s %8d %8d' % (name, a, b))
    for name, key in (('seconds stopped as lost', 'lost'), ('seconds followed apart', 'self'),
                      ('seconds paused', 'paused')):
        a = sum(1 for f in trips if start <= f['ms'] <= end and f.get(key) == '1' and number(f, 'speed', 0) > 0)
        b = sum(1 for f in replayed if start <= f['ms'] <= end and f.get(key) == '1' and number(f, 'speed', 0) > 0)
        print('%-34s %8d %8d' % (name, a, b))

    # Where it has changed: the stretches the mark driven again is much closer to GPS or much farther from it.
    stretches = []
    current = None
    for s in both:
        change = now[s] - was[s]
        kind = 'better' if change < -args.change else 'worse' if change > args.change else None
        if kind and current and current['kind'] == kind and s - current['to'] <= 3:
            current['to'] = s
            current['was'].append(was[s])
            current['now'].append(now[s])
        elif kind:
            current = dict(kind=kind, start=s, to=s, was=[was[s]], now=[now[s]])
            stretches.append(current)
        else:
            current = None
    stretches = [x for x in stretches if x['to'] - x['start'] >= 2]
    print('stretches of 3 s or longer changed by more than %d m: better %d (%d s), worse %d (%d s)' % (
        args.change, sum(x['kind'] == 'better' for x in stretches),
        sum(x['to'] - x['start'] + 1 for x in stretches if x['kind'] == 'better'),
        sum(x['kind'] == 'worse' for x in stretches),
        sum(x['to'] - x['start'] + 1 for x in stretches if x['kind'] == 'worse')))
    for x in stretches:
        print('  %s %3d s %-6s  the trip %4.0f m, again %4.0f m from GPS at the worst' % (
            clock(x['start'] * 1000), x['to'] - x['start'] + 1, x['kind'], max(x['was']), max(x['now'])))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest='command', required=True)
    for name in ('input', 'compare'):
        sub = commands.add_parser(name)
        sub.add_argument('logs', nargs='+', help='the trip logs of the application, app*.log')
        sub.add_argument('--day', required=True, help='MM-DD, as the lines of the log begin')
        sub.add_argument('--from', dest='start', help='HH:MM[:SS]')
        sub.add_argument('--to', dest='end', help='HH:MM[:SS]')
        if name == 'input':
            sub.add_argument('-o', '--output', default='nogps-replay-in.txt')
            sub.add_argument('--no-taps', action='store_true', help='only the mark every trip starts with')
            sub.add_argument('--no-route', action='store_true', help='without the routes the user asked for')
        else:
            sub.add_argument('--replay', required=True, help='nogps-replay-out.txt of the application')
            sub.add_argument('--change', type=float, default=15, help='meters that count as a change')
    args = parser.parse_args()
    if args.command == 'input':
        make_input(args)
    else:
        compare(args)


if __name__ == '__main__':
    main()

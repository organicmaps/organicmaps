#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import xml.etree.ElementTree as ET
import operator

from collections import namedtuple
from itertools import islice
from math import sin, cos, atan2, radians, sqrt

LatLon = namedtuple('LatLon', 'lat, lon')

def distance(x, y):
    """Implements https://en.wikipedia.org/wiki/Haversine_formula.

    >>> int(distance(LatLon(55.747043, 37.655554), LatLon(55.754892, 37.657013)))
    875
    >>> int(distance(LatLon(60.013918, 29.718361), LatLon(59.951572, 30.205536)))
    27910
    """

    φ1, φ2 = map(radians, [x.lat, y.lat])
    λ1, λ2 = map(radians, [x.lon, y.lon])
    Δφ = φ2 - φ1
    Δλ = λ2 - λ1
    a = sin(Δφ/2)**2 + cos(φ1) * cos(φ2) * sin(Δλ/2)**2
    R = 6356863  # Earth radius in meters.
    return 2 * R * atan2(sqrt(a), sqrt(1 - a))

def lcs(l1, l2, eq=operator.eq):
    """Finds the longest common subsequence of l1 and l2.
    Returns a list of common parts and a list of differences.

    >>> lcs([1, 2, 3], [2])
    ([2], [1, 3])
    >>> lcs([1, 2, 3, 3, 4], [2, 3, 4, 5])
    ([2, 3, 4], [1, 3, 5])
    >>> lcs('banana', 'baraban')
    (['b', 'a', 'a', 'n'], ['a', 'r', 'b', 'n', 'a'])
    >>> lcs('abraban', 'banana')
    (['b', 'a', 'a', 'n'], ['a', 'r', 'n', 'b', 'a'])
    >>> lcs([1, 2, 3], [4, 5])
    ([], [4, 5, 1, 2, 3])
    >>> lcs([4, 5], [1, 2, 3])
    ([], [1, 2, 3, 4, 5])
    """
    prefs_len = [
        [0] * (len(l2) + 1)
        for _ in range(len(l1) + 1)
    ]
    for i in range(1, len(l1) + 1):
        for j in range(1, len(l2) + 1):
            if eq(l1[i - 1], l2[j - 1]):
                prefs_len[i][j] = prefs_len[i - 1][j - 1] + 1
            else:
                prefs_len[i][j] = max(prefs_len[i - 1][j], prefs_len[i][j - 1])
    common = []
    diff = []
    i, j = len(l1), len(l2)
    while i and j:
        assert i >= 0
        assert j >= 0
        if l1[i - 1] == l2[j - 1]:
            common.append(l1[i - 1])
            i -= 1
            j -= 1
        elif prefs_len[i - 1][j] >= prefs_len[i][j - 1]:
            i -= 1
            diff.append(l1[i])
        else:
            j -= 1
            diff.append(l2[j])
    diff.extend(reversed(l1[:i]))
    diff.extend(reversed(l2[:j]))
    return common[::-1], diff[::-1]

def common_part(l1, l2):
    common, diff = lcs(l1, l2)
    common_len = sum(distance(*x) for x in common)
    diff_len = sum(distance(*x) for x in diff)
    assert (not common) or common_len
    assert (not diff) or diff_len
    return 1.0 - common_len / (common_len + diff_len)

class Segment:
    def __init__(self, segment_id, matched_route, golden_route):
        #TODO(mgsergio): Remove this when deal with auto golden routes.
        assert matched_route
        assert golden_route
        self.segment_id = segment_id
        self.matched_route = matched_route
        self.golden_route = golden_route or None

    def __repr__(self):
        return 'Segment({})'.format(self.segment_id)

    def as_tuple(self):
        return self.segment_id, self.matched_route, self.golden_route

def parse_route(route):
    if not route:
        return None
    result = []
    for edge in route.findall('RoadEdge'):
        start = edge.find('StartJunction')
        end = edge.find('EndJunction')
        result.append((
            LatLon(float(start.find('lat').text), float(start.find('lon').text)),
            LatLon(float(end.find('lat').text), float(end.find('lon').text))
        ))
    return result

def parse_segments(tree, limit):
    segments = islice(tree.findall('.//Segment'), limit)
    for s in segments:
        ignored = s.find('Ignored')
        if ignored is not None and ignored.text == 'true':
            continue
        segment_id = int(s.find('.//ReportSegmentID').text)
        matched_route = parse_route(s.find('Route'))
        # TODO(mgsergio): This is a temproraty hack. All untouched segments
        # within limit are considered accurate, so golden path should be equal
        # matched path.
        golden_route = parse_route(s.find('GoldenRoute')) or matched_route
        if not matched_route and not golden_route:
            raise ValueError('At least one of golden route or matched route should be present')
        try:
            yield Segment(segment_id, matched_route, golden_route)
        except:
            print('Broken segment is {}'.format(segment_id))
            raise

def calculate(tree):
    ms = sorted(
        (
            (
                s.segment_id,
                common_part(s.golden_route, s.matched_route)
            )
            for s in parse_segments(tree, args.limit)
        ),
        key=lambda x: -x[1]
    )
    print('{}\t{}'.format(
        'segment_id', 'intersection_weight')
    )
    for x in ms:
        print('{}\t{}'.format(*x))

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(
        description='Use this tool to get numerical scores on segments matching'
    )
    parser.add_argument(
        'assessed_path', type=str,
        help='An assessed matching file.')
    parser.add_argument(
        '-l', '--limit', type=int, default=None,
        help='Process no more than limit segments'
    )
    parser.add_argument(
        '--merge', type=str, default=None,
        help='A path to a file to take matched routes from'
    )

    args = parser.parse_args()

    tree = ET.parse(args.assessed_path)
    calculate(tree)

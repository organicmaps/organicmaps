#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import operator
import xml.etree.ElementTree as ET

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
        if eq(l1[i - 1], l2[j - 1]):
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

def almost_equal(s1, s2, eps=1e-5):
    """
    >>> a = (LatLon(55.77286, 37.8976), LatLon(55.77291, 37.89766))
    >>> b = (LatLon(55.77286, 37.89761), LatLon(55.77291, 37.89767))
    >>> almost_equal(a, b)
    True
    >>> a = (LatLon(55.89259, 37.72521), LatLon(55.89269, 37.72535))
    >>> b = (LatLon(55.89259, 37.72522), LatLon(55.8927, 37.72536))
    >>> almost_equal(a, b)
    True
    >>> a = (LatLon(55.89259, 37.72519), LatLon(55.89269, 37.72535))
    >>> b = (LatLon(55.89259, 37.72522), LatLon(55.8927, 37.72536))
    >>> almost_equal(a, b)
    False
    """
    eps *= 2
    return all(
        abs(p1.lat - p2.lat) <= eps and abs(p1.lon - p2.lon) <= eps
        for p1, p2 in zip(s1, s2)
    )

def common_part(l1, l2):
    assert l1, 'left hand side argument should not be empty'
    if not l2:
        return 0.0
    common, diff = lcs(l1, l2, eq=almost_equal)
    common_len = sum(distance(*x) for x in common)
    diff_len = sum(distance(*x) for x in diff)
    assert common_len + diff_len
    return common_len / (common_len + diff_len)

class Segment:
    class NoGoldenPathError(ValueError):
        pass

    def __init__(self, segment_id, golden_route, matched_route):
        if not golden_route:
            raise NoGoldenPathError(
                "segment {} does not have a golden route"
                .format(segment_id)
            )
        self.segment_id = segment_id
        self.golden_route = golden_route
        self.matched_route = matched_route or []

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
        yield Segment(segment_id, golden_route, matched_route)

def calculate(tree):
    result = {}
    for s in parse_segments(tree, args.limit):
        try:
            result[s.segment_id] = common_part(s.golden_route, s.matched_route)
        except AssertionError:
            print('Something is wrong with segment {}'.format(s))
            raise
        except Segment.NoGoldenPathError:
            raise
    return result

def merge(src, dst):
    # If segment was ignored it does not have a golden route.
    # We should mark the corresponding route in dst as ignored too.
    golden_routes = {
        int(s.find('.//ReportSegmentID').text): s.find('GoldenRoute')
        for s in src.findall('Segment')
    }
    for s in dst.findall('Segment'):
        assert not s.find('GoldenRoute')
        golden_route = golden_routes[int(s.find('.//ReportSegmentID').text)]
        if not golden_route:
            elem = ET.Element('Ignored')
            elem.text = 'true'
            s.append(elem)
            continue
        s.append(golden_route)

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

    assessed = ET.parse(args.assessed_path)

    assessed_scores = calculate(assessed)
    if args.merge:
        candidate = ET.parse(args.merge)
        merge(assessed, candidate)
        candidate_scores = calculate(candidate)

        print('{}\t{}\t{}\t{}'.format(
            'segment_id', 'A', 'B', 'Diff')
        )
        for seg_id in assessed_scores:
            print('{}\t{}\t{}\t{}'.format(
                seg_id,
                assessed_scores[seg_id], candidate_scores[seg_id],
                assessed_scores[seg_id] - candidate_scores[seg_id]
            ))
        mean1 = np.mean(list(assessed_scores.values()))
        std1 = np.std(list(assessed_scores.values()), ddof=1)
        mean2 = np.mean(list(candidate_scores.values()))
        std2 = np.std(list(candidate_scores.values()), ddof=1)
        # TODO(mgsergio): Use statistical methods to reason about quality.
        print('Base: mean: {:.4f}, std: {:.4f}'.format(mean1, std1))
        print('New: mean: {:.4f}, std: {:.4f}'.format(mean2, std2))
        print('{} is better on avarage: mean1 - mean2: {:.4f}'.format(
            'Base' if mean1 - mean2 > 0 else 'New',
            mean1 - mean2
        ))
    else:
        print('{}\t{}'.format(
            'segment_id', 'intersection_weight')
        )
        for x in assessed_scores.items():
            print('{}\t{}'.format(*x))
        print('mean: {:.4f}, std: {:.4f}'.format(
            np.mean(list(assessed_scores.values())),
            np.std(list(assessed_scores.values()), ddof=1)
        ))

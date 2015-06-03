#!/usr/bin/env python
# -*- coding: utf-8 -*-
#    This file is part of kothic, the realtime map renderer.

#   kothic is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   kothic is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with kothic.  If not, see <http://www.gnu.org/licenses/>.

type_matches = {
    "": ('area', 'line', 'way', 'node'),
    "area": ("area", "way"),
    "node": ("node",),
    "way": ("line", "area", "way"),
    "line": ("line", "area"),
    }

class Rule():
    def __init__(self, s=''):
        self.conditions = []
        # self.isAnd = True
        self.minZoom = 0
        self.maxZoom = 19
        if s == "*":
            s = ""
        self.subject = s    # "", "way", "node" or "relation"

    def __repr__(self):
        return "%s|z%s-%s %s" % (self.subject, self.minZoom, self.maxZoom, self.conditions)

    def test(self, obj, tags, zoom):
        if (zoom < self.minZoom) or (zoom > self.maxZoom):
            return False

        if (self.subject != '') and not _test_feature_compatibility(obj, self.subject, tags):
            return False

        subpart = "::default"
        for condition in self.conditions:
            res = condition.test(tags)
            if not res:
                return False
            if type(res) != bool:
                subpart = res
        return subpart

    def test_zoom(self, zoom):
        return (zoom >= self.minZoom) and (zoom <= self.maxZoom)

    def get_compatible_types(self):
        return type_matches.get(self.subject, (self.subject,))

    def get_interesting_tags(self, obj, zoom):
        if obj:
            if (self.subject != '') and not _test_feature_compatibility(obj, self.subject, {}):
                return set()

        if zoom and not self.test_zoom(zoom):
            return set()

        a = set()
        for condition in self.conditions:
            a.update(condition.get_interesting_tags())
        return a

    def extract_tags(self):
        a = set()
        for condition in self.conditions:
            a.update(condition.extract_tags())
            if "*" in a:
                break
        return a

    def get_numerics(self):
        a = set()
        for condition in self.conditions:
            a.add(condition.get_numerics())
        a.discard(False)
        return a

    def get_sql_hints(self, obj, zoom):
        if obj:
            if (self.subject != '') and not _test_feature_compatibility(obj, self.subject, {":area": "yes"}):
                return set()
        if not self.test_zoom(zoom):
            return set()
        a = set()
        b = set()
        for condition in self.conditions:
            q = condition.get_sql()
            if q:
                if q[1]:
                    a.add(q[0])
                    b.add(q[1])
        b = " AND ".join(b)
        return a, b


def _test_feature_compatibility(f1, f2, tags={}):
    """
    Checks if feature of type f1 is compatible with f2.
    """
    if f2 == f1:
        return True
    if f2 not in ("way", "area", "line"):
        return False
    elif f2 == "way" and f1 == "line":
        return True
    elif f2 == "way" and f1 == "area":
        return True
    elif f2 == "area" and f1 in ("way", "area"):
#      if ":area" in tags:
        return True
#      else:
#        return False
    elif f2 == "line" and f1 in ("way", "line", "area"):
        return True
    else:
        return False
    # print f1, f2, True
    return True

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
        self.runtime_conditions = []
        self.conditions = []
        # self.isAnd = True
        self.minZoom = 0
        self.maxZoom = 19
        if s == "*":
            s = ""
        self.subject = s    # "", "way", "node" or "relation"

    def __repr__(self):
        return "%s|z%s-%s %s %s" % (self.subject, self.minZoom, self.maxZoom, self.conditions, self.runtime_conditions)

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

    def get_compatible_types(self):
        return type_matches.get(self.subject, (self.subject,))

    def extract_tags(self):
        a = set()
        for condition in self.conditions:
            a.add(condition.extract_tag())
            if "*" in a:
                a = set(["*"])
                break
        return a


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

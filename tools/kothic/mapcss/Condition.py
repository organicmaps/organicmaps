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

import re

class Condition:
    def __init__(self, typez, params):
        self.type = typez         # eq, regex, lt, gt etc.
        if type(params) == type(str()):
            params = (params,)
        self.params = params      # e.g. ('highway','primary')
        if typez == "regex":
            self.regex = re.compile(self.params[0], re.I)

    def extract_tag(self):
        if self.params[0][:2] == "::" or self.type == "regex":
            return "*" # unknown
        return self.params[0]

    def test(self, tags):
        """
        Test a hash against this condition
        """
        t = self.type
        params = self.params
        if t == 'eq':   # don't compare tags against sublayers
            if params[0][:2] == "::":
                return params[1]
        try:
            if t == 'eq':
                return tags[params[0]] == params[1]
            if t == 'ne':
                return tags.get(params[0], "") != params[1]
            if t == 'regex':
                return bool(self.regex.match(tags[params[0]]))
            if t == 'true':
                return tags.get(params[0]) == 'yes'
            if t == 'untrue':
                return tags.get(params[0]) == 'no'
            if t == 'set':
                if params[0] in tags:
                    return tags[params[0]] != ''
                return False
            if t == 'unset':
                if params[0] in tags:
                    return tags[params[0]] == ''
                return True
            if t == '<':
                return (Number(tags[params[0]]) < Number(params[1]))
            if t == '<=':
                return (Number(tags[params[0]]) <= Number(params[1]))
            if t == '>':
                return (Number(tags[params[0]]) > Number(params[1]))
            if t == '>=':
                return (Number(tags[params[0]]) >= Number(params[1]))
        except KeyError:
            pass
        return False

    def __repr__(self):
        t = self.type
        params = self.params
        if t == 'eq' and params[0][:2] == "::":
            return "::%s" % (params[1])
        if t == 'eq':
            return "%s=%s" % (params[0], params[1])
        if t == 'ne':
            return "%s=%s" % (params[0], params[1])
        if t == 'regex':
            return "%s=~/%s/" % (params[0], params[1]);
        if t == 'true':
            return "%s?" % (params[0])
        if t == 'untrue':
            return "!%s?" % (params[0])
        if t == 'set':
            return "%s" % (params[0])
        if t == 'unset':
            return "!%s" % (params[0])
        if t == '<':
            return "%s<%s" % (params[0], params[1])
        if t == '<=':
            return "%s<=%s" % (params[0], params[1])
        if t == '>':
            return "%s>%s" % (params[0], params[1])
        if t == '>=':
            return "%s>=%s" % (params[0], params[1])
        return "%s %s " % (self.type, repr(self.params))

    def __eq__(self, a):
        return (self.params == a.params) and (self.type == a.type)

def Number(tt):
    """
    Wrap float() not to produce exceptions
    """
    try:
        return float(tt)
    except ValueError:
        return 0


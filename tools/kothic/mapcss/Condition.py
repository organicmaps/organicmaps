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

# Fast conditions

class EqConditionDD:
    def __init__(self, params):
        self.value = params[1]
    def extract_tags(self):
        return set(["*"])
    def test(self, tags):
        return self.value

class EqCondition:
    def __init__(self, params):
        self.tag = params[0]
        self.value = params[1]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] == self.value
        else:
            return False

class NotEqCondition:
    def __init__(self, params):
        self.tag = params[0]
        self.value = params[1]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] != self.value
        else:
            return False

class SetCondition:
    def __init__(self, params):
        self.tag = params[0]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] != ''
        return False

class UnsetCondition:
    def __init__(self, params):
        self.tag = params[0]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] == ''
        return True

class TrueCondition:
    def __init__(self, params):
        self.tag = params[0]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] == 'yes'
        return False

class UntrueCondition:
    def __init__(self, params):
        self.tag = params[0]
    def extract_tags(self):
        return set([self.tag])
    def test(self, tags):
        if self.tag in tags:
            return tags[self.tag] == 'no'
        return False

# Slow condition

class Condition:
    def __init__(self, typez, params):
        self.type = typez         # eq, regex, lt, gt etc.
        if type(params) == type(str()):
            params = (params,)
        self.params = params       # e.g. ('highway','primary')
        if typez == "regex":
            self.regex = re.compile(self.params[0], re.I)

    def extract_tags(self):
        if self.params[0][:2] == "::" or self.type == "regex":
            return set(["*"]) # unknown
        return set([self.params[0]])

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

# Some conditions we can optimize by using "python polymorthism"

def OptimizeCondition(condition):
    if (condition.type == "eq"):
        if (condition.params[0][:2] == "::"):
            return EqConditionDD(condition.params)
        else:
            return EqCondition(condition.params)
    elif (condition.type == "ne"):
        return NotEqCondition(condition.params)
    elif (condition.type == "set"):
        return SetCondition(condition.params)
    elif (condition.type == "unset"):
        return UnsetCondition(condition.params)
    elif (condition.type == "true"):
        return TrueCondition(condition.params)
    elif (condition.type == "untrue"):
        return UntrueCondition(condition.params)
    else:
        return condition

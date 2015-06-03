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

INVERSIONS = {"eq": "ne", "true": "false", "set": "unset", "<": ">=", ">": "<="}
in2 = {}
for a, b in INVERSIONS.iteritems():
    in2[b] = a
INVERSIONS.update(in2)
del in2

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
        self.compiled_regex = ""

    def get_interesting_tags(self):
        if self.params[0][:2] == "::":
            return []
        return set([self.params[0]])

    def extract_tags(self):
        if self.params[0][:2] == "::" or self.type == "regex":
            return set(["*"]) # unknown
        return set([self.params[0]])

    def get_numerics(self):
        if self.type in ("<", ">", ">=", "<="):
            return self.params[0]
        else:
            return False

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

    def inverse(self):
        """
        Get a not-A for condition A
        """
        t = self.type
        params = self.params
        try:
            return Condition(INVERSIONS[t], params)
            if t == 'regex':
                ### FIXME: learn how to invert regexes
                return Condition("regex", params)
        except KeyError:
            pass
        return self

    def get_sql(self):
        # params = [re.escape(x) for x in self.params]
        params = self.params
        t = self.type
        if t == 'eq':   # don't compare tags against sublayers
            if params[0][:2] == "::":
                return ("", "")
        try:
            if t == 'eq':
                return params[0], '"%s" = \'%s\'' % (params[0], params[1])
            if t == 'ne':
                return params[0], '("%s" != \'%s\' or "%s" IS NULL)' % (params[0], params[1], params[0])
            if t == 'regex':
                return params[0], '"%s" ~ \'%s\'' % (params[0], params[1].replace("'", "\\'"))
            if t == 'true':
                return params[0], '"%s" = \'yes\'' % (params[0])
            if t == 'untrue':
                return params[0], '"%s" = \'no\'' % (params[0])
            if t == 'set':
                return params[0], '"%s" IS NOT NULL' % (params[0])
            if t == 'unset':
                return params[0], '"%s" IS NULL' % (params[0])

            if t == '<':
                return params[0], """(CASE WHEN "%s"  ~  E'^[-]?[[:digit:]]+([.][[:digit:]]+)?$' THEN CAST ("%s" AS FLOAT) &lt; %s ELSE false END) """ % (params[0], params[0], params[1])
            if t == '<=':
                return params[0], """(CASE WHEN "%s"  ~  E'^[-]?[[:digit:]]+([.][[:digit:]]+)?$' THEN CAST ("%s" AS FLOAT) &lt;= %s ELSE false END)""" % (params[0], params[0], params[1])
            if t == '>':
                return params[0], """(CASE WHEN "%s"  ~  E'^[-]?[[:digit:]]+([.][[:digit:]]+)?$' THEN CAST ("%s" AS FLOAT) > %s ELSE false END) """ % (params[0], params[0], params[1])
            if t == '>=':
                return params[0], """(CASE WHEN "%s"  ~  E'^[-]?[[:digit:]]+([.][[:digit:]]+)?$' THEN CAST ("%s" AS FLOAT) >= %s ELSE false END) """ % (params[0], params[0], params[1])
        except KeyError:
            pass

    def get_mapnik_filter(self):
        # params = [re.escape(x) for x in self.params]
        params = self.params
        t = self.type
        if t == 'eq':   # don't compare tags against sublayers
            if params[0][:2] == "::":
                return ''
        try:
            if t == 'eq':
                return '[%s] = \'%s\'' % (params[0], params[1])
            if t == 'ne':
                return 'not([%s] = \'%s\')' % (params[0], params[1])
            if t == 'regex':
                return '[%s].match(\'%s\')' % (params[0], params[1].replace("'", "\\'"))
            if t == 'true':
                return '[%s] = \'yes\'' % (params[0])
            if t == 'untrue':
                return '[%s] = \'no\'' % (params[0])
            if t == 'set':
                return '[%s] != \'\'' % (params[0])
            if t == 'unset':
                return 'not([%s] != \'\')' % (params[0])

            if t == '<':
                return '[%s__num] &lt; %s' % (params[0], float(params[1]))
            if t == '<=':
                return '[%s__num] &lt;= %s' % (params[0], float(params[1]))
            if t == '>':
                return '[%s__num] &gt; %s' % (params[0], float(params[1]))
            if t == '>=':
                return '[%s__num] &gt;= %s' % (params[0], float(params[1]))
            # return ""
        except KeyError:
            pass

    def __repr__(self):
        return "%s %s " % (self.type, repr(self.params))

    def __eq__(self, a):
        return (self.params == a.params) and (self.type == a.type)

    def and_with(self, c2):
        """
        merges two rules with AND.
        """
        # TODO: possible other minimizations
        if c2.params[0] == self.params[0]:
            if c2.params == self.params:
                if c2.type == INVERSIONS[self.type]:  # for example,  eq AND ne = 0
                    return False
                if c2.type == self.type:
                    return (self,)

                if self.type == ">=" and c2.type == "<=":  # a<=2 and a>=2 --> a=2
                    return (Condition("eq", self.params),)
                if self.type == "<=" and c2.type == ">=":
                    return (Condition("eq", self.params),)
                if self.type == ">" and c2.type == "<":
                    return False
                if self.type == "<" and c2.type == ">":
                    return False

            if c2.type == "eq" and self.type in ("ne", "<", ">"):
                if c2.params[1] != self.params[1]:
                    return (c2,)
            if self.type == "eq" and c2.type in ("ne", "<", ">"):
                if c2.params[1] != self.params[1]:
                    return (self,)
            if self.type == "eq" and c2.type == "eq":
                if c2.params[1] != self.params[1]:
                    return False
            if c2.type == "set" and self.type in ("eq", "ne", "regex", "<", "<=", ">", ">="):  # a is set and a == b -> a == b
                return (self,)
            if c2.type == "unset" and self.type in ("eq", "ne", "regex", "<", "<=", ">", ">="):  # a is unset and a == b -> impossible
                return False
            if self.type == "set" and c2.type in ("eq", "ne", "regex", "<", "<=", ">", ">="):
                return (c2,)
            if self.type == "unset" and c2.type in ("eq", "ne", "regex", "<", "<=", ">", ">="):
                return False

        return self, c2

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

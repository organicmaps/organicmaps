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


class Eval():
    def __init__(self, s='eval()'):
        """
        Parse expression and convert it into Python
        """
        s = s.strip()[5:-1].strip()
        self.expr_text = s
        try:
            self.expr = compile(s, "MapCSS expression", "eval")
        except:
            # print "Can't compile %s" % s
            self.expr = compile("0", "MapCSS expression", "eval")

    def extract_tags(self):
        """
        Extracts list of tags that might be used in calculation
        """
        def fake_compute(*x):
            """
            Perform a fake computation. Always computes all the parameters, always returns 0.
            WARNING: Might not cope with complex statements.
            """
            for t in x:
                q = x
            return 0

        # print self.expr_text
        tags = set([])
        a = eval(self.expr, {}, {
                 "tag": lambda x: max([tags.add(x), 0]),
                 "prop": lambda x: 0,
                 "num": lambda x: 0,
                 "metric": fake_compute,
                 "zmetric": fake_compute,
                 "str": lambda x: "",
                 "any": fake_compute,
                 "min": fake_compute,
                 "max": fake_compute,
                 })
        return tags

    def compute(self, tags={}, props={}, xscale=1., zscale=0.5):
        """
        Compute this eval()
        """
        """
        for k, v in tags.iteritems():
            try:
                tags[k] = float(v)
            except:
                pass
        """
        try:
            return str(eval(self.expr, {}, {
                            "tag": lambda x: tags.get(x, ""),
                            "prop": lambda x: props.get(x, ""),
                            "num": m_num,
                            "metric": lambda x: m_metric(x, xscale),
                            "zmetric": lambda x: m_metric(x, zscale),
                            "str": str,
                            "any": m_any,
                            "min": m_min,
                            "max": m_max,
                            "cond": m_cond,
                            "boolean": m_boolean
                            }))
        except:
            return ""

    def __repr__(self):
        return "eval(%s)" % self.expr_text


def m_boolean(expr):
    expr = str(expr)
    if expr in ("", "0", "no", "false", "False"):
        return False
    else:
        return True


def m_cond(why, yes, no):
    if m_boolean(why):
        return yes
    else:
        return no


def m_min(*x):
    """
    min() MapCSS Feature
    """
    try:
        return min([m_num(t) for t in x])
    except:
        return 0


def m_max(*x):
    """
    max() MapCSS Feature
    """
    try:
        return max([m_num(t) for t in x])
    except:
        return 0


def m_any(*x):
    """
    any() MapCSS feature
    """
    for t in x:
        if t:
            return t
    else:
        return ""


def m_num(x):
    """
    num() MapCSS feature
    """
    try:
        return float(str(x))
    except ValueError:
        return 0


def m_metric(x, t):
    """
    metric() and zmetric() function.
    """
    x = str(x)
    try:
        return float(x) * float(t)
    except:
        "Heuristics."
        # FIXME: add ft, m and friends
        x = x.strip()
        try:
            if x[-2:] in ("cm", "CM", "см"):
                return float(x[0:-2]) * float(t) / 100
            if x[-2:] in ("mm", "MM", "мм"):
                return float(x[0:-2]) * float(t) / 1000
            if x[-1] in ("m", "M", "м"):
                return float(x[0:-1]) * float(t)
        except:
            return ""


# def str(x):
    #"""
    # str() MapCSS feature
    #"""
    # return __builtins__.str(x)


if __name__ == "__main__":
    a = Eval(""" eval( any( metric(tag("height")), metric ( num(tag("building:levels")) * 3), metric("1m"))) """)
    print repr(a)
    print a.compute({"building:levels": "3"})
    print a.extract_tags()

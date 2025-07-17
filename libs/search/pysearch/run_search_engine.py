#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
import argparse
import os
import pysearch as search


DIR = os.path.dirname(__file__)
RESOURCE_PATH = os.path.realpath(os.path.join(DIR, '..', '..', 'data'))
MWM_PATH = os.path.realpath(os.path.join(DIR, '..', '..', 'data'))

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-r', metavar='RESOURCE_PATH', default=RESOURCE_PATH, help='Path to resources directory.')
parser.add_argument('-m', metavar='MWM_PATH', default=MWM_PATH, help='Path to mwm files.')
args = parser.parse_args()

search.init(args.r, args.m)
engine = search.SearchEngine()

params = search.Params()
params.query = 'кафе юность'
params.locale = 'ru'
params.position = search.Mercator(37.618705, 67.455669)
params.viewport = search.Viewport(search.Mercator(37.1336, 67.1349),
                                  search.Mercator(38.0314, 67.7348))
print(engine.query(params))
print(engine.trace(params))

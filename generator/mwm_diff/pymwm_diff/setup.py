#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, '..', '..', '..'))

from pyhelpers.setup import setup_omim_pybinding


NAME = "pymwm_diff"

setup_omim_pybinding(name=NAME)

#!/usr/bin/env python
# coding: utf8
from __future__ import print_function

from collections import namedtuple, defaultdict
from datetime import datetime
from sklearn import metrics
import argparse
import base64
import json
import logging
import matplotlib.pyplot as plt
import os
import pickle
import time
import urllib2
import re

# init logging
logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(levelname)s: %(message)s')


def load_binary_list(path):
    """Loads reference binary classifier output. """
    bits = []
    with open(path, 'r') as fd:
        for line in fd:
            if (not line.strip()) or line.startswith('#'):
                continue
            bits.append(1 if line.startswith('y') else 0)
    return bits


def load_score_list(path):
    """Loads list of matching scores. """
    scores = []
    with open(path, 'r') as fd:
        for line in fd:
            if (not line.strip()) or line.startswith('#'):
                continue
            scores.append(float(re.search(r'result score: (\d*\.\d+)', line).group(1)))
    return scores


def process_options():
    # TODO(mgsergio): Fix description.
    parser = argparse.ArgumentParser(description="Download and process booking hotels.")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose")
    parser.add_argument("-q", "--quiet", action="store_false", dest="verbose")

    parser.add_argument("--reference_list", dest="reference_list", help="Path to data files")
    parser.add_argument("--sample_list", dest="sample_list", help="Name and destination for output file")

    parser.add_argument("--show", dest="show", default=False, action="store_true",
                        help="Show graph for precision and recall")

    options = parser.parse_args()

    if not options.reference_list or not options.sample_list:
        parser.print_help()
        exit()

    return options


def main():
    options = process_options()
    reference = load_binary_list(options.reference_list)
    sample = load_score_list(options.sample_list)

    precision, recall, threshold = metrics.precision_recall_curve(reference, sample)
    aa = zip(precision, recall, threshold)
    max_by_hmean = max(aa, key=lambda (p, r, t): p*r/(p+r))
    print("Optimal threshold: {2} for precision: {0} and recall: {1}".format(*max_by_hmean))
    print("AUC: {0}".format(metrics.roc_auc_score(reference, sample)))

    if options.show:
        plt.plot(recall, precision)
        plt.title("Precision/Recall")
        plt.ylabel("Precision")
        plt.xlabel("Recall")
        plt.show()


if __name__ == "__main__":
    main()

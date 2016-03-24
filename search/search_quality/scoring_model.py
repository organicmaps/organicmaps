#!/usr/bin/env python3

from math import exp, log
from sklearn import cross_validation, grid_search, svm
import argparse
import collections
import itertools
import numpy as np
import pandas as pd
import sys

FEATURES = ['DistanceToPivot', 'Rank', 'NameScore', 'SearchType']

DISTANCE_WINDOW = 1e9
MAX_RANK = 255
RELEVANCES = {'Irrelevant': 0, 'Relevant': 1, 'Vital': 3}
NAME_SCORES = ['Zero', 'Substring Prefix', 'Substring', 'Full Match Prefix', 'Full Match']
SEARCH_TYPES = {'POI': 0,
                'BUILDING': 0,
                'STREET': 1,
                'UNCLASSIFIED': 2,
                'VILLAGE': 3,
                'CITY': 4,
                'STATE': 5,
                'COUNTRY': 6}


def normalize_data(data):
    transform_distance = lambda d: exp(- d * 1000 / DISTANCE_WINDOW)

    max_name_score = len(NAME_SCORES) - 1
    max_search_type = SEARCH_TYPES['COUNTRY']

    data['DistanceToPivot'] = data['DistanceToPivot'].apply(transform_distance)
    data['Rank'] = data['Rank'].apply(lambda rank: rank / MAX_RANK)
    data['NameScore'] = data['NameScore'].apply(lambda s: NAME_SCORES.index(s) / max_name_score)
    data['SearchType'] = data['SearchType'].apply(lambda t: SEARCH_TYPES[t] / max_search_type)
    data['Relevance'] = data['Relevance'].apply(lambda r: RELEVANCES[r])


def compute_ndcg(relevances):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    array of scores.
    """

    relevances_summary = collections.defaultdict(int)

    dcg = 0
    for i, relevance in enumerate(relevances):
        dcg += relevance / log(2 + i, 2)
        relevances_summary[relevance] += 1

    dcg_norm, i = 0, 0
    for relevance in sorted(relevances_summary.keys(), reverse=True):
        for _ in range(relevances_summary[relevance]):
            dcg_norm += relevance / log(2 + i, 2)
            i += 1

    if dcg_norm == 0:
        return 0
    return dcg / dcg_norm


def compute_ndcg_without_w(data):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    data. Returns an array of ndcg scores in the shape [num groups of
    features].
    """

    grouped = data.groupby(data['SampleId'], sort=False).groups

    ndcgs = []
    for id in grouped:
        indices = grouped[id]
        relevances = np.array(data.ix[indices]['Relevance'])
        ndcgs.append(compute_ndcg(relevances))

    return np.array(ndcgs)


def compute_ndcg_for_w(data, w):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    data and an array of coeffs in a linear model. Returns an array of
    ndcg scores in the shape [num groups of features].
    """

    data_scores = np.array([np.dot(data.ix[i][FEATURES], w) for i in data.index])
    grouped = data.groupby(data['SampleId'], sort=False).groups

    ndcgs = []
    for id in grouped:
        indices = grouped[id]

        relevances = np.array(data.ix[indices]['Relevance'])
        scores = data_scores[indices]

        # Reoders relevances in accordance with decreasing scores.
        relevances = relevances[scores.argsort()[::-1]]
        ndcgs.append(compute_ndcg(relevances))

    return np.array(ndcgs)


def transform_data(data):
    """
    By a given data computes x and y that can be used as an input to a
    linear SVM.
    """

    grouped = data.groupby(data['SampleId'], sort=False)

    xs, ys = [], []

    # k is used to create a balanced samples set for better linear
    # separation.
    k = 1
    for _, group in grouped:
        features, relevances = group[FEATURES], group['Relevance']

        n, total = len(group), 0
        for _, (i, j) in enumerate(itertools.combinations(range(n), 2)):
            y = np.sign(relevances.iloc[j] - relevances.iloc[i])
            if y == 0:
                continue

            x = np.array(features.iloc[j]) - np.array(features.iloc[i])
            if y != k:
                x = np.negative(x)
                y = -y

            xs.append(x)
            ys.append(y)
            total += 1
            k = -k

        # Scales this group of features to equalize different search
        # queries.
        for i in range(-1, -total, -1):
            xs[i] = xs[i] / total
    return xs, ys


def main(args):
    data = pd.read_csv(sys.stdin)
    normalize_data(data)

    ndcg = compute_ndcg_without_w(data);
    print('Current NDCG: {}, std: {}'.format(np.mean(ndcg), np.std(ndcg)))
    print()

    x, y = transform_data(data)

    clf = svm.LinearSVC(random_state=args.seed)
    cv = cross_validation.KFold(len(y), n_folds=5, shuffle=True, random_state=args.seed)

    # "C" stands for the regularizer constant.
    grid = {'C': np.power(10.0, np.arange(-5, 6))}
    gs = grid_search.GridSearchCV(clf, grid, scoring='accuracy', cv=cv)
    gs.fit(x, y)

    w = gs.best_estimator_.coef_[0]
    ndcg = compute_ndcg_for_w(data, w)

    print('NDCG mean: {}, std: {}'.format(np.mean(ndcg), np.std(ndcg)))
    print()
    print('Linear model weights:')
    for f, c in zip(FEATURES, w):
        print('{}: {}'.format(f, c))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--seed', help='random seed', type=int)
    args = parser.parse_args()
    main(args)

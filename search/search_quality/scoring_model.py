#!/usr/bin/env python3

from math import exp, log
from sklearn import cross_validation, grid_search, svm
import argparse
import collections
import itertools
import numpy as np
import pandas as pd
import sys

FEATURES = ['MinDistance', 'Rank', 'SearchType', 'NameScore', 'NameCoverage']

DISTANCE_WINDOW = 1e9
MAX_RANK = 256
RELEVANCES = {'Irrelevant': 0, 'Relevant': 1, 'Vital': 3}
NAME_SCORES = ['Zero', 'Substring Prefix', 'Substring', 'Full Match Prefix', 'Full Match']
SEARCH_TYPES = ['POI', 'BUILDING', 'STREET', 'UNCLASSIFIED', 'VILLAGE', 'CITY', 'STATE', 'COUNTRY']


def normalize_data(data):
    transform_distance = lambda d: exp(-d / DISTANCE_WINDOW)

    data['DistanceToViewport'] = data['DistanceToViewport'].apply(transform_distance)
    data['DistanceToPosition'] = data['DistanceToPosition'].apply(transform_distance)
    data['Rank'] = data['Rank'].apply(lambda rank: rank / MAX_RANK)
    data['NameScore'] = data['NameScore'].apply(lambda s: NAME_SCORES.index(s) / len(NAME_SCORES))
    data['SearchType'] = data['SearchType'].apply(
        lambda t: SEARCH_TYPES.index(t) / len(SEARCH_TYPES))
    data['Relevance'] = data['Relevance'].apply(lambda r: RELEVANCES[r])
    data['MinDistance'] = pd.Series(np.minimum(data['DistanceToViewport'], data['DistanceToPosition']))


def compute_ndcg(scores):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    array of scores.
    """

    scores_summary = collections.defaultdict(int)

    dcg = 0
    for i, score in enumerate(scores):
        dcg += score / log(2 + i, 2)
        scores_summary[score] += 1

    dcg_norm, i = 0, 0
    for score in sorted(scores_summary.keys(), reverse=True):
        for _ in range(scores_summary[score]):
            dcg_norm += score / log(2 + i, 2)
            i += 1

    if dcg_norm == 0:
        return 0
    return dcg / dcg_norm


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

    grouped = data.groupby(data['SampleId'], sort=False).groups

    xs, ys = [], []
    for id in grouped:
        indices = grouped[id]
        features = data.ix[indices][FEATURES]
        relevances = np.array(data.ix[indices]['Relevance'])

        n, total = len(indices), 0
        for _, (i, j) in enumerate(itertools.combinations(range(n), 2)):
            y = np.sign(relevances[j] - relevances[i])
            if y == 0:
                continue
            x = (np.array(features.iloc[j]) - np.array(features.iloc[i]))
            xs.append(x)
            ys.append(y)
            total = total + 1

        # Scales this group of features to equalize different search
        # queries.
        for i in range(-1, -total, -1):
            xs[i] = xs[i] / total
    return xs, ys


def main(args):
    data = pd.read_csv(sys.stdin)
    normalize_data(data)
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

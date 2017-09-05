#!/usr/bin/env python3

from math import exp, log
from scipy.stats import pearsonr
from sklearn import svm
from sklearn.model_selection import GridSearchCV, KFold
import argparse
import collections
import itertools
import numpy as np
import pandas as pd
import random
import sys


MAX_DISTANCE_METERS = 2e6
MAX_RANK = 255
RELEVANCES = {'Irrelevant': 0, 'Relevant': 1, 'Vital': 3}
NAME_SCORES = ['Zero', 'Substring', 'Prefix', 'Full Match']
SEARCH_TYPES = ['POI', 'Building', 'Street', 'Unclassified', 'Village', 'City', 'State', 'Country']
FEATURES = ['DistanceToPivot', 'Rank', 'FalseCats', 'ErrorsMade'] + NAME_SCORES + SEARCH_TYPES


def transform_name_score(value, categories_match):
    if categories_match == 1:
        return 'Zero'
    else:
        return value


def normalize_data(data):
    transform_distance = lambda v: min(v, MAX_DISTANCE_METERS) / MAX_DISTANCE_METERS

    data['DistanceToPivot'] = data['DistanceToPivot'].apply(transform_distance)
    data['Rank'] = data['Rank'].apply(lambda v: v / MAX_RANK)
    data['Relevance'] = data['Relevance'].apply(lambda v: RELEVANCES[v])

    cats = data['PureCats'].combine(data['FalseCats'], max)

    # TODO (@y, @m): do forward/backward/subset selection of features
    # instead of this merging.  It would be great to conduct PCA on
    # the features too.
    data['NameScore'] = data['NameScore'].combine(cats, transform_name_score)

    # Adds dummy variables to data for NAME_SCORES.
    for ns in NAME_SCORES:
        data[ns] = data['NameScore'].apply(lambda v: int(ns == v))

    # Adds dummy variables to data for SEARCH_TYPES.

    # We unify BUILDING with POI here, as we don't have enough
    # training data to distinguish between them.  Remove following
    # line as soon as the model will be changed or we will have enough
    # training data.
    data['SearchType'] = data['SearchType'].apply(lambda v: v if v != 'Building' else 'POI')
    for st in SEARCH_TYPES:
        data[st] = data['SearchType'].apply(lambda v: int(st == v))


def compute_ndcg(relevances):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    array of scores.
    """

    dcg = sum(r / log(2 + i, 2) for i, r in enumerate(relevances))
    dcg_norm = sum(r / log(2 + i, 2) for i, r in enumerate(sorted(relevances, reverse=True)))
    return dcg / dcg_norm if dcg_norm != 0 else 0


def compute_ndcgs_without_ws(data):
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

    return ndcgs


def compute_ndcgs_for_ws(data, ws):
    """
    Computes NDCG (Normalized Discounted Cumulative Gain) for a given
    data and an array of coeffs in a linear model. Returns an array of
    ndcg scores in the shape [num groups of features].
    """

    data_scores = np.array([np.dot(data.ix[i][FEATURES], ws) for i in data.index])
    grouped = data.groupby(data['SampleId'], sort=False).groups

    ndcgs = []
    for id in grouped:
        indices = grouped[id]

        relevances = np.array(data.ix[indices]['Relevance'])
        scores = data_scores[indices]

        # Reoders relevances in accordance with decreasing scores.
        relevances = relevances[scores.argsort()[::-1]]
        ndcgs.append(compute_ndcg(relevances))

    return ndcgs


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
            dr = relevances.iloc[j] - relevances.iloc[i]
            y = np.sign(dr)
            if y == 0:
                continue

            x = np.array(features.iloc[j]) - np.array(features.iloc[i])

            # Need to multiply x by average drop in NDCG when i-th and
            # j-th are exchanged.
            x *= abs(dr * (1 / log(j + 2, 2) - 1 / log(i + 2, 2)))

            # This is needed to prevent disbalance in classes sizes.
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


def show_pearson_statistics(xs, ys, features):
    """
    Shows info about Pearson coefficient between features and
    relevancy.
    """

    print('***** Correlation table *****')
    print('H0 - feature not is correlated with relevancy')
    print('H1 - feature is correlated with relevancy')
    print()

    cs, ncs = [], []
    for i, f in enumerate(features):
        zs = [x[i] for x in xs]
        (c, p) = pearsonr(zs, ys)

        correlated = p < 0.05
        print('{}: pearson={:.3f}, P(H1)={}'.format(f, c, 1 - p))
        if correlated:
            cs.append(f)
        else:
            ncs.append(f)

    print()
    print('Correlated:', cs)
    print('Non-correlated:', ncs)


def raw_output(features, ws):
    """
    Prints feature-coeff pairs to the standard output.
    """

    for f, w in zip(features, ws):
        print('{}: {}'.format(f, w))


def print_const(name, value):
    print('double const k{} = {:.7f};'.format(name, value))


def print_array(name, size, values):
    print('double const {}[{}] = {{'.format(name, size))
    print(',\n'.join('  {:.7f} /* {} */'.format(w, f) for (f, w) in values))
    print('};')

def cpp_output(features, ws):
    """
    Prints feature-coeff pairs in the C++-compatible format.
    """

    ns, st = [], []

    for f, w in zip(features, ws):
        if f in NAME_SCORES:
            ns.append((f, w))
        elif f in SEARCH_TYPES:
            st.append((f, w))
        else:
            print_const(f, w)
    print_array('kNameScore', 'NameScore::NAME_SCORE_COUNT', ns)
    print_array('kType', 'Model::TYPE_COUNT', st)


def main(args):
    data = pd.read_csv(sys.stdin)
    normalize_data(data)

    ndcgs = compute_ndcgs_without_ws(data);
    print('Current NDCG: {:.3f}, std: {:.3f}'.format(np.mean(ndcgs), np.std(ndcgs)))
    print()

    xs, ys = transform_data(data)

    clf = svm.LinearSVC(random_state=args.seed)
    cv = KFold(n_splits=5, shuffle=True, random_state=args.seed)

    # "C" stands for the regularizer constant.
    grid = {'C': np.power(10.0, np.arange(-5, 6))}
    gs = GridSearchCV(clf, grid, scoring='roc_auc', cv=cv)
    gs.fit(xs, ys)

    ws = gs.best_estimator_.coef_[0]
    max_w = max(abs(w) for w in ws)
    ws = np.divide(ws, max_w)

    # Following code restores coeffs for merged features.
    ws[FEATURES.index('Building')] = ws[FEATURES.index('POI')]

    ndcgs = compute_ndcgs_for_ws(data, ws)

    print('NDCG mean: {:.3f}, std: {:.3f}'.format(np.mean(ndcgs), np.std(ndcgs)))
    print('ROC AUC: {:.3f}'.format(gs.best_score_))

    if args.pearson:
        print()
        show_pearson_statistics(xs, ys, FEATURES)

    print()
    print('***** Linear model weights *****')
    if args.cpp:
        cpp_output(FEATURES, ws)
    else:
        raw_output(FEATURES, ws)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--seed', help='random seed', type=int)
    parser.add_argument('--pearson', help='show pearson statistics', action='store_true')
    parser.add_argument('--cpp', help='generate output in the C++ format', action='store_true')
    args = parser.parse_args()
    main(args)

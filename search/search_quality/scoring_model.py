#!/usr/bin/env python3
from math import exp, log
import collections
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


def gradient_descent(w_init, grad, eps=1e-6, rate=1e-6, lam=1e-3, num_steps=1000):
    n = len(w_init)
    w, dw = np.copy(w_init), np.zeros(n)
    for step in range(1, num_steps):
        wn = w - rate / step * grad(w) + lam * dw
        w, dw = wn, wn - w
        if np.linalg.norm(dw) < eps:
            break
    return w


class NaiveLoss:
    """
    Represents a gradient implementation for a naive loss function f,
    such that:

    df / dx = (f(x + eps) - f(x)) / eps
    """

    def __init__(self, data, eps=1e-6):
        self.data, self.eps = data, eps

    def value(self, w):
        return compute_ndcg_for_w(self.data, w)

    def gradient(self, w):
        n = len(w)
        g = np.zeros(n)

        fw = self.value(w)
        for i in range(n):
            w[i] += self.eps
            g[i] = (self.value(w) - fw) / self.eps
            w[i] -= self.eps
        return g


class RankingSVMLoss:
    """
    Represents a loss function with a gradient for a RankingSVM model.
    Simple version of a loss function for a ranked list of features
    has following form:

    loss(w) = sum{i < j: max(0, 1 - sign(y[j] - y[i]) * dot(w, x[j] - x[i]))} + lam * dot(w, w)

    This version is slightly modified, as we are dealing with a group
    of ranked lists, so loss function is actually a weighted sum of
    loss values for each list, where each weight is a 1 / list size.
    """

    @staticmethod
    def sign(x):
        if x < 0:
            return -1
        elif x > 0:
            return 1
        return 0


    def __init__(self, data, lam=1e-3):
        self.coeffs, self.lam = [], lam

        grouped = data.groupby(data['SampleId'], sort=False).groups
        for id in grouped:
            indices = grouped[id]
            features = data.ix[indices][FEATURES]
            relevances = np.array(data.ix[indices]['Relevance'])
            n = len(indices)
            for i in range(n):
                for j in range(i + 1, n):
                    y = self.sign(relevances[j] - relevances[i]) / n
                    dx = y * (np.array(features.iloc[j]) - np.array(features.iloc[i]))
                    self.coeffs.append(dx)


    def value(self, w):
        result = self.lam * np.dot(w, w)
        for coeff in self.coeffs:
            v = 1 - np.dot(coeff, w)
            if v > 0:
                result += v
        return result


    def gradient(self, w):
        result = 2 * self.lam * w
        for coeff in self.coeffs:
            if 1 - np.dot(coeff, w) > 0:
                result = result - coeff
        return result


def main():
    data = pd.read_csv(sys.stdin)
    normalize_data(data)

    best_w = np.ones(len(FEATURES))
    best_mean = np.mean(compute_ndcg_for_w(data, best_w))

    loss = RankingSVMLoss(data, lam=1e-3)
    grad = lambda w: loss.gradient(w)

    num_steps = 1000
    for i in range(num_steps):
        if ((i * 100) % num_steps == 0):
            print((i * 100) // num_steps, '%')
        w_init = np.random.random(len(FEATURES))
        w = gradient_descent(w_init, grad, eps=0.01, rate=0.01)
        mean = np.mean(compute_ndcg_for_w(data, w))
        if mean > best_mean:
            best_mean, best_w = mean, w
            print(best_mean)

    ndcg = compute_ndcg_for_w(data, best_w)
    print(np.mean(ndcg), np.std(ndcg), best_w)

if __name__ == "__main__":
    main()

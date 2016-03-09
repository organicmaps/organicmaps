#!/usr/bin/env python3
from math import exp, log
import collections
import numpy as np
import pandas as pd
import sys

DISTANCE_WINDOW = 1e9
MAX_RANK = 256
RELEVANCES = {'Irrelevant': 0, 'Relevant': 1, 'Vital': 3}
NAME_SCORES = ['Zero', 'Substring Prefix', 'Substring', 'Full Match Prefix', 'Full Match']
SEARCH_TYPES = ['POI', 'BUILDING', 'STREET', 'UNCLASSIFIED', 'VILLAGE', 'CITY', 'STATE', 'COUNTRY']

def transform_distance(distance):
    return exp(-distance / DISTANCE_WINDOW)

def transform_rank(rank):
    return rank / MAX_RANK

def transform_relevance(score):
    return RELEVANCES[score]

def transform_name_score(score):
    return NAME_SCORES.index(score) / len(NAME_SCORES)

def transform_search_type(type):
    return SEARCH_TYPES.index(type) / len(SEARCH_TYPES)

# This function may use any fields of row to compute score except
# 'Relevance' and 'SampleId'.
#
# TODO (@y, @m): learn a linear model here or find good coeffs by
# brute-force.
def get_score(row):
    x = row[['MinDistance', 'Rank', 'SearchType', 'NameScore']]
    w = np.array([1, 1, 1, 1])
    return np.dot(x, w)

def normalize_data(data):
    data['DistanceToViewport'] = data['DistanceToViewport'].apply(transform_distance)
    data['DistanceToPosition'] = data['DistanceToPosition'].apply(transform_distance)
    data['Rank'] = data['Rank'].apply(transform_rank)
    data['NameScore'] = data['NameScore'].apply(transform_name_score)
    data['SearchType'] = data['SearchType'].apply(transform_search_type)
    data['Relevance'] = data['Relevance'].apply(transform_relevance)

    # Adds some new columns to the data frame.
    data['MinDistance'] = pd.Series(np.minimum(data['DistanceToViewport'], data['DistanceToPosition']))
    data['Score'] = pd.Series([get_score(data.ix[i]) for i in data.index])

def compute_ndcg(scores):
    scores_summary = collections.defaultdict(int)

    dcg = 0
    for i, score in enumerate(scores):
        dcg += score / log(2 + i, 2)
        scores_summary[score] += 1

    dcg_norm, i = 0, 0
    for score in sorted(scores_summary.keys(), reverse=True):
        for j in range(scores_summary[score]):
            dcg_norm += score / log(2 + i, 2)
            i += 1

    if dcg_norm == 0:
        return 0
    return dcg / dcg_norm

def main():
    data = pd.read_csv(sys.stdin)
    normalize_data(data)
    grouped = data.groupby(data['SampleId'], sort=False).groups

    ndcgs = []
    for id in grouped:
        indices = grouped[id]
        group = data.ix[indices]
        sorted_group = group.sort_values('Score', ascending=False)
        ndcgs.append(compute_ndcg(sorted_group['Relevance']))

    ndcgs = np.array(ndcgs)
    print('NDCG mean: {}, std: {}'.format(np.mean(ndcgs), np.std(ndcgs)))

if __name__ == "__main__":
    main()

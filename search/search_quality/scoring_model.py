from math import exp, log
import numpy as np
import pandas as pd

DISTANCE_WINDOW = 1e9
MAX_RANK = 256
RELEVANCES = ['Irrelevant', 'Relevant', 'Vital']
NAME_SCORES = ['Zero', 'Substring Prefix', 'Substring', 'Full Match Prefix', 'Full Match']
SEARCH_TYPES = ['POI', 'BUILDING', 'STREET', 'UNCLASSIFIED', 'VILLAGE', 'CITY', 'STATE', 'COUNTRY']

def transform_distance(distance):
    return exp(-distance / DISTANCE_WINDOW)

def transform_rank(rank):
    return rank / MAX_RANK

def transform_relevance(score):
    return RELEVANCES.index(score)

def transform_name_score(score):
    return NAME_SCORES.index(score)

def transform_search_type(type):
    return SEARCH_TYPES.index(type)

# This function may use any fields of row to compute score except
# 'Relevance' and 'SampleId'.
def get_score(row):
    # TODO (@y, @m): learn a linear model here or find good coeffs by
    # brute-force.
    return row['MinDistance'] + \
        row['Rank'] + \
        (row['SearchType'] / len(SEARCH_TYPES)) + \
        (row['NameScore'] / len(NAME_SCORES))

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
    scores_summary = {}

    dcg, i = 0, 0
    for score in scores:
        dcg = dcg + score / log(2 + i)
        if score in scores_summary:
            scores_summary[score] = scores_summary[score] + 1;
        else:
            scores_summary[score] = 1
        i = i + 1

    dcg_norm, i = 0, 0
    for score in sorted(scores_summary.keys(), reverse=True):
        for j in range(scores_summary[score]):
            dcg_norm = dcg_norm + score / log(2 + i)
            i = i + 1

    if dcg_norm == 0:
        return 0
    return dcg / dcg_norm

data = pd.read_csv('queries.csv')
normalize_data(data)
grouped = data.groupby(data['SampleId'], sort=False).groups

ndcgs = []
for id in grouped:
    indices = grouped[id]
    group = data.ix[indices]
    sorted_group = group.sort_values('Score', ascending=False)
    ndcgs.append(compute_ndcg(sorted_group['Relevance']))

ndcgs = np.array(ndcgs)
print('NDCG mean:', np.mean(ndcgs), 'std:', np.std(ndcgs))

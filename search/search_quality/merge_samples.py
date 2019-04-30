import argparse
import copy
import json


def read_samples(path):
    with open(path, "r") as f:
        return [json.loads(line) for line in f]


def doubles_equal(a, b):
    EPS = 1e-6
    return abs(a - b) < EPS


def points_equal(a, b):
    return doubles_equal(a["x"], b["x"]) and doubles_equal(a["y"], b["y"])


def rects_equal(a, b):
    for key in ["minx", "miny", "maxx", "maxy"]:
        if not doubles_equal(a[key], b[key]):
            return False
    return True


def samples_equal(a, b):
    """
    Returns whether two samples originate from the same unassessed sample,
    i.e. found results and uselessness are not taken into account.
    """
    for field in ["query", "locale"]:
        if a[field] != b[field]:
            return False

    pos_key = "position"
    bad_point = {"x": -1, "y": -1}
    a_pos = a[pos_key] or bad_point
    b_pos = b[pos_key] or bad_point
    if not points_equal(a_pos, b_pos):
        return False

    vp_key = "viewport"
    if not rects_equal(a[vp_key], b[vp_key]):
        return False

    return True


def results_equal(a, b):
    """
    Returns whether two found results are equal before assessing.
    Does not take relevance into account.
    """
    pos_key = "position"
    name_key = "name"
    hn_key = "houseNumber"
    types_key = "types"
    if a[name_key] != b[name_key]:
        return False
    if not points_equal(a[pos_key], b[pos_key]):
        return False
    if a[hn_key] != b[hn_key]:
        return False
    if sorted(a[types_key]) != sorted(b[types_key]):
        return False
    return True


def greedily_match_results(a, b):
    match_to_a = [-1] * len(a)
    match_to_b = [-1] * len(b)
    for i in range(len(a)):
        if match_to_a[i] >= 0:
            continue
        for j in range(len(b)):
            if match_to_b[j] >= 0:
                continue
            if results_equal(a[i], b[j]):
                match_to_a[i] = j
                match_to_b[j] = i
                break
    return match_to_a, match_to_b


def merge_relevancies(a, b):
    """
    The second element of the returned tuple is True
    iff the relevancies are compatible and can be merged.
    Two relevancies are incompatible if one of them is a
    "strong" one, i.e. either "harmful" or "vital", and
    the other is "weak" and has the opposite sign ("relevant"
    and "irrelevant" respectively).

    If a and b are compatible, then the first element of the tuple is
    the resulting relevance. The less intuitive merge results are:
      strong, weak -> strong
      relevant, irrelevant -> relevant
    """
    RELEVANCIES = ["harmful", "irrelevant", "relevant", "vital"]
    id_a = RELEVANCIES.index(a)
    id_b = RELEVANCIES.index(b)
    if id_a > id_b:
        a, b = b, a
        id_a, id_b = id_b, id_a
    if id_a == 0 and id_b <= 1:
        return a, True
    if id_a == 1 and id_b <= 2:
        return b, True
    if id_a > 1:
        return b, True
    return a, False


def merge_two_samples(a, b, line_number):
    if not samples_equal(a, b):
        raise Exception(f"Tried to merge two non-equivalent samples, line {line_number}")

    useless_key = "useless"
    useless_a = a.get(useless_key, False)
    useless_b = b.get(useless_key, False)

    if useless_a != useless_b:
        print(line_number, "useless:", useless_a, useless_b)

    if useless_a and not useless_b:
        return b
    if useless_b:
        return a

    # Both are not useless.

    res_key = "results"
    match_to_a, match_to_b = greedily_match_results(a[res_key], b[res_key])

    lst = [x for x in match_to_a if x < 0]

    rel_key = "relevancy"

    c = copy.deepcopy(a)
    c[res_key] = []

    for i, j in enumerate(match_to_a):
        if j < 0:
            continue
        res = a[res_key][i]
        ra = a[res_key][i][rel_key]
        rb = b[res_key][j][rel_key]
        rc, ok = merge_relevancies(ra, rb)
        res[rel_key] = rc
        if not ok:
            print(line_number, ra, rb, a[res_key][i]["name"])
            continue
        c[res_key].append(res)

    # Add all unmatched results as is.
    def add_unmatched(x, match_to_x):
        c[res_key].extend(
            x[res_key][i]
            for i in range(len(match_to_x))
            if match_to_x[i] < 0
        )

    add_unmatched(a, match_to_a)
    add_unmatched(b, match_to_b)

    return c


def merge_two_files(path0, path1, path_out):
    """
    Merges two .jsonl files. The files must contain the same number
    of samples, one JSON-encoded sample per line. The first sample
    of the first file will be merged with the first sample of the second
    file, etc.
    """
    a = read_samples(path0)
    b = read_samples(path1)
    if len(a) != len(b):
        raise Exception(f"Different sizes of samples are not supported: {len(a)} and {len(b)}")

    result = [merge_two_samples(a[i], b[i], i+1) for i in range(len(a))]

    with open(path_out, "w") as f:
        for sample in result:
            f.write(json.dumps(sample) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Utilities to merge assessed samples from different assessors")
    parser.add_argument("--input0", required=True, dest="input0", help="Path to the first input file")
    parser.add_argument("--input1", required=True, dest="input1", help="Path to the second input file")
    parser.add_argument("--output", required=True, dest="output", help="Path to the output file")
    args = parser.parse_args()

    merge_two_files(args.input0, args.input1, args.output)


if __name__ == "__main__":
    main()

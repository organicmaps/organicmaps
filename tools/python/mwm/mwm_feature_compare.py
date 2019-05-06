import argparse
import multiprocessing
import os

from .mwm import MWM

OMIM_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..")


def count_feature(mwm_path, feature_name):
    mwm = MWM(open(mwm_path, "rb"))
    mwm.read_header()
    mwm.read_types(os.path.join(OMIM_ROOT, "data", "types.txt"))
    counter = 0
    for feature in mwm.iter_features():
        if feature_name in feature["header"]["types"]:
            counter += 1
    return counter


def compare_feature_num(args_tuple):
    old_mwm, new_mwm, feature_name, threshold = args_tuple
    old_feature_count = count_feature(old_mwm, feature_name)
    new_feature_count = count_feature(new_mwm, feature_name)
    delta = new_feature_count - old_feature_count

    if delta < 0:
        p_change = float(abs(delta)) / old_feature_count * 100

        if p_change > threshold:
            print("In \"{0}\" number of \"{1}\" decreased by {2:.0f}% ({3} → {4})".format(
                os.path.basename(new_mwm), feature_name, round(p_change), old_feature_count, new_feature_count))
            return False
    return True


def compare_mwm(old_mwm_path, new_mwm_path, feature_name, threshold):
    def valid_mwm(mwm_name):
        return mwm_name.endswith(".mwm") and not mwm_name.startswith("World")

    def generate_names_dict(path):
        return dict((file_name, os.path.abspath(os.path.join(path, file_name)))
                    for file_name in os.listdir(path) if valid_mwm(file_name))

    old_mwm_list = generate_names_dict(old_mwm_path)
    new_mwm_list = generate_names_dict(new_mwm_path)

    same_mwm_names = set(new_mwm_list).intersection(set(old_mwm_list))
    args = ((old_mwm_list[mwm], new_mwm_list[mwm], feature_name, threshold) for mwm in same_mwm_names)

    pool = multiprocessing.Pool()
    return all(pool.imap(compare_feature_num, args))

import multiprocessing
import os

from mwm.find_feature import find_features


def compare_feature_num(old_mwm, new_mwm, name, threshold):
    old_count = len(find_features(old_mwm, "et", name))
    new_count = len(find_features(new_mwm, "et", name))

    delta = new_count - old_count
    if delta < 0:
        p_change = float(abs(delta)) / old_count * 100
        if p_change > threshold:
            print(
                f'In "{os.path.basename(new_mwm)}" number of "{name}" '
                f"decreased by {round(p_change)} ({old_count} → {new_count})"
            )
            return False
    return True


def compare_mwm(old_mwm_path, new_mwm_path, name, threshold):
    def generate_names(path):
        return {
            file_name: os.path.abspath(os.path.join(path, file_name))
            for file_name in os.listdir(path)
            if file_name.endswith(".mwm") and not file_name.startswith("World")
        }

    old_mwms = generate_names(old_mwm_path)
    new_mwms = generate_names(new_mwm_path)

    same_mwms = set(new_mwms) & set(new_mwms)
    args = ((old_mwms[mwm], new_mwms[mwm], name, threshold) for mwm in same_mwms)

    pool = multiprocessing.Pool()
    return all(pool.imap(compare_feature_num, args))

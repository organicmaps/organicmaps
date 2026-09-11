from collections import defaultdict
from functools import lru_cache

from maps_generator.checks import check
from mwm import Mwm
from mwm import NAME_TO_INDEX_TYPE_MAPPING


@lru_cache(maxsize=None)
def count_all_types(path: str):
    c = defaultdict(int)
    for ft in Mwm(path, parse=False):
        for t in ft.types():
            c[t] += 1
    return c


def get_mwm_types_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    """
    Returns a mwm types check set, that checks a difference in a number of
    each type between old mwms and new mwms.
    """
    cs = check.CompareCheckSet("Mwm types check")

    def make_do(index):
        return lambda path: count_all_types(path)[index]

    for t_name, t_index in NAME_TO_INDEX_TYPE_MAPPING.items():
        cs.add_check(
            check.build_check_set_for_files(
                f"Type {t_name} check",
                old_path,
                new_path,
                ext=".mwm",
                do=make_do(t_index),
            )
        )
    return cs

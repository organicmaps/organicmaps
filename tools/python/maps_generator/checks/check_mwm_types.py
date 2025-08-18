from collections import defaultdict
from functools import lru_cache
from typing import Union

from maps_generator.checks import check
from mwm import Mwm
from mwm import NAME_TO_INDEX_TYPE_MAPPING
from mwm import readable_type
from mwm import type_index


@lru_cache(maxsize=None)
def count_all_types(path: str):
    c = defaultdict(int)
    for ft in Mwm(path, parse=False):
        for t in ft.types():
            c[t] += 1
    return c


def get_mwm_type_check_set(
    old_path: str, new_path: str, type_: Union[str, int]
) -> check.CompareCheckSet:
    """
    Returns a  mwm type check set, that checks a difference in a number of
    type [type_] between old mwms and new mwms.
    """
    if isinstance(type_, str):
        type_ = type_index(type_)
    assert type_ >= 0, type_

    return check.build_check_set_for_files(
        f"Types check [{readable_type(type_)}]",
        old_path,
        new_path,
        ext=".mwm",
        do=lambda path: count_all_types(path)[type_],
    )


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

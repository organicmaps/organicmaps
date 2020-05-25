from collections import defaultdict
from functools import lru_cache
from typing import Union

from maps_generator.checks import check
from mwm import Mwm
from mwm.types import NAME_TO_INDEX_TYPE_MAPPING
from mwm.types import readable_type
from mwm.types import type_index


@lru_cache(maxsize=None)
def count_all_types(path: str):
    c = defaultdict(int)
    for ft in Mwm(path, parse=False):
        for t in ft.types():
            c[t] += 1
    return c


def get_mwm_type_check_set(
    old_path: str, new_path: str, _type: Union[str, int]
) -> check.CompareCheckSet:
    if isinstance(_type, str):
        _type = type_index(_type)
    assert _type >= 0, _type

    return check.build_check_set_for_files(
        f"Types check [{readable_type(_type)}]",
        old_path,
        new_path,
        ext=".mwm",
        do=lambda path: count_all_types(path)[_type],
    )


def get_mwm_all_types_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    cs = check.CompareCheckSet("Sections categories check")

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

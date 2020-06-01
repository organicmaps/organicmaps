import sys
from collections import namedtuple
from enum import Enum
from typing import Callable
from typing import Mapping

from maps_generator.checks import check
from maps_generator.checks.check_addresses import get_addresses_check_set
from maps_generator.checks.check_categories import get_categories_check_set
from maps_generator.checks.check_log_levels import get_log_levels_check_set
from maps_generator.checks.check_mwm_types import get_all_mwm_types_check_set
from maps_generator.checks.check_mwm_types import get_mwm_type_check_set
from maps_generator.checks.check_sections import get_sections_existence_check_set
from maps_generator.checks.check_sections import get_sections_size_check_set
from maps_generator.checks.check_size import get_size_check_set


class CheckType(Enum):
    low = 1
    medium = 2
    hard = 3
    strict = 4


Threshold = namedtuple("Threshold", ["abs", "rel"])

_check_type_map = {
    CheckType.low: Threshold(abs=20, rel=20),
    CheckType.medium: Threshold(abs=15, rel=15),
    CheckType.hard: Threshold(abs=10, rel=10),
    CheckType.strict: Threshold(abs=0, rel=0),
}


def set_threshold(check_type_map: Mapping[CheckType, Threshold]):
    global _check_type_map
    _check_type_map = check_type_map


def make_default_filter(check_type_map: Mapping[CheckType, Threshold] = None):
    if check_type_map is None:
        check_type_map = _check_type_map

    def maker(check_type: CheckType):
        threshold = check_type_map[check_type]

        def default_filter(r: check.ResLine):
            return (
                check.norm(r.diff) > threshold.abs and check.get_rel(r) > threshold.rel
            )

        return default_filter

    return maker


def get_mwm_check_sets_and_filters(
    old_path: str, new_path: str, categories_path: str
) -> Mapping[check.Check, Callable]:
    check_type_map_size = {
        CheckType.low: Threshold(abs=20, rel=20000),
        CheckType.medium: Threshold(abs=15, rel=15000),
        CheckType.hard: Threshold(abs=10, rel=1000),
        CheckType.strict: Threshold(abs=0, rel=0),
    }

    return {
        get_categories_check_set(
            old_path, new_path, categories_path
        ): make_default_filter(),
        get_mwm_type_check_set(
            old_path, new_path, "sponsored-booking"
        ): make_default_filter(),
        get_all_mwm_types_check_set(old_path, new_path): make_default_filter(),
        get_size_check_set(old_path, new_path): make_default_filter(
            check_type_map_size
        ),
        get_sections_size_check_set(old_path, new_path): make_default_filter(
            check_type_map_size
        ),
        get_sections_existence_check_set(old_path, new_path): None,
    }


def get_logs_check_sets_and_filters(
    old_path: str, new_path: str
) -> Mapping[check.Check, Callable]:
    return {
        get_addresses_check_set(old_path, new_path): make_default_filter(),
        get_log_levels_check_set(old_path, new_path): None,
    }


def _print_header(file, header, width=100, s="="):
    stars = s * ((width - len(header)) // 2)
    rstars = stars
    if 2 * len(stars) + len(header) < width:
        rstars += s
    print(stars, header, rstars, file=file)


def run_checks_and_print_results(
    checks: Mapping[check.Check, Callable],
    check_type: CheckType,
    silent_if_no_results: bool = True,
    file=sys.stdout,
):
    for check, make_filt in checks.items():
        check.check()
        _print_header(file, check.name)
        check.print(
            silent_if_no_results=silent_if_no_results,
            filt=None if make_filt is None else make_filt(check_type),
            file=file,
        )

import sys
from collections import namedtuple
from enum import Enum
from typing import Callable
from typing import Mapping
from typing import Optional
from typing import Set
from typing import Tuple

from maps_generator.checks import check
from maps_generator.checks.check_addresses import get_addresses_check_set
from maps_generator.checks.check_categories import get_categories_check_set
from maps_generator.checks.check_log_levels import get_log_levels_check_set
from maps_generator.checks.check_mwm_types import get_mwm_type_check_set
from maps_generator.checks.check_mwm_types import get_mwm_types_check_set
from maps_generator.checks.check_sections import get_sections_existence_check_set
from maps_generator.checks.check_sections import get_sections_size_check_set
from maps_generator.checks.check_size import get_size_check_set


class CheckType(Enum):
    low = 1
    medium = 2
    hard = 3
    strict = 4


Threshold = namedtuple("Threshold", ["abs", "rel"])

_default_thresholds = {
    CheckType.low: Threshold(abs=20, rel=20),
    CheckType.medium: Threshold(abs=15, rel=15),
    CheckType.hard: Threshold(abs=10, rel=10),
    CheckType.strict: Threshold(abs=0, rel=0),
}


def set_thresholds(check_type_map: Mapping[CheckType, Threshold]):
    global _default_thresholds
    _default_thresholds = check_type_map


def make_tmap(
    low: Optional[Tuple[float, float]] = None,
    medium: Optional[Tuple[float, float]] = None,
    hard: Optional[Tuple[float, float]] = None,
    strict: Optional[Tuple[float, float]] = None,
):
    thresholds = _default_thresholds.copy()
    if low is not None:
        thresholds[CheckType.low] = Threshold(*low)
    if medium is not None:
        thresholds[CheckType.medium] = Threshold(*medium)
    if hard is not None:
        thresholds[CheckType.hard] = Threshold(*hard)
    if strict is not None:
        thresholds[CheckType.strict] = Threshold(*strict)
    return thresholds


def make_default_filter(check_type_map: Mapping[CheckType, Threshold] = None):
    if check_type_map is None:
        check_type_map = _default_thresholds

    def maker(check_type: CheckType):
        threshold = check_type_map[check_type]

        def default_filter(r: check.ResLine):
            return (
                check.norm(r.diff) > threshold.abs and check.get_rel(r) > threshold.rel
            )

        return default_filter

    return maker


class MwmsChecks(Enum):
    sections_existence = 1
    sections_size = 2
    mwm_size = 3
    types = 4
    booking = 5
    categories = 6


def get_mwm_check_sets_and_filters(
    old_path: str, new_path: str, checks: Set[MwmsChecks] = None, **kwargs
) -> Mapping[check.Check, Callable]:
    def need_add(t: MwmsChecks):
        return checks is None or t in checks

    m = {get_sections_existence_check_set(old_path, new_path): None}

    if need_add(MwmsChecks.sections_size):
        c = get_sections_size_check_set(old_path, new_path)
        thresholds = make_tmap(low=(0, 20), medium=(0, 10), hard=(0, 5))
        m[c] = make_default_filter(thresholds)

    mb = 1 << 20

    if need_add(MwmsChecks.mwm_size):
        c = get_size_check_set(old_path, new_path)
        thresholds = make_tmap(low=(2 * mb, 10), medium=(mb, 5), hard=(0.5 * mb, 2))
        m[c] = make_default_filter(thresholds)

    if need_add(MwmsChecks.types):
        c = get_mwm_types_check_set(old_path, new_path)
        thresholds = make_tmap(low=(500, 30), medium=(100, 20), hard=(100, 10))
        m[c] = make_default_filter(thresholds)

    if need_add(MwmsChecks.booking):
        c = get_mwm_type_check_set(old_path, new_path, "sponsored-booking")
        thresholds = make_tmap(low=(500, 20), medium=(50, 10), hard=(50, 5))
        m[c] = make_default_filter(thresholds)

    if need_add(MwmsChecks.categories):
        c = get_categories_check_set(old_path, new_path, kwargs["categories_path"])
        thresholds = make_tmap(low=(200, 20), medium=(50, 10), hard=(50, 5))
        m[c] = make_default_filter(thresholds)

    return m


class LogsChecks(Enum):
    log_levels = 1
    addresses = 2


def get_logs_check_sets_and_filters(
    old_path: str, new_path: str, checks: Set[LogsChecks] = None
) -> Mapping[check.Check, Callable]:
    def need_add(t: LogsChecks):
        return checks is None or t in checks

    m = {get_log_levels_check_set(old_path, new_path): None}

    if need_add(LogsChecks.addresses):
        c = get_addresses_check_set(old_path, new_path)
        thresholds = make_tmap(low=(50, 20), medium=(20, 10), hard=(10, 5))
        m[c] = make_default_filter(thresholds)

    return m


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

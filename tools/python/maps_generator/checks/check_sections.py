import os
from functools import lru_cache

from maps_generator.checks import check
from mwm import Mwm


class SectionNames:
    def __init__(self, sections):
        self.sections = sections

    def __sub__(self, other):
        return SectionNames(
            {k: self.sections[k] for k in set(self.sections) - set(other.sections)}
        )

    def __lt__(self, other):
        if isinstance(other, int):
            return len(self.sections) < other
        elif isinstance(other, SectionNames):
            return self.sections < other.sections
        assert False, type(other)

    def __gt__(self, other):
        if isinstance(other, int):
            return len(self.sections) > other
        elif isinstance(other, SectionNames):
            return self.sections > other.sections
        assert False, type(other)

    def __len__(self):
        return len(self.sections)

    def __str__(self):
        return str(self.sections)


@lru_cache(maxsize=None)
def read_sections(path: str):
    return Mwm(path, parse=False).sections_info()


def get_appeared_sections_check_set(
    old_path: str, new_path: str
) -> check.CompareCheckSet:
    return check.build_check_set_for_files(
        f"Appeared sections check",
        old_path,
        new_path,
        ext=".mwm",
        do=lambda path: SectionNames(read_sections(path)),
        diff_format=lambda s: ", ".join(f"{k}:{v.size}" for k, v in s.sections.items()),
        format=lambda s: f"number of sections: {len(s.sections)}",
    )


def get_disappeared_sections_check_set(
    old_path: str, new_path: str
) -> check.CompareCheckSet:
    return check.build_check_set_for_files(
        f"Disappeared sections check",
        old_path,
        new_path,
        ext=".mwm",
        do=lambda path: SectionNames(read_sections(path)),
        op=lambda previous, current: previous - current,
        diff_format=lambda s: ", ".join(f"{k}:{v.size}" for k, v in s.sections.items()),
        format=lambda s: f"number of sections: {len(s.sections)}",
    )


def get_sections_existence_check_set(
    old_path: str, new_path: str
) -> check.CompareCheckSet:
    """
    Returns a sections existence check set, that checks appeared and
    disappeared sections between old mwms and new mwms.
    """
    cs = check.CompareCheckSet("Sections existence check")
    cs.add_check(get_appeared_sections_check_set(old_path, new_path))
    cs.add_check(get_disappeared_sections_check_set(old_path, new_path))
    return cs


def _get_sections_set(path):
    sections = set()
    for file in os.listdir(path):
        p = os.path.join(path, file)
        if os.path.isfile(p) and file.endswith(".mwm"):
            sections.update(read_sections(p).keys())
    return sections


def get_sections_size_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    """
    Returns a sections size check set, that checks a difference in a size
    of each sections of mwm between old mwms and new mwms.
    """
    sections_set = _get_sections_set(old_path)
    sections_set.update(_get_sections_set(new_path))

    cs = check.CompareCheckSet("Sections size check")

    def make_do(section):
        def do(path):
            sections = read_sections(path)
            if section not in sections:
                return None

            return sections[section].size

        return do

    for section in sections_set:
        cs.add_check(
            check.build_check_set_for_files(
                f"Size of {section} check",
                old_path,
                new_path,
                ext=".mwm",
                do=make_do(section),
            )
        )
    return cs

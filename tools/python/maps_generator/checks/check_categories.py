from collections import defaultdict

from maps_generator.checks import check
from maps_generator.checks.check_mwm_types import count_all_types
from mwm import NAME_TO_INDEX_TYPE_MAPPING


def parse_groups(path):
    groups = defaultdict(set)
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                continue

            if line.startswith("@"):
                continue

            array = line.split("@", maxsplit=1)
            if len(array) == 2:
                types_str, categories = array
                types_int = {
                    NAME_TO_INDEX_TYPE_MAPPING[t]
                    for t in types_str.strip("|").split("|")
                }
                for category in categories.split("|"):
                    category = category.replace("@", "", 1)
                    groups[category].update(types_int)
        return groups


def get_categories_check_set(
    old_path: str, new_path: str, categories_path: str
) -> check.CompareCheckSet:
    """
    Returns a categories check set, that checks a difference in a number of
    objects of categories(from categories.txt) between old mwms and new mwms.
    """
    cs = check.CompareCheckSet("Categories check")

    def make_do(indexes):
        def do(path):
            all_types = count_all_types(path)
            return sum(all_types[i] for i in indexes)

        return do

    for category, types in parse_groups(categories_path).items():
        cs.add_check(
            check.build_check_set_for_files(
                f"Category {category} check",
                old_path,
                new_path,
                ext=".mwm",
                do=make_do(types),
            )
        )
    return cs

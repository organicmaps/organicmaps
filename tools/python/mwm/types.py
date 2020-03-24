import os
from typing import Dict
from typing import Tuple


def read_types_mappings() -> Tuple[Dict[int, str], Dict[str, int]]:
    resources_path = os.environ.get("MWM_RESOURCES_DIR")
    name_to_index = {}
    index_to_name = {}
    with open(os.path.join(resources_path, "types.txt")) as f:
        for i, line in enumerate(f):
            s = line.strip()
            name = s.replace("|", "-")
            if s.startswith("*"):
                name = name[1:]
                name_to_index[name] = i

            index_to_name[i] = name

    return index_to_name, name_to_index


INDEX_TO_NAME_TYPE_MAPPING, NAME_TO_INDEX_TYPE_MAPPING = read_types_mappings()


def readable_type(index: int) -> str:
    try:
        return INDEX_TO_NAME_TYPE_MAPPING[index]
    except KeyError:
        return "unknown"


def type_index(type_name: str) -> int:
    try:
        return NAME_TO_INDEX_TYPE_MAPPING[type_name]
    except KeyError:
        return -1

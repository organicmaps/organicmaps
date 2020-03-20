import os
from typing import Dict


def read_types_mappings() -> Dict[int, str]:
    resources_path = os.environ.get("MWM_RESOURCES_DIR")
    types = {}
    with open(os.path.join(resources_path, "types.txt")) as f:
        for i, line in enumerate(f):
            if line.startswith("*"):
                types[i] = line[1:].strip().replace("|", "-")

    return types


TYPES_MAPPING = read_types_mappings()


def readable_type(type: int) -> str:
    try:
        return TYPES_MAPPING[type]
    except KeyError:
        return "unknown"

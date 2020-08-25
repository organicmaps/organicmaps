import os

NAME_TO_INDEX_TYPE_MAPPING = {}
INDEX_TO_NAME_TYPE_MAPPING = {}


def init(resource_path):
    global NAME_TO_INDEX_TYPE_MAPPING
    global INDEX_TO_NAME_TYPE_MAPPING

    NAME_TO_INDEX_TYPE_MAPPING = {}
    INDEX_TO_NAME_TYPE_MAPPING = {}
    with open(os.path.join(resource_path, "types.txt")) as f:
        for i, line in enumerate(f):
            s = line.strip()
            name = s.replace("|", "-")
            if s.startswith("*"):
                name = name[1:]
                NAME_TO_INDEX_TYPE_MAPPING[name] = i

            INDEX_TO_NAME_TYPE_MAPPING[i] = name


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

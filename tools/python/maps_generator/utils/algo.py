from re import finditer


def unique(s):
    seen = set()
    seen_add = seen.add
    return [x for x in s if not (x in seen or seen_add(x))]


def camel_case_split(identifier):
    matches = finditer(
        ".+?(?:(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])|$)", identifier
    )
    return [m.group(0) for m in matches]

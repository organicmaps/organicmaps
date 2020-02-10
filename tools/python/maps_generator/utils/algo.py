def unique(s):
    seen = set()
    seen_add = seen.add
    return [x for x in s if not (x in seen or seen_add(x))]

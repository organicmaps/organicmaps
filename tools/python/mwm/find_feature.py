import json
import os.path

from .mwm import MWM


def find_feature(path, typ, string):
    mwm = MWM(open(path, "rb"))
    mwm.read_header()
    mwm.read_types(os.path.join(os.path.dirname(__file__),
                                "..", "..", "..", "data", "types.txt"))
    for i, feature in enumerate(mwm.iter_features(metadata=True)):
        found = False
        if typ == "n" and "name" in feature["header"]:
            for value in feature["header"]["name"].values():
                if string in value:
                    found = True
        elif typ in ("t", "et"):
            for t in feature["header"]["types"]:
                if t == string:
                    found = True
                elif typ == "t" and string in t:
                    found = True
        elif typ == "m" and "metadata" in feature:
            if string in feature["metadata"]:
                found = True
        elif typ == "id" and i == int(string):
            found = True
        if found:
            print(json.dumps(feature, ensure_ascii=False,
                             sort_keys=True).encode("utf-8"))

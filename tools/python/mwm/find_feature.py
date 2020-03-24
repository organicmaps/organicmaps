import json
from typing import List

from mwm import EnumAsStrEncoder
from mwm import Feature
from mwm import Mwm
from mwm import readable_type


def find_features(path: str, typ: str, string: str) -> List[Feature]:
    features = []
    index = int(string) if typ == "id" else None
    for feature in Mwm(path):
        found = False
        if typ == "n":
            for value in feature.names().values():
                if string in value:
                    found = True
                    break
        elif typ in ("t", "et"):
            for t in feature.types():
                readable_type_ = readable_type(t)
                if readable_type_ == string:
                    found = True
                    break
                elif typ == "t" and string in readable_type_:
                    found = True
                    break
        elif typ == "m":
            for f in feature.metadata():
                if string in f.name:
                    found = True
                    break
        elif typ == "id" and index == feature.index():
            found = True

        if found:
            features.append(feature)

    return features


def find_and_print_features(path: str, typ: str, string: str):
    for feature in find_features(path, typ, string):
        print(
            json.dumps(
                feature.to_json(),
                ensure_ascii=False,
                sort_keys=True,
                cls=EnumAsStrEncoder,
            )
        )

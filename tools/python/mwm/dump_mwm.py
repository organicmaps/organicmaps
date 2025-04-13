import json

from mwm import EnumAsStrEncoder
from mwm import Mwm


def dump_mwm(path, format, need_features):
    mwm = Mwm(path)
    if format == "str":
        print(mwm)
    elif format == "json":
        print(json.dumps(mwm.to_json(), ensure_ascii=False, cls=EnumAsStrEncoder))

    if need_features:
        for ft in mwm:
            if format == "str":
                print(ft)
            elif format == "json":
                print(
                    json.dumps(ft.to_json(), ensure_ascii=False, cls=EnumAsStrEncoder)
                )

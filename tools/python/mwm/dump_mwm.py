import json
import os.path
import sys

from .mwm import MWM


def dump_mwm(path, format):
    mwm = MWM(open(path, "rb"))
    mwm.read_types(os.path.join(os.path.dirname(sys.argv[0]),
                                "..", "..", "..", "data", "types.txt"))
    header = mwm.read_header()

    if format == "meta" or format == "tags":
        print("Tags:")
        tvv = sorted([(k, v[0], v[1]) for k, v in mwm.tags.items()], key=lambda x: x[1])
        for tv in tvv:
            print("  {0:<8}: offs {1:9} len {2:8}".format(tv[0], tv[1], tv[2]))

    if format == "meta":
        v = mwm.read_version()
        print("Format: {0}, version: {1}".format(v["fmt"], v["date"].strftime("%Y-%m-%d %H:%M")))
        print("Header: {0}".format(header))
        print("Region Info: {0}".format(mwm.read_region_info()))
        print("Metadata count: {0}".format(len(mwm.read_metadata())))
        print("Feature count: {0}".format(len(list(mwm.iter_features()))))
        cross = mwm.read_crossmwm()
        if cross:
            print("Outgoing points: {0}, incoming: {1}".format(len(cross["out"]), len(cross["in"])))
            print("Outgoing regions: {0}".format(set(cross["neighbours"])))
    elif format == "features":
        fts = list(mwm.iter_features())
        print("Features:")
        for ft in fts:
            print(json.dumps(ft, ensure_ascii=False))

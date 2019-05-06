from . import mwm


def ft2osm(path, ftid):
    with open(path, "rb") as f:
        ft2osm = mwm.read_osm2ft(f, ft2osm=True)

    type_abbr = {"n": "node", "w": "way", "r": "relation"}
    ftid = int(ftid)
    if ftid in ft2osm:
        return f"https://www.openstreetmap.org/{type_abbr[ft2osm[ftid][0]]}/{ft2osm[ftid][1]}"
    return None

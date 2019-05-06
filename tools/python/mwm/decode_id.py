import re

from . import mwm


def decode_id(id):
    if id.isdigit():
        osm_id = mwm.unpack_osmid(int(id))
        type_abbr = {"n": "node", "w": "way", "r": "relation"}
        return f"https://www.openstreetmap.org/{type_abbr[osm_id[0]]}/{osm_id[1]}"
    else:
        m = re.search(r"/(node|way|relation)/(\d+)", id)
        if m:
            oid = int(m.group(2))
            if m.group(1) == "node":
                oid |= mwm.OsmIdCode.NODE
            elif m.group(1) == "way":
                oid |= mwm.OsmIdCode.WAY
            elif m.group(1) == "relation":
                oid |= mwm.OsmIdCode.RELATION
            return oid
        else:
            return None

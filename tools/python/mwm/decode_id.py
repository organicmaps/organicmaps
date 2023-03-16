import re

from mwm.ft2osm import OsmIdCode
from mwm.ft2osm import unpack_osmid


def decode_id(id):
    if id.isdigit():
        osm_id = unpack_osmid(int(id))
        type_abbr = {"n": "node", "w": "way", "r": "relation"}
        return f"https://www.openstreetmap.org/{type_abbr[osm_id[0]]}/{osm_id[1]}"
    else:
        m = re.search(r"/(node|way|relation)/(\d+)", id)
        if m:
            type_name = m.group(1)
            oid = int(m.group(2))
            if type_name == "node":
                oid |= OsmIdCode.NODE
            elif type_name == "way":
                oid |= OsmIdCode.WAY
            elif type_name == "relation":
                oid |= OsmIdCode.RELATION
            return oid
        else:
            return None

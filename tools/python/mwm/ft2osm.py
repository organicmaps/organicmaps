from mwm.mwm_python import read_uint
from mwm.mwm_python import read_varuint


class OsmIdCode:
    # We use here obsolete types. If we change this types to new types,
    # we must support it here. See base/geo_object_id.hpp.
    NODE = 0x4000000000000000
    WAY = 0x8000000000000000
    RELATION = 0xC000000000000000
    FULL_MASK = NODE | WAY | RELATION
    RESET = ~FULL_MASK

    @staticmethod
    def is_node(code):
        return code & OsmIdCode.FULL_MASK == OsmIdCode.NODE

    @staticmethod
    def is_way(code):
        return code & OsmIdCode.FULL_MASK == OsmIdCode.WAY

    @staticmethod
    def is_relation(code):
        return code & OsmIdCode.FULL_MASK == OsmIdCode.RELATION

    @staticmethod
    def get_type(code):
        if OsmIdCode.is_relation(code):
            return "r"
        elif OsmIdCode.is_node(code):
            return "n"
        elif OsmIdCode.is_way(code):
            return "w"
        return None

    @staticmethod
    def get_id(code):
        return code & OsmIdCode.RESET


def unpack_osmid(num):
    typ = OsmIdCode.get_type(num)
    if typ is None:
        return None
    return typ, OsmIdCode.get_id(num)


def _read_osm2ft_v0(f, ft2osm, tuples):
    count = read_varuint(f)
    result = {}
    for i in range(count):
        osmid = read_uint(f, 8)
        if tuples:
            osmid = unpack_osmid(osmid)
        fid = read_uint(f, 4)
        read_uint(f, 4)  # filler
        if osmid is not None:
            if ft2osm:
                result[fid] = osmid
            else:
                result[osmid] = fid
    return result


def _read_osm2ft_v1(f, ft2osm, tuples):
    count = read_varuint(f)
    result = {}
    for i in range(count):
        osmid = read_uint(f, 8)
        # V1 use complex ids. Here we want to skip second part of complex id
        # to save old interface osm2ft.
        read_uint(f, 8)
        if tuples:
            osmid = unpack_osmid(osmid)
        fid = read_uint(f, 4)
        read_uint(f, 4)  # filler
        if osmid is not None:
            if ft2osm:
                result[fid] = osmid
            else:
                result[osmid] = fid
    return result


def read_osm2ft(f, ft2osm=False, tuples=True):
    """Reads mwm.osm2ft file, returning a dict of feature id <-> osm id."""
    header = read_uint(f, 4)
    is_new_format = header == 0xFFFFFFFF
    if is_new_format:
        version = read_uint(f, 1)
        if version == 1:
            return _read_osm2ft_v1(f, ft2osm, tuples)
        else:
            raise Exception("Format {0} is not supported".format(version))
    else:
        f.seek(0)
        return _read_osm2ft_v0(f, ft2osm, tuples)


def ft2osm(path, ftid):
    with open(path, "rb") as f:
        ft2osm = read_osm2ft(f, ft2osm=True)

    type_abbr = {"n": "node", "w": "way", "r": "relation"}
    ftid = int(ftid)
    if ftid in ft2osm:
        return f"https://www.openstreetmap.org/{type_abbr[ft2osm[ftid][0]]}/{ft2osm[ftid][1]}"
    return None

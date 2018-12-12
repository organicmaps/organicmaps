# MWM Reader Module
import struct
import math
from datetime import datetime

# Unprocessed sections: geomN, trgN, idx, sdx (search index), addr (search address), offs (feature offsets - succinct)
# Routing sections: mercedes (matrix), daewoo (edge data), infinity (edge id), skoda (shortcuts), chrysler (cross context), ftseg, node2ftseg
# (these mostly are succinct structures, except chrysler and node2ftseg, so no use trying to load them here)

# TODO:
# - Predictive reading of LineStrings
# - Find why polygon geometry is incorrect in iter_features()
# - Find feature ids in the 'dat' section, or find a way to read the 'offs' section


class OsmIdCode:
    NODE = 0x4000000000000000
    WAY = 0x8000000000000000
    RELATION = 0xC000000000000000
    RESET = ~(NODE | WAY | RELATION)

    @staticmethod
    def is_node(code):
        return code & OsmIdCode.NODE == OsmIdCode.NODE

    @staticmethod
    def is_way(code):
        return code & OsmIdCode.WAY == OsmIdCode.WAY

    @staticmethod
    def is_relation(code):
        return code & OsmIdCode.RELATION == OsmIdCode.RELATION

    @staticmethod
    def get_type(code):
        if OsmIdCode.is_relation(code):
            return 'r'
        elif OsmIdCode.is_node(code):
            return 'n'
        elif OsmIdCode.is_way(code):
            return 'w'
        return None

    @staticmethod
    def get_id(code):
        return code & OsmIdCode.RESET


class MWM:
    # coding/string_utf8_multilang.cpp
    languages = ["default",
                 "en", "ja", "fr", "ko_rm", "ar", "de", "int_name", "ru", "sv", "zh", "fi", "be", "ka", "ko",
                 "he", "nl", "ga", "ja_rm", "el", "it", "es", "zh_pinyin", "th", "cy", "sr", "uk", "ca", "hu",
                 "hsb", "eu", "fa", "br", "pl", "hy", "kn", "sl", "ro", "sq", "am", "fy", "cs", "gd", "sk",
                 "af", "ja_kana", "lb", "pt", "hr", "fur", "vi", "tr", "bg", "eo", "lt", "la", "kk", "gsw",
                 "et", "ku", "mn", "mk", "lv", "hi"]

    # indexer/feature_meta.hpp
    metadata = ["0",
                "cuisine", "open_hours", "phone_number", "fax_number", "stars",
                "operator", "url", "website", "internet", "ele",
                "turn_lanes", "turn_lanes_forward", "turn_lanes_backward", "email", "postcode",
                "wikipedia", "maxspeed", "flats", "height", "min_height",
                "denomination", "building_levels", "test_id", "ref:sponsored", "price_rate",
                "rating", "banner_url", "level", "iata", "brand"]

    regiondata = ["languages", "driving", "timezone", "addr_fmt", "phone_fmt", "postcode_fmt", "holidays", "housenames"]

    def __init__(self, f):
        self.f = f
        self.coord_size = None
        self.base_point = (0, 0)
        self.read_info()
        self.type_mapping = []

    def read_types(self, filename):
        with open(filename, 'r') as ft:
            for line in ft:
                if len(line.strip()) > 0:
                    self.type_mapping.append(line.strip().replace('|', '-'))

    def read_info(self):
        self.f.seek(0)
        self.f.seek(self.read_uint(8))
        cnt = self.read_varuint()
        self.tags = {}
        for i in range(cnt):
            name = self.read_string(plain=True)
            offset = self.read_varuint()
            length = self.read_varuint()
            self.tags[name] = (offset, length)

    def has_tag(self, tag):
        return tag in self.tags and self.tags[tag][1] > 0

    def seek_tag(self, tag):
        self.f.seek(self.tags[tag][0])

    def tag_offset(self, tag):
        return self.f.tell() - self.tags[tag][0]

    def inside_tag(self, tag):
        pos = self.tag_offset(tag)
        return pos >= 0 and pos < self.tags[tag][1]

    def read_version(self):
        """Reads 'version' section."""
        self.seek_tag('version')
        self.f.read(4)  # skip prolog
        fmt = self.read_varuint() + 1
        version = self.read_varuint()
        if version < 161231:
            vdate = datetime(2000 + int(version / 10000), int(version / 100) % 100, version % 100)
        else:
            vdate = datetime.fromtimestamp(version)
            version = int(vdate.strftime('%y%m%d'))
        return {'fmt': fmt, 'version': version, 'date': vdate}

    def read_header(self):
        """Reads 'header' section."""
        if not self.has_tag('header'):
            # Stub for routing files
            self.coord_size = (1 << 30) - 1
            return {}
        self.seek_tag('header')
        result = {}
        coord_bits = self.read_varuint()
        self.coord_size = (1 << coord_bits) - 1
        self.base_point = mwm_bitwise_split(self.read_varuint())
        result['basePoint'] = self.to_4326(self.base_point)
        result['bounds'] = self.read_bounds()
        result['scales'] = self.read_uint_array()
        langs = self.read_uint_array()
        for i in range(len(langs)):
            if i < len(self.languages):
                langs[i] = self.languages[langs[i]]
        result['langs'] = langs
        map_type = self.read_varint()
        if map_type == 0:
            result['mapType'] = 'world'
        elif map_type == 1:
            result['mapType'] = 'worldcoasts'
        elif map_type == 2:
            result['mapType'] = 'country'
        else:
            result['mapType'] = 'unknown: {0}'.format(map_type)
        return result

    # COMPLEX READERS

    def read_region_info(self):
        if not self.has_tag('rgninfo'):
            return {}
        fields = {}
        self.seek_tag('rgninfo')
        sz = self.read_varuint()
        if sz:
            for i in range(sz):
                t = self.read_varuint()
                t = self.regiondata[t] if t < len(self.regiondata) else str(t)
                fields[t] = self.read_string()
                if t == 'languages':
                    fields[t] = [self.languages[ord(x)] for x in fields[t]]
        return fields

    def read_metadata(self):
        """Reads 'meta' and 'metaidx' sections."""
        if not self.has_tag('metaidx'):
            return {}
        # Metadata format is different since v8
        fmt = self.read_version()['fmt']
        # First, read metaidx, to match featureId <-> metadata
        self.seek_tag('metaidx')
        ftid_meta = []
        while self.inside_tag('metaidx'):
            ftid = self.read_uint(4)
            moffs = self.read_uint(4)
            ftid_meta.append((moffs, ftid))
        # Sort ftid_meta array
        ftid_meta.sort(key=lambda x: x[0])
        ftpos = 0
        # Now read metadata
        self.seek_tag('meta')
        metadatar = {}
        while self.inside_tag('meta'):
            tag_pos = self.tag_offset('meta')
            fields = {}
            if fmt >= 8:
                sz = self.read_varuint()
                if sz:
                    for i in range(sz):
                        t = self.read_varuint()
                        t = self.metadata[t] if t < len(self.metadata) else str(t)
                        fields[t] = self.read_string()
                        if t == 'fuel':
                            fields[t] = fields[t].split('\x01')
            else:
                while True:
                    t = self.read_uint(1)
                    is_last = t & 0x80 > 0
                    t = t & 0x7f
                    t = self.metadata[t] if t < len(self.metadata) else str(t)
                    l = self.read_uint(1)
                    fields[t] = self.f.read(l).decode('utf-8')
                    if is_last:
                        break

            if len(fields):
                while ftpos < len(ftid_meta) and ftid_meta[ftpos][0] < tag_pos:
                    ftpos += 1
                if ftpos < len(ftid_meta):
                    if ftid_meta[ftpos][0] == tag_pos:
                        metadatar[ftid_meta[ftpos][1]] = fields
        return metadatar

    def read_crossmwm(self):
        """Reads 'chrysler' section (cross-mwm routing table)."""
        if not self.has_tag('chrysler'):
            return {}
        self.seek_tag('chrysler')
        # Ingoing nodes: array of (nodeId, coord) tuples
        incomingCount = self.read_uint(4)
        incoming = []
        for i in range(incomingCount):
            nodeId = self.read_uint(4)
            point = self.read_coord(False)
            incoming.append((nodeId, point))
        # Outgoing nodes: array of (nodeId, coord, outIndex) tuples
        # outIndex is an index in neighbours array
        outgoingCount = self.read_uint(4)
        outgoing = []
        for i in range(outgoingCount):
            nodeId = self.read_uint(4)
            point = self.read_coord(False)
            outIndex = self.read_uint(1)
            outgoing.append((nodeId, point, outIndex))
        # Adjacency matrix: costs of routes for each (incoming, outgoing) tuple
        matrix = []
        for i in range(incomingCount):
            sub = []
            for j in range(outgoingCount):
                sub.append(self.read_uint(4))
            matrix.append(sub)
        # List of mwms to which leads each outgoing node
        neighboursCount = self.read_uint(4)
        neighbours = []
        for i in range(neighboursCount):
            size = self.read_uint(4)
            neighbours.append(self.f.read(size).decode('utf-8'))
        return { 'in': incoming, 'out': outgoing, 'matrix': matrix, 'neighbours': neighbours }

    class GeomType:
        POINT = 0
        LINE = 1 << 5
        AREA = 1 << 6
        POINT_EX = 3 << 5

    def iter_features(self, metadata=False):
        """Reads 'dat' section."""
        if not self.has_tag('dat'):
            return
        # TODO: read 'offs'?
        md = {}
        if metadata:
            md = self.read_metadata()
        self.seek_tag('dat')
        ftid = -1
        while self.inside_tag('dat'):
            ftid += 1
            feature = {'id': ftid}
            feature_size = self.read_varuint()
            next_feature = self.f.tell() + feature_size
            feature['size'] = feature_size

            # Header
            header = {}
            header_bits = self.read_uint(1)
            types_count = (header_bits & 0x07) + 1
            has_name = header_bits & 0x08 > 0
            has_layer = header_bits & 0x10 > 0
            has_addinfo = header_bits & 0x80 > 0
            geom_type = header_bits & 0x60
            types = []
            for i in range(types_count):
                type_id = self.read_varuint()
                if type_id < len(self.type_mapping):
                    types.append(self.type_mapping[type_id])
                else:
                    types.append(str(type_id + 1))  # So the numbers match with mapcss-mapping.csv
            header['types'] = types
            if has_name:
                header['name'] = self.read_multilang()
            if has_layer:
                header['layer'] = self.read_uint(1)
            if has_addinfo:
                if geom_type == MWM.GeomType.POINT:
                    header['rank'] = self.read_uint(1)
                elif geom_type == MWM.GeomType.LINE:
                    header['ref'] = self.read_string()
                elif geom_type == MWM.GeomType.AREA or geom_type == MWM.GeomType.POINT_EX:
                    header['house'] = self.read_numeric_string()
            feature['header'] = header

            # Metadata
            if ftid in md:
                feature['metadata'] = md[ftid]

            # Geometry
            geometry = {}
            if geom_type == MWM.GeomType.POINT or geom_type == MWM.GeomType.POINT_EX:
                geometry['type'] = 'Point'
            elif geom_type == MWM.GeomType.LINE:
                geometry['type'] = 'LineString'
            elif geom_type == MWM.GeomType.AREA:
                geometry['type'] = 'Polygon'
            if geom_type == MWM.GeomType.POINT:
                geometry['coordinates'] = list(self.read_coord())

            # (flipping table emoticon)
            feature['geometry'] = geometry
            if False:
                if geom_type != MWM.GeomType.POINT:
                    polygon_count = self.read_varuint()
                    polygons = []
                    for i in range(polygon_count):
                        count = self.read_varuint()
                        buf = self.f.read(count)
                        # TODO: decode
                    geometry['coordinates'] = polygons
                    feature['coastCell'] = self.read_varint()

                # OSM IDs
                count = self.read_varuint()
                osmids = []
                for i in range(count):
                    encid = self.read_uint(8)
                    osmids.append('{0}{1}'.format(
                        OsmIdCode.get_type(encid) or '',
                        OsmIdCode.get_id(encid)
                    ))
                feature['osmIds'] = osmids

            if self.f.tell() > next_feature:
                raise Exception('Feature parsing error, read too much')
            yield feature
            self.f.seek(next_feature)

    # BITWISE READERS

    def read_uint(self, bytelen=1):
        return read_uint(self.f, bytelen)

    def read_varuint(self):
        return read_varuint(self.f)

    def read_varint(self):
        return read_varint(self.f)

    def read_point(self, ref, packed=True):
        """Reads an unsigned point, returns (x, y)."""
        if packed:
            u = self.read_varuint()
        else:
            u = self.read_uint(8)
        return mwm_decode_delta(u, ref)

    def to_4326(self, point):
        """Convert a point in maps.me-mercator CS to WGS-84 (EPSG:4326)."""
        if self.coord_size is None:
            raise Exception('Call read_header() first.')
        merc_bounds = (-180.0, -180.0, 180.0, 180.0)  # Xmin, Ymin, Xmax, Ymax
        x = point[0] * (merc_bounds[2] - merc_bounds[0]) / self.coord_size + merc_bounds[0]
        y = point[1] * (merc_bounds[3] - merc_bounds[1]) / self.coord_size + merc_bounds[1]
        y = 360.0 * math.atan(math.tanh(y * math.pi / 360.0)) / math.pi
        return (x, y)

    def read_coord(self, packed=True):
        """Reads a pair of coords in degrees mercator, returns (lon, lat)."""
        point = self.read_point(self.base_point, packed)
        return self.to_4326(point)

    def read_bounds(self):
        """Reads mercator bounds, returns (min_lon, min_lat, max_lon, max_lat)."""
        rmin = mwm_bitwise_split(self.read_varint())
        rmax = mwm_bitwise_split(self.read_varint())
        pmin = self.to_4326(rmin)
        pmax = self.to_4326(rmax)
        return (pmin[0], pmin[1], pmax[0], pmax[1])

    def read_string(self, plain=False, decode=True):
        length = self.read_varuint() + (0 if plain else 1)
        s = self.f.read(length)
        return s.decode('utf-8') if decode else s

    def read_uint_array(self):
        length = self.read_varuint()
        result = []
        for i in range(length):
            result.append(self.read_varuint())
        return result

    def read_numeric_string(self):
        sz = self.read_varuint()
        if sz & 1 != 0:
            return str(sz >> 1)
        sz = (sz >> 1) + 1
        return self.f.read(sz).decode('utf-8')

    def read_multilang(self):
        def find_multilang_next(s, i):
            i += 1
            while i < len(s):
                try:
                    c = ord(s[i])
                except:
                    c = s[i]
                if c & 0xC0 == 0x80:
                    break
                if c & 0x80 == 0:
                    pass
                elif c & 0xFE == 0xFE:
                    i += 6
                elif c & 0xFC == 0xFC:
                    i += 5
                elif c & 0xF8 == 0xF8:
                    i += 4
                elif c & 0xF0 == 0xF0:
                    i += 3
                elif c & 0xE0 == 0xE0:
                    i += 2
                elif c & 0xC0 == 0xC0:
                    i += 1
                i += 1
            return i

        s = self.read_string(decode=False)
        langs = {}
        i = 0
        while i < len(s):
            n = find_multilang_next(s, i)
            try:
                lng = ord(s[i]) & 0x3F
            except TypeError:
                lng = s[i] & 0x3F
            if lng < len(self.languages):
                langs[self.languages[lng]] = s[i+1:n].decode('utf-8')
            i = n
        return langs


def mwm_unshuffle(x):
    x = ((x & 0x22222222) << 1) | ((x >> 1) & 0x22222222) | (x & 0x99999999)
    x = ((x & 0x0C0C0C0C) << 2) | ((x >> 2) & 0x0C0C0C0C) | (x & 0xC3C3C3C3)
    x = ((x & 0x00F000F0) << 4) | ((x >> 4) & 0x00F000F0) | (x & 0xF00FF00F)
    x = ((x & 0x0000FF00) << 8) | ((x >> 8) & 0x0000FF00) | (x & 0xFF0000FF)
    return x


def mwm_bitwise_split(v):
    hi = mwm_unshuffle(v >> 32)
    lo = mwm_unshuffle(v & 0xFFFFFFFF)
    x = ((hi & 0xFFFF) << 16) | (lo & 0xFFFF)
    y =     (hi & 0xFFFF0000) | (lo >> 16)
    return (x, y)


def mwm_decode_delta(v, ref):
    x, y = mwm_bitwise_split(v)
    return ref[0] + zigzag_decode(x), ref[1] + zigzag_decode(y)


def read_uint(f, bytelen=1):
    if bytelen == 1:
        fmt = 'B'
    elif bytelen == 2:
        fmt = 'H'
    elif bytelen == 4:
        fmt = 'I'
    elif bytelen == 8:
        fmt = 'Q'
    else:
        raise Exception('Bytelen {0} is not supported'.format(bytelen))
    res = struct.unpack(fmt, f.read(bytelen))
    return res[0]


def read_varuint(f):
    res = 0
    shift = 0
    more = True
    while more:
        b = f.read(1)
        if not b:
            return res
        try:
            bc = ord(b)
        except TypeError:
            bc = b
        res |= (bc & 0x7F) << shift
        shift += 7
        more = bc >= 0x80
    return res


def zigzag_decode(uint):
    res = uint >> 1
    return res if uint & 1 == 0 else -res


def read_varint(f):
    return zigzag_decode(read_varuint(f))


def unpack_osmid(num):
    typ = OsmIdCode.get_type(num)
    if typ is None:
        return None
    return typ, OsmIdCode.get_id(num)


# TODO(zverik, mgsergio): Move this to a separate module, cause it has nothing
# to do with mwm.
def read_osm2ft(f, ft2osm=False, tuples=True):
    """Reads mwm.osm2ft file, returning a dict of feature id <-> osm id."""
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

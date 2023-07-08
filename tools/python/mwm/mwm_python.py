import logging
import mmap
import struct
from datetime import datetime
from typing import AnyStr
from typing import Dict
from typing import Iterable
from typing import List
from typing import Tuple
from typing import Union

import math

from mwm import mwm_interface as mi
from mwm.exceptions import FeaturesSectionParseError

logger = logging.getLogger(__name__)


class MwmPython(mi.Mwm):
    def __init__(self, filename: str, parse: bool = False):
        super().__init__(filename)

        self.f = open(filename, "rb")
        self.file = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)

        self.tags = self._read_sections_info()

        self.seek_tag("header")
        coord_bits = read_varuint(self.file)
        self.coord_size = (1 << coord_bits) - 1
        self.base_point = mwm_bitwise_split(read_varuint(self.file))
        self.bounds_ = read_bounds(self.file, self.coord_size)
        self.scales = read_uint_array(self.file)
        self.langs = [mi.LANGS[code] for code in read_uint_array(self.file)]
        self.map_type = mi.MapType(read_varint(self.file))

        self.version_ = self._read_version()
        self.metadata_offsets = self._read_metadata_offsets()

    def version(self) -> mi.MwmVersion:
        return self.version_

    def type(self) -> mi.MapType:
        return self.map_type

    def bounds(self) -> mi.Rect:
        return self.bounds_

    def sections_info(self) -> Dict[str, mi.SectionInfo]:
        return self.tags

    def __len__(self) -> int:
        old_pos = self.file.tell()
        pos, end = self._get_features_offset_and_size()
        size = 0
        while pos < end:
            self.file.seek(pos)
            feature_size = read_varuint(self.file)
            pos = self.file.tell() + feature_size
            size += 1
        self.file.seek(old_pos)
        return size

    def __iter__(self) -> Iterable:
        return MwmPythonIter(self)

    def get_tag(self, name: str) -> mi.SectionInfo:
        return self.tags[name]

    def seek_tag(self, name: str):
        self.file.seek(self.tags[name].offset)

    def has_tag(self, name: str) -> bool:
        return name in self.tags and self.tags[name].size > 0

    def _read_sections_info(self) -> Dict[str, mi.SectionInfo]:
        self.file.seek(0)
        self.file.seek(read_uint(self.file, 8))
        tags = {}
        for _ in range(read_varuint(self.file)):
            name = read_string(self.file, plain=True)
            offset = read_varuint(self.file)
            length = read_varuint(self.file)
            tags[name] = mi.SectionInfo(name=name, offset=offset, size=length)
        return tags

    def _read_metadata_offsets(self) -> Dict[int, int]:
        if self.version_.format >= 10 :
          logger.warn("Method _read_metadata_offsets() does not have an implementation.")
          return None

        self.seek_tag("metaidx")
        tag_info = self.get_tag("metaidx")
        current = 0
        metadata_offsets = {}
        while current < tag_info.size:
            id = read_uint(self.file, 4)
            offs = read_uint(self.file, 4)
            metadata_offsets[id] = offs
            current += 8
        return metadata_offsets

    def _get_features_offset_and_size(self) -> Tuple[int, int]:
        old_pos = self.file.tell()
        pos = 0
        end = 0
        if self.version_.format < 10:
            assert self.has_tag("dat")
            tag_info = self.get_tag("dat")
            pos = tag_info.offset
            end = pos + tag_info.size
        else:
            assert self.has_tag("features")
            tag_info = self.get_tag("features")
            self.seek_tag("features")
            version = read_uint(self.file, 1)
            if version != 0:
                self.file.seek(old_pos)
                raise FeaturesSectionParseError(f"Unexpected features section version: {version}.")
            features_offset = read_uint(self.file, bytelen=4)
            if features_offset >= tag_info.size:
                self.file.seek(old_pos)
                raise FeaturesSectionParseError(f"Wrong features offset: {features_offset}.")
            pos = tag_info.offset + features_offset
            end = pos + tag_info.size - features_offset
        self.file.seek(old_pos)
        return pos, end

    def _read_version(self) -> mi.MwmVersion:
        self.seek_tag("version")
        # Skip prolog.
        self.file.read(4)
        fmt = read_varuint(self.file) + 1
        seconds_since_epoch = read_varuint(self.file)
        vdate = datetime.fromtimestamp(seconds_since_epoch)
        version = int(vdate.strftime("%y%m%d"))
        return mi.MwmVersion(
            format=fmt, seconds_since_epoch=seconds_since_epoch, version=version
        )


class MwmPythonIter:
    def __init__(self, mwm: MwmPython):
        self.mwm = mwm
        self.pos, self.end = self.mwm._get_features_offset_and_size()
        self.index = 0

    def __iter__(self) -> "MwmPythonIter":
        return self

    def __next__(self) -> "FeaturePython":
        if self.end <= self.pos:
            raise StopIteration

        self.mwm.file.seek(self.pos)
        feature_size = read_varuint(self.mwm.file)
        self.pos = self.mwm.file.tell() + feature_size
        feature = FeaturePython(self.mwm, self.index)
        self.index += 1
        return feature


class GeomType:
    POINT = 0
    LINE = 1 << 5
    AREA = 1 << 6
    POINT_EX = 3 << 5


class FeaturePython(mi.Feature):
    def __init__(self, mwm: MwmPython, index: int):
        self.mwm = mwm
        self._index = index

        header_bits = read_uint(self.mwm.file, 1)
        types_count = (header_bits & 0x07) + 1
        has_name = header_bits & 0x08 > 0
        has_layer = header_bits & 0x10 > 0
        has_addinfo = header_bits & 0x80 > 0
        geom_type = header_bits & 0x60

        self._types = [read_varuint(self.mwm.file) for _ in range(types_count)]
        self._names = read_multilang(self.mwm.file) if has_name else {}
        self._layer = read_uint(self.mwm.file, 1) if has_layer else 0

        self._rank = 0
        self._road_number = ""
        self._house_number = ""

        if has_addinfo:
            if geom_type == GeomType.POINT:
                self._rank = read_uint(self.mwm.file, 1)
            elif geom_type == GeomType.LINE:
                self._road_number = read_string(self.mwm.file)
            elif geom_type == GeomType.AREA or geom_type == GeomType.POINT_EX:
                self._house_number = read_numeric_string(self.mwm.file)

        self._geom_type = mi.GeomType.undefined
        self._geometry = []

        if geom_type == GeomType.POINT or geom_type == GeomType.POINT_EX:
            self._geometry = mi.GeomType.point
            geometry = [
                read_coord(self.mwm.file, self.mwm.base_point, self.mwm.coord_size)
            ]
        elif geom_type == GeomType.LINE:
            self._geometry = mi.GeomType.line
        elif geom_type == GeomType.AREA:
            self._geometry = mi.GeomType.area

    def readable_name(self) -> str:
        if "default" in self._names:
            return self._names["default"]
        elif "en" in self._names:
            return self._names["en"]
        elif self._names:
            k = next(iter(self._names))
            return self._names[k]
        return ""

    def population(self) -> int:
        logger.warn("Method population() does not have an implementation.")

    def center(self) -> mi.Point:
        logger.warn("Method center() does not have an implementation.")

    def limit_rect(self) -> mi.Rect:
        logger.warn("Method limit_rect() does not have an implementation.")

    def index(self) -> int:
        return self._index

    def types(self) -> List[int]:
        return self._types

    def metadata(self) -> Dict[mi.MetadataField, str]:
        mwm = self.mwm
        if mwm.metadata_offsets is None or self._index not in mwm.metadata_offsets:
            return {}

        old_pos = mwm.file.tell()
        new_pos = mwm.get_tag("meta").offset + mwm.metadata_offsets[self._index]
        mwm.file.seek(new_pos)
        metadata = {}
        if mwm.version().format >= 8:
            sz = read_varuint(mwm.file)
            for _ in range(sz):
                t = read_varuint(mwm.file)
                field = mi.MetadataField(t)
                metadata[field] = read_string(mwm.file)
        else:
            while True:
                t = read_uint(mwm.file, 1)
                is_last = t & 0x80 > 0
                t = t & 0x7F
                l = read_uint(mwm.file, 1)
                field = mi.MetadataField(t)
                metadata[field] = mwm.file.read(l).decode("utf-8")
                if is_last:
                    break

        mwm.file.seek(old_pos)
        return metadata

    def names(self) -> Dict[str, str]:
        return self._names

    def rank(self) -> int:
        return self._rank

    def road_number(self) -> str:
        return self._road_number

    def house_number(self) -> str:
        return self._house_number

    def layer(self) -> int:
        return self._layer

    def geom_type(self) -> mi.GeomType:
        return self._geom_type

    def geometry(self) -> Union[List[mi.Point], List[mi.Triangle]]:
        if self._geometry == mi.GeomType.line:
            logger.warn("Method geometry() does not have an implementation for line.")
        elif self._geometry == mi.GeomType.area:
            logger.warn("Method geometry() does not have an implementation for area.")

        return self._geometry

    def parse(self):
        pass


def get_region_info(path):
    m = MwmPython(path)
    if not m.has_tag("rgninfo"):
        return {}

    region_info = {}
    m.seek_tag("rgninfo")
    sz = read_varuint(m.file)
    for _ in range(sz):
        t = read_varuint(m.file)
        field = mi.RegionDataField(t)
        region_info[field] = read_string(m.file)
        if t == mi.RegionDataField.languages:
            region_info[field] = [mi.LANGS[ord(x)] for x in region_info[field]]
    return region_info


def read_point(f, base_point: mi.Point, packed: bool = True) -> mi.Point:
    """Reads an unsigned point, returns (x, y)."""
    u = read_varuint(f) if packed else read_uint(f, 8)
    return mwm_decode_delta(u, base_point)


def to_4326(coord_size: int, point: mi.Point) -> mi.Point:
    """Convert a point in maps.me-mercator CS to WGS-84 (EPSG:4326)."""
    merc_bounds = (-180.0, -180.0, 180.0, 180.0)  # Xmin, Ymin, Xmax, Ymax
    x = point.x * (merc_bounds[2] - merc_bounds[0]) / coord_size + merc_bounds[0]
    y = point.y * (merc_bounds[3] - merc_bounds[1]) / coord_size + merc_bounds[1]
    y = 360.0 * math.atan(math.tanh(y * math.pi / 360.0)) / math.pi
    return mi.Point(x, y)


def read_coord(
    f, base_point: mi.Point, coord_size: int, packed: bool = True
) -> mi.Point:
    """Reads a pair of coords in degrees mercator, returns (lon, lat)."""
    point = read_point(f, base_point, packed)
    return to_4326(coord_size, point)


def read_bounds(f, coord_size) -> mi.Rect:
    """Reads mercator bounds, returns (min_lon, min_lat, max_lon, max_lat)."""
    rmin = mwm_bitwise_split(read_varint(f))
    rmax = mwm_bitwise_split(read_varint(f))
    pmin = to_4326(coord_size, rmin)
    pmax = to_4326(coord_size, rmax)
    return mi.Rect(left_bottom=pmin, right_top=pmax)


def read_string(f, plain: bool = False, decode: bool = True) -> AnyStr:
    length = read_varuint(f) + (0 if plain else 1)
    s = f.read(length)
    return s.decode("utf-8") if decode else s


def read_uint_array(f) -> List[int]:
    length = read_varuint(f)
    return [read_varuint(f) for _ in range(length)]


def read_numeric_string(f) -> str:
    sz = read_varuint(f)
    if sz & 1 != 0:
        return str(sz >> 1)
    sz = (sz >> 1) + 1
    return f.read(sz).decode("utf-8")


def read_multilang(f) -> Dict[str, str]:
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

    s = read_string(f, decode=False)
    langs = {}
    i = 0
    while i < len(s):
        n = find_multilang_next(s, i)
        try:
            lng = ord(s[i]) & 0x3F
        except TypeError:
            lng = s[i] & 0x3F
        if lng < len(mi.LANGS):
            try:
                langs[mi.LANGS[lng]] = s[i + 1: n].decode("utf-8")
            except:
                print(s[i + 1: n])
        i = n
    return langs


def mwm_unshuffle(x: int) -> int:
    x = ((x & 0x22222222) << 1) | ((x >> 1) & 0x22222222) | (x & 0x99999999)
    x = ((x & 0x0C0C0C0C) << 2) | ((x >> 2) & 0x0C0C0C0C) | (x & 0xC3C3C3C3)
    x = ((x & 0x00F000F0) << 4) | ((x >> 4) & 0x00F000F0) | (x & 0xF00FF00F)
    x = ((x & 0x0000FF00) << 8) | ((x >> 8) & 0x0000FF00) | (x & 0xFF0000FF)
    return x


def mwm_bitwise_split(v) -> mi.Point:
    hi = mwm_unshuffle(v >> 32)
    lo = mwm_unshuffle(v & 0xFFFFFFFF)
    x = ((hi & 0xFFFF) << 16) | (lo & 0xFFFF)
    y = (hi & 0xFFFF0000) | (lo >> 16)
    return mi.Point(x, y)


def mwm_decode_delta(v, base_point: mi.Point) -> mi.Point:
    p = mwm_bitwise_split(v)
    return p + base_point


def read_uint(f, bytelen: int = 1) -> int:
    if bytelen == 1:
        fmt = "B"
    elif bytelen == 2:
        fmt = "H"
    elif bytelen == 4:
        fmt = "I"
    elif bytelen == 8:
        fmt = "Q"
    else:
        raise Exception("Bytelen {0} is not supported".format(bytelen))
    res = struct.unpack(fmt, f.read(bytelen))
    return res[0]


def read_varuint(f) -> int:
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


def zigzag_decode(uint: int) -> int:
    res = uint >> 1
    return res if uint & 1 == 0 else -res


def read_varint(f) -> int:
    return zigzag_decode(read_varuint(f))

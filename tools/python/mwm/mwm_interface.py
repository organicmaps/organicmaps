import enum
import os
from abc import ABC
from abc import abstractmethod
from typing import Dict
from typing import Iterable
from typing import List
from typing import Union

from mwm.feature_types import readable_type

# See coding/string_utf8_multilang.cpp to synchronize languages.
LANGS = (
    "default",
    "en",
    "ja",
    "fr",
    "ko_rm",
    "ar",
    "de",
    "int_name",
    "ru",
    "sv",
    "zh",
    "fi",
    "be",
    "ka",
    "ko",
    "he",
    "nl",
    "ga",
    "ja_rm",
    "el",
    "it",
    "es",
    "zh_pinyin",
    "th",
    "cy",
    "sr",
    "uk",
    "ca",
    "hu",
    "reserved (earlier hsb)",
    "eu",
    "fa",
    "reserved (earlier br)",
    "pl",
    "hy",
    "reserved (earlier kn)",
    "sl",
    "ro",
    "sq",
    "am",
    "no",
    "cs",
    "id",
    "sk",
    "af",
    "ja_kana",
    "reserved (earlier lb)",
    "pt",
    "hr",
    "da",
    "vi",
    "tr",
    "bg",
    "alt_name",
    "lt",
    "old_name",
    "kk",
    "reserved (earlier gsw)",
    "et",
    "ku",
    "mn",
    "mk",
    "lv",
    "hi",
)


class MetadataField(enum.Enum):
    cuisine = 1
    open_hours = 2
    phone_number = 3
    fax_number = 4
    stars = 5
    operator = 6
    url = 7
    website = 8
    internet = 9
    ele = 10
    turn_lanes = 11
    turn_lanes_forward = 12
    turn_lanes_backward = 13
    email = 14
    postcode = 15
    wikipedia = 16
    flats = 18
    height = 19
    min_height = 20
    denomination = 21
    building_levels = 22
    test_id = 23
    sponsored_id = 24
    price_rate = 25
    rating = 26
    banner_url = 27
    level = 28
    airport_iata = 29
    brand = 30
    duration = 31
    building_min_level = 40


class RegionDataField(enum.Enum):
    languages = 0
    driving = 1
    timezone = 2
    address_format = 3
    phone_format = 4
    postcode_format = 5
    public_holidays = 6
    allow_housenames = 7


class MapType(enum.Enum):
    world = 0
    world_coasts = 1
    country = 2


class GeomType(enum.Enum):
    undefined = -1
    point = 0
    line = 1
    area = 2


class SectionInfo:
    __slots__ = "name", "offset", "size"

    def __init__(self, name, offset, size):
        self.name = name
        self.offset = offset
        self.size = size

    def __repr__(self):
        return (
            f"SectionInfo[name: {self.name}, "
            f"offset: {self.offset}, "
            f"size: {self.size}]"
        )

    def to_json(self):
        return {"name": self.name, "offset": self.offset, "size": self.size}


class MwmVersion:
    __slots__ = "format", "seconds_since_epoch", "version"

    def __init__(self, format, seconds_since_epoch, version):
        self.format = format
        self.seconds_since_epoch = seconds_since_epoch
        self.version = version

    def __repr__(self):
        return (
            f"MwmVersion[format: {self.format}, "
            f"seconds since epoch: {self.seconds_since_epoch}, "
            f"version: {self.version}]"
        )

    def to_json(self):
        return {
            "format": self.format,
            "secondsSinceEpoch": self.seconds_since_epoch,
            "version": self.version,
        }


class Point:
    __slots__ = "x", "y"

    def __init__(self, x=0.0, y=0.0):
        self.x = x
        self.y = y

    def __add__(self, other):
        if isinstance(other, Point):
            return Point(self.x + other.x, self.y + other.y)
        raise NotImplementedError

    def __iadd__(self, other):
        if isinstance(other, Point):
            self.x += other.x
            self.y += other.y
        raise NotImplementedError

    def __repr__(self):
        return f"({self.x}, {self.y})"

    def to_json(self):
        return {"x": self.x, "y": self.y}


class Rect:
    __slots__ = "left_bottom", "right_top"

    def __init__(self, left_bottom: Point, right_top: Point):
        self.left_bottom = left_bottom
        self.right_top = right_top

    def __repr__(self):
        return f"Rect[{self.left_bottom}, {self.right_top}]"

    def to_json(self):
        return {
            "leftBottom": self.left_bottom.to_json(),
            "rightTop": self.right_top.to_json(),
        }


class Triangle:
    __slots__ = "x", "y", "z"

    def __init__(self, x: Point, y: Point, z: Point):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return f"Triangle[{self.x}, {self.y}, {self.z}]"

    def to_json(self):
        return {"x": self.x.to_json(), "y": self.y.to_json(), "z": self.z.to_json()}


class Mwm(ABC):
    def __init__(self, filename: str):
        self.filename = filename

    def name(self) -> str:
        return os.path.basename(self.filename)

    def path(self) -> str:
        return self.filename

    @abstractmethod
    def version(self) -> MwmVersion:
        pass

    @abstractmethod
    def type(self) -> MapType:
        pass

    @abstractmethod
    def bounds(self) -> Rect:
        pass

    @abstractmethod
    def sections_info(self) -> Dict[str, SectionInfo]:
        pass

    @abstractmethod
    def __len__(self) -> int:
        pass

    @abstractmethod
    def __iter__(self) -> Iterable:
        pass

    def __repr__(self):
        si = "\n".join(
            [
                f"    {s}"
                for s in sorted(self.sections_info().values(), key=lambda x: x.offset)
            ]
        )
        return (
            f"Mwm[\n"
            f"  name: {self.name()}\n"
            f"  type: {self.type()}\n"
            f"  version: {self.version()}\n"
            f"  number of features: {len(self)}\n"
            f"  bounds: {self.bounds()}\n"
            f"  sections info: [\n{si} \n  ]\n"
            f"]"
        )

    def to_json(self, with_features=False):
        m = {
            "name": self.name(),
            "version": self.version().to_json(),
            "type": self.type(),
            "bounds": self.bounds().to_json(),
            "sections_info": {k: v.to_json() for k, v in self.sections_info().items()},
            "size": len(self),
        }

        if with_features:
            m["features"] = [f.to_json() for f in self]

        return m


class Feature(ABC):
    @abstractmethod
    def index(self) -> int:
        pass

    @abstractmethod
    def types(self) -> List[int]:
        pass

    def readable_types(self) -> List[str]:
        return [readable_type(i) for i in self.types()]

    @abstractmethod
    def metadata(self) -> Dict[MetadataField, str]:
        pass

    @abstractmethod
    def names(self) -> Dict[str, str]:
        pass

    @abstractmethod
    def readable_name(self) -> str:
        pass

    @abstractmethod
    def rank(self) -> int:
        pass

    @abstractmethod
    def population(self) -> int:
        pass

    @abstractmethod
    def road_number(self) -> str:
        pass

    @abstractmethod
    def house_number(self) -> str:
        pass

    @abstractmethod
    def layer(self) -> int:
        pass

    @abstractmethod
    def geom_type(self) -> GeomType:
        pass

    @abstractmethod
    def center(self) -> Point:
        pass

    @abstractmethod
    def geometry(self) -> Union[List[Point], List[Triangle]]:
        pass

    @abstractmethod
    def limit_rect(self) -> Rect:
        pass

    @abstractmethod
    def parse(self):
        pass

    def __repr__(self):
        return (
            f"Feature[\n"
            f"  index: {self.index()}\n"
            f"  readable name: {self.readable_name()}\n"
            f"  types: {self.readable_types()}\n"
            f"  names: {self.names()}\n"
            f"  metadata: {self.metadata()}\n"
            f"  geom_type: {self.geom_type()}\n"
            f"  center: {self.center()}\n"
            f"  limit_rect: {self.limit_rect()}\n"
            f"]"
        )

    def to_json(self):
        center = None
        center_ = self.center()
        if center_:
            center = self.center().to_json()

        limit_rect = None
        limit_rect_ = self.limit_rect()
        if limit_rect_:
            limit_rect = limit_rect_.to_json()

        return {
            "index": self.index(),
            "types": {t: readable_type(t) for t in self.types()},
            "metadata": {k.name: v for k, v in self.metadata().items()},
            "names": self.names(),
            "readable_name": self.readable_name(),
            "rank": self.rank(),
            "population": self.population(),
            "road_number": self.road_number(),
            "house_number": self.house_number(),
            "layer": self.layer(),
            "geom_type": self.geom_type(),
            "center": center,
            "limit_rect": limit_rect,
        }

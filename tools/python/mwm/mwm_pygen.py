from typing import Dict
from typing import Iterable
from typing import List
from typing import Union

from pygen import classif
from pygen import geometry
from pygen import mwm

from mwm import mwm_interface as mi


def init(resource_path):
    classif.init_classificator(resource_path)


class MwmPygen(mi.Mwm):
    def __init__(self, filename: str, parse: bool = False):
        super().__init__(filename)

        self.mwm = mwm.Mwm(filename, parse)

    def version(self) -> mi.MwmVersion:
        v = self.mwm.version()
        return mi.MwmVersion(
            format=int(v.format()) + 1,
            seconds_since_epoch=v.seconds_since_epoch(),
            version=v.version(),
        )

    def type(self) -> mi.MapType:
        t = self.mwm.type()
        return mi.MapType(int(t))

    def bounds(self) -> mi.Rect:
        b = self.mwm.bounds()
        return from_pygen_rect(b)

    def sections_info(self) -> Dict[str, mi.SectionInfo]:
        si = self.mwm.sections_info()
        return {
            k: mi.SectionInfo(name=v.tag, offset=v.offset, size=v.size)
            for k, v in si.items()
        }

    def __len__(self) -> int:
        return self.mwm.__len__()

    def __iter__(self) -> Iterable:
        return FeaturePygenIter(self.mwm.__iter__())


class FeaturePygenIter:
    def __init__(self, iter: mwm.MwmIter):
        self.iter = iter

    def __iter__(self) -> "FeaturePygenIter":
        return self

    def __next__(self) -> "FeaturePygen":
        ft = self.iter.__next__()
        return FeaturePygen(ft)


class FeaturePygen(mi.Feature):
    def __init__(self, ft: mwm.FeatureType):
        self.ft = ft

    def index(self) -> int:
        return self.ft.index()

    def types(self) -> List[int]:
        return self.ft.types()

    def metadata(self) -> Dict[mi.MetadataField, str]:
        m = self.ft.metadata()
        return {mi.MetadataField(int(k)): v for k, v in m.items()}

    def names(self) -> Dict[str, str]:
        return self.ft.names()

    def readable_name(self) -> str:
        return self.ft.readable_name()

    def rank(self) -> int:
        return self.ft.rank()

    def population(self) -> int:
        return self.ft.population()

    def road_number(self) -> str:
        return self.ft.road_number()

    def house_number(self) -> str:
        return self.ft.house_number()

    def layer(self) -> int:
        return self.ft.layer()

    def geom_type(self) -> mi.GeomType:
        g = self.ft.geom_type()
        return mi.GeomType(int(g))

    def center(self) -> mi.Point:
        c = self.ft.center()
        return from_pygen_point(c)

    def geometry(self) -> Union[List[mi.Point], List[mi.Triangle]]:
        if self.geom_type() == mi.GeomType.area:
            return [from_pygen_triangle(t) for t in self.ft.geometry()]

        return [from_pygen_point(t) for t in self.ft.geometry()]

    def limit_rect(self) -> mi.Rect:
        r = self.ft.limit_rect()
        return from_pygen_rect(r)

    def parse(self):
        self.ft.parse()


def from_pygen_point(p: geometry.PointD) -> mi.Point:
    return mi.Point(p.x, p.y)


def from_pygen_rect(r: geometry.RectD) -> mi.Rect:
    return mi.Rect(from_pygen_point(r.left_bottom), from_pygen_point(r.right_top))


def from_pygen_triangle(t: geometry.TriangleD) -> mi.Triangle:
    return mi.Triangle(
        from_pygen_point(t.x()), from_pygen_point(t.y()), from_pygen_point(t.z())
    )

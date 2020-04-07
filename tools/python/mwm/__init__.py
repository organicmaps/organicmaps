import os

resource_path = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "..", "..", "..", "data",
)

from mwm.types import init as _init

_init(resource_path)

from mwm.mwm_interface import GeomType
from mwm.mwm_interface import MapType
from mwm.mwm_interface import MetadataField
from mwm.mwm_interface import Point
from mwm.mwm_interface import Rect
from mwm.mwm_interface import RegionDataField
from mwm.mwm_interface import Triangle
from mwm.mwm_python import get_region_info
from mwm.types import readable_type
from mwm.types import type_index
from mwm.utils import EnumAsStrEncoder

try:
    from mwm.mwm_pygen import MwmPygen as Mwm
    from mwm.mwm_pygen import FeaturePygen as Feature

    from mwm.mwm_pygen import init as _init

    _init(resource_path)
except ImportError:
    from mwm.mwm_python import MwmPython as Mwm
    from mwm.mwm_python import FeaturePython as Feature

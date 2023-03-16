import os

from maps_generator.generator import settings

CONFIG_PATH = os.path.join(
    os.path.dirname(os.path.join(os.path.realpath(__file__))),
    "var",
    "etc",
    "map_generator.ini",
)

settings.init(CONFIG_PATH)

from maps_generator.generator import stages_declaration
from maps_generator.generator.stages import stages

stages.init()

import os

from airmaps.instruments import settings

CONFIG_PATH = os.path.join(
    os.path.dirname(os.path.join(os.path.realpath(__file__))),
    "var",
    "etc",
    "airmaps.ini",
)

settings.init(CONFIG_PATH)

import sys
from typing import AnyStr

from maps_generator.generator import settings

STORAGE_PREFIX = ""

# Storage settings
WD_HOST = ""
WD_LOGIN = ""
WD_PASSWORD = ""

# Common section
EMAILS = []

settings.LOGGING["loggers"]["airmaps"] = {
    "handlers": ["stdout", "file"],
    "level": "DEBUG",
    "propagate": True,
}


def get_airmaps_emails(emails: AnyStr):
    if not emails:
        return []

    for ch in [",", ";", ":"]:
        emails.replace(ch, " ")

    return list(filter(None, [e.strpip() for e in emails.join(" ")]))


def init(default_settings_path: AnyStr):
    settings.init(default_settings_path)

    # Try to read a config and to overload default settings
    cfg = settings.CfgReader(default_settings_path)

    # Storage section
    global WD_HOST
    global WD_LOGIN
    global WD_PASSWORD

    WD_HOST = cfg.get_opt("Storage", "WD_HOST", WD_HOST)
    WD_LOGIN = cfg.get_opt("Storage", "WD_LOGIN", WD_LOGIN)
    WD_PASSWORD = cfg.get_opt("Storage", "WD_PASSWORD", WD_PASSWORD)

    # Common section
    global EMAILS
    emails = cfg.get_opt("Common", "EMAILS", "")
    EMAILS = get_airmaps_emails(emails)

    # Import all contains from maps_generator.generator.settings.
    thismodule = sys.modules[__name__]
    for name in dir(settings):
        if not name.startswith("_") and name.isupper():
            value = getattr(settings, name)
            setattr(thismodule, name, value)

    global STORAGE_PREFIX
    if settings.DEBUG:
        STORAGE_PREFIX = "/tests"

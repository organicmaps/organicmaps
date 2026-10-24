from pathlib import Path
from configparser import ConfigParser

from .holiday import HolidayDb


def load_from_twine(holiday_db: HolidayDb, path: Path) -> None:
    """
    Reads a twine file produced by write_to_twine and reconstructs a HolidayDb.
    """
    if not path.exists():
        return

    text = path.read_text(encoding="utf-8")
    # Strip the [[public_holidays]] header so ConfigParser can handle the rest.
    text = text.replace("[[public_holidays]]\n", "", 1)

    config = ConfigParser()
    config.optionxform = str  # preserve case of keys (language codes like 'ru', 'zh-Hans', …)
    config.read_string(text)

    for section in config.sections():
        items = dict(config[section])

        # The English name is stored under the "en" key; fall back to the section name.
        english_name = items.pop("en", section)

        holiday = holiday_db.get_holiday(english_name)

        if "tags" in items:
            holiday.name.tags = items.pop("tags")
        else:
            items.pop("tags", None)

        # Drop the comment field – it is reconstructed on write.
        items.pop("comment", None)

        # Everything left is a language-code → translated-name mapping.
        for lang, name in items.items():
            holiday.name.translations[lang] = name


def write_to_twine(holiday_db: HolidayDb, path: Path) -> None:
    """
    [[public_holidays]]

    [<holiday_name_snake_case>]
    comment = <ISO Codes of countries that celebrate this holiday>
    tags = <tags>
    en = <holiday name in English>
    <lang> = <holiday name from Holiday.name.translations
    """
    config = ConfigParser()
    config.optionxform = str  # preserve case of keys

    for holiday in holiday_db.holidays:
        if not holiday.holiday_date:
            continue

        section = holiday.name.snake_case_name
        if section in config:
            print(f"Warning: duplicate section {section} in twine config")
            print(f"  New: {holiday.name.name} ({holiday.name.tags})")
            print(f"  Old: {config[section]['en']} ({config[section]['tags']})")
            # raise ValueError("Duplicate section in twine config")
        config[section] = {
            "comment": "Countries: " + ", ".join(sorted(holiday.holiday_date.keys())),
            "tags": holiday.name.tags,
            "en": holiday.name.name,
        }
        for lang, name in holiday.name.translations.items():
            config[section][lang] = name

    with path.open("w", encoding="utf-8") as f:
        f.write("[[public_holidays]]\n\n")
        config.write(f)

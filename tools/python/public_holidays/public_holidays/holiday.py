import re
import unicodedata
from datetime import date
from dataclasses import dataclass
from enum import Enum
from typing import Optional

CountryIsoCode = str


def _normalize_name(name: str) -> str:
    """Normalize a holiday name by:
    - Stripping leading/trailing whitespace and zero-width characters
    - Transliterating accented/special Unicode letters to ASCII equivalents
    - Replacing slashes, hyphens, ampersands and other separators with spaces
    - Removing remaining non-alphanumeric characters (apostrophes, dots, commas, parentheses, etc.)
    - Collapsing multiple spaces into one
    """
    # Strip zero-width and other invisible Unicode characters
    name = name.strip()
    name = re.sub(r"[\u200b\u200c\u200d\ufeff]", "", name)

    # Transliterate accented characters to their ASCII base (e.g. á→a, ē→e, ţ→t)
    name = unicodedata.normalize("NFD", name)
    name = "".join(ch for ch in name if unicodedata.category(ch) != "Mn")

    # Replace separators (slash, hyphen, ampersand) with a space so words stay separate
    name = re.sub(r"[/\-&]", " ", name)

    # Remove characters that are not alphanumeric or spaces
    # This covers: apostrophes, dots, commas, parentheses, colons, etc.
    name = re.sub(r"[^\w\s]", "", name)

    # Collapse multiple whitespace characters into a single space and strip
    name = re.sub(r"\s+", " ", name).strip()

    return name


def _to_snake_case(name: str) -> str:
    """Convert a holiday name to snake_case identifier.

    Leading digits are prefixed with an underscore so the result is a valid identifier.
    Example: '1848 Revolution Memorial Day' -> '_1848_revolution_memorial_day'
    """
    normalized = _normalize_name(name)
    snake = normalized.replace(" ", "_").lower()
    if snake and snake[0].isdigit():
        snake = "_" + snake
    return snake


def _to_camel_case(name: str) -> str:
    """Convert a holiday name to CamelCase (PascalCase) identifier.

    Leading digits are prefixed with an underscore so the result is a valid identifier.
    Example: '1848 Revolution Memorial Day' -> '_1848RevolutionMemorialDay'
    """
    normalized = _normalize_name(name)
    camel = "".join(word.capitalize() for word in normalized.split())
    if camel and camel[0].isdigit():
        camel = "_" + camel
    return camel


class HolidayType(Enum):
    Public = 0
    Bank = 1
    Optional = 2
    School = 3
    BackToSchool = 4
    EndOfLessons = 5
    Authorities = 6
    Observance = 7


@dataclass
class HolidayName:
    # Holiday name in English
    name: str
    # holiday_name
    snake_case_name: str
    # HolidayName
    camel_case_name: str
    translations: dict[str, str]
    tags: str
    comment: Optional[str]

    def __init__(self, name: str):
        self.name = name
        self.snake_case_name = _to_snake_case(name)
        self.camel_case_name = _to_camel_case(name)
        self.translations = {}
        self.tags = "android-sdk"

    def __str__(self):
        return self.name


@dataclass
class Holiday:
    name: HolidayName
    holiday_date: dict[CountryIsoCode, set[date]]
    type: Optional[HolidayType]

    def add_date(self, country: CountryIsoCode, holiday_date: date):
        self.holiday_date.setdefault(country, set()).add(holiday_date)

    def __lt__(self, other):
        return self.name.snake_case_name < other.name.snake_case_name

class HolidayDb:
    def __init__(self):
        self._holidays: list[Holiday] = []

    def get_holiday(self, name: str):
        for holiday in self._holidays:
            if holiday.name.snake_case_name == name or holiday.name.camel_case_name == name or holiday.name.name == name:
                return holiday
        new_holiday = Holiday(name=HolidayName(name=name), holiday_date={}, type=None)
        self._holidays.append(new_holiday)
        return new_holiday

    @property
    def holidays(self) -> list[Holiday]:
        return sorted(self._holidays)

    def get_holidays_for_country(self, country: CountryIsoCode) -> list[Holiday]:
        return [h for h in self._holidays if country in h.holiday_date]

import re
import json
from datetime import datetime, timezone
from pathlib import Path

SCRIPT_DIR: Path = Path(__file__).parent.resolve()
MAPCSS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/include/Icons.mapcss"
TAGINFO_FILE: Path = SCRIPT_DIR / "../../data/taginfo.json"

BASE_ICON_URL: str = "https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/styles/default/light/symbols/"

PROJECT_INFO: dict[str, str] = {
    "name": "Organic Maps",
    "description": "Free Android & iOS offline maps app for travelers, tourists, hikers, and cyclists",
    "project_url": "https://organicmaps.app",
    "doc_url": "https://github.com/organicmaps/organicmaps",
    "icon_url": "https://organicmaps.app/logos/green-on-transparent.svg",
    "contact_name": "Organic Maps",
    "contact_email": "hello@organicmaps.app"
}

def parse_mapcss(text: str) -> list[dict[str, any]]:
    tags: dict[tuple[str, str | None, str], dict[str, any]] = {}

    # Split blocks into: selector { props }
    blocks: list[tuple[str, str]] = re.findall(r"([^\{]+)\{([^\}]*)\}", text, re.MULTILINE)

    for selector, props in blocks:
        # Extract icon filename from props
        icon_re: re.Pattern = re.compile(r"icon-image:\s*([^;]+);")
        icon_match: re.Match | None = icon_re.search(props)
        icon_url: str | None = None
        if icon_match:
            icon_file: str = icon_match.group(1).strip()
            if icon_file and icon_file.lower() not in ["none", "zero-icon.svg"]:
                icon_url = BASE_ICON_URL + icon_file

        # Split the selector into lines
        lines: list[str] = [line.strip() for line in selector.split("\n") if line.strip()]
        for line in lines:
            # Find anything inside square brackets
            square_brackets_re: re.Pattern = re.compile(r"\[(.*?)\]")
            square_brackets: list[str] = square_brackets_re.findall(line)
            if not square_brackets:
                continue

            # Find key=value pairs
            pairs: list[tuple[str, str | None]] = []
            for sqb in square_brackets:
                key, sep, value = sqb.partition("=")
                key = key.strip()
                if key.startswith("!"):
                    continue  # skip negated keys
                value = value.strip() if sep else None
                pairs.append((key, value))

            # Hardcode: convert value "not" to "no"
            pairs = [(k, "no" if v == "not" else v) for k, v in pairs]

            if not pairs:
                continue  # skip if no valid pairs

            # Build shared description from all pairs
            desc: str = " + ".join(f"{k}={v if v is not None else '*'}" for k, v in pairs)

            # Emit a tag per pair
            for key, value in pairs:
                tag_id: tuple[str, str | None, str] = (key, value, desc)
                if tag_id not in tags:
                    tag: dict[str, any] = {
                        "description": desc,
                        "key": key,
                    }
                    if value is not None:
                        tag["value"] = value
                    if icon_url:
                        tag["icon_url"] = icon_url
                    tags[tag_id] = tag
                else:
                    if icon_url:
                        tags[tag_id]["icon_url"] = icon_url

    # Sort by description, then key, then value
    return sorted(tags.values(), key=lambda x: (x["description"], x["key"], x.get("value", "")))


def main() -> None:
    with open(MAPCSS_FILE, "r", encoding="utf-8") as f:
        mapcss: str = f.read()

    tags: list[dict[str, any]] = parse_mapcss(mapcss)

    data: dict[str, any] = {
        "data_format": 1,
        "data_url": "https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/taginfo.json",
        "data_updated": datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ"),
        "project": PROJECT_INFO,
        "tags": tags
    }

    with open(TAGINFO_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4, ensure_ascii=False)

    print(f"✅ JSON saved to {TAGINFO_FILE}")


if __name__ == "__main__":
    main()

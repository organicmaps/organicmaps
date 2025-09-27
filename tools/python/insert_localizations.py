import os
import re
import sys
import xml.etree.ElementTree as ET

# Android config
android_base = "../../android/app/src/main/res"
android_file = "strings.xml"

# iOS config
ios_base = "../../iphone/Maps/LocalizedStrings"
ios_file = "Localizable.strings"

def parse_translations(text: str) -> dict:
    translations = {}
    for line in text.strip().splitlines():
        locale, translation = line.strip().split(":", 1)
        translations[locale.strip()] = translation.strip()
    return translations

def android_folder_for_locale(locale: str) -> str:
    """
    Map 'en' -> values
    Map 'es' -> values-es
    Map 'en-GB' -> values-en-rGB (region qualifier)
    Otherwise -> values-<locale>
    """
    if locale == "en":
        return "values"
    if "-" in locale:
        lang, region = locale.split("-", 1)
        if len(region) == 2 and region.isalpha():
            return f"values-{lang}-r{region.upper()}"
        return f"values-{locale}"
    return f"values-{locale}"

def load_tree_preserving_comments(file_path: str) -> ET.ElementTree:
    parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    return ET.parse(file_path, parser=parser)

def update_android(key: str, translations: dict):
    for locale, text in translations.items():
        folder_name = android_folder_for_locale(locale)
        folder = os.path.join(android_base, folder_name)
        os.makedirs(folder, exist_ok=True)
        file_path = os.path.join(folder, android_file)

        if os.path.exists(file_path):
            tree = load_tree_preserving_comments(file_path)
            root = tree.getroot()
        else:
            root = ET.Element("resources")
            tree = ET.ElementTree(root)

        # Find existing <string name="key">
        target = None
        for node in list(root):
            if isinstance(node.tag, str) and node.tag == "string" and node.get("name") == key:
                target = node
                break

        if target is not None:
            target.text = text
        else:
            ET.SubElement(root, "string", {"name": key}).text = text

        ET.indent(tree, space="    ")

        tree.write(file_path, encoding="utf-8", xml_declaration=True)

        # Force double quotes in XML declaration
        with open(file_path, "r+", encoding="utf-8") as f:
            content = f.read()
            content = re.sub(r"<\?xml version='1.0' encoding='utf-8'\?>",
                             '<?xml version="1.0" encoding="utf-8"?>',
                             content)
            # Ensure the file ends with a newline
            if not content.endswith("\n"):
                content += "\n"
            f.seek(0)
            f.write(content)
            f.truncate()

        print(f"✅ Updated Android: {file_path}")

def update_ios(key: str, translations: dict):
    for locale, text in translations.items():
        folder = os.path.join(ios_base, f"{locale}.lproj")
        os.makedirs(folder, exist_ok=True)
        file_path = os.path.join(folder, ios_file)

        new_line = f"\"{key}\" = \"{text}\";"

        if os.path.exists(file_path):
            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()

            if f"\"{key}\"" in content:
                # Replace existing translation
                content = re.sub(rf'"{key}"\s*=\s*".*?";', new_line, content)
            else:
                content += "\n" + new_line if not content.endswith("\n") else new_line + "\n"
        else:
            content = new_line + "\n"

        with open(file_path, "w", encoding="utf-8") as f:
            f.write(content)

        print(f"✅ Updated iOS: {file_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python insert_localizations.py <key> <translations.txt>")
        sys.exit(1)

    key = sys.argv[1]
    file_path = sys.argv[2]

    with open(file_path, "r", encoding="utf-8") as f:
        translations_text = f.read()

    translations = parse_translations(translations_text)

    update_android(key, translations)
    update_ios(key, translations)

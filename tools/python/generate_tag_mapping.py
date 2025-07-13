import argparse
import csv
import json
import re
import os

def parse_mapcss_selectors(selectors_str):
    """
    Parses MapCSS selectors like [key1=value1][key2=value2] into list of tags
    """
    primary_selectors = selectors_str.split(',')[0]
    tags = []
    pattern = re.compile(r'\[([^=\]]+)=([^\]]+)\]')
    matches = pattern.findall(primary_selectors)
    
    for key, value in matches:
        tags.append({"key": key, "value": value})
    return tags

def generate_mapping(csv_path, json_path):
    mapping = {}
    
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.reader(f, delimiter=';')
        for row in reader:
            if not row or row[0].strip().startswith('#') or len(row) < 2:
                continue

            om_type_full = row[0].strip()
            if om_type_full.startswith(('deprecated:', 'moved:')):
                continue

            om_type = om_type_full.replace('|', '-')
            tags = []
            # Check for long format
            if len(row) > 5 and row[1].strip().startswith('['):
                selectors = row[1].strip()
                tags = parse_mapcss_selectors(selectors)
            # Check for short format
            elif len(row) >= 2 and row[1].strip().isdigit():
                parts = om_type_full.split('|')
                if len(parts) >= 2:
                    tags.append({"key": parts[0], "value": "|".join(parts[1:])})

            if tags:
                mapping[om_type] = tags

    os.makedirs(os.path.dirname(json_path), exist_ok=True)
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(mapping, f, indent=2, sort_keys=True, ensure_ascii=False)
    print(f"Generated tag mapping: {os.path.relpath(json_path)}")

def main():
    parser = argparse.ArgumentParser(description="Generate OM type to OSM tag mapping JSON")
    parser.add_argument('--input', required=True, help="Path to mapcss-mapping.csv")
    parser.add_argument('--output', required=True, help="Path for the output om_type_to_osm_tags.json")
    args = parser.parse_args()
    generate_mapping(args.input, args.output)

if __name__ == '__main__':
    main()

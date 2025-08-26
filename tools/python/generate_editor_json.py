import argparse
import csv
import json
import re
import os
import xml.etree.ElementTree as ET
from collections import OrderedDict

def parse_mapcss_selectors(selectors_str):
    """Parses MapCSS selectors like [key1=value1] into a dictionary of tags."""
    primary_selectors = selectors_str.split(',')[0]
    tags = {}
    pattern = re.compile(r'\[([^=\]]+)=([^\]]+)\]')
    matches = pattern.findall(primary_selectors)
    for key, value in matches:
        tags[key] = value
    return tags

def get_tag_mapping_from_csv(csv_path):
    """Creates a dictionary of OM Type -> OSM Tags from mapcss-mapping.csv."""
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
            tags = {}

            # Long format: e.g., amenity|recycling|centre;[amenity=recycling][recycling_type=centre];...
            if len(row) > 5 and row[1].strip().startswith('['):
                selectors = row[1].strip()
                tags = parse_mapcss_selectors(selectors)
            # Short format: e.g., highway|residential;2;
            elif len(row) >= 2 and row[1].strip().isdigit():
                parts = om_type_full.split('|')
                if len(parts) >= 2:
                    tags[parts[0]] = "|".join(parts[1:])
            if tags:
                mapping[om_type] = tags
    return mapping

def parse_editor_config_xml(xml_path):
    """Parses the editor.config XML into a Python dictionary."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    editor_node = root.find('editor')

    config = OrderedDict()
    config["fields"] = OrderedDict()
    config["groups"] = OrderedDict()
    config["types"] = []  # Using a list to preserve order

    # Parse Fields
    for field in editor_node.find('fields').findall('field'):
        field_name = field.get('name')
        field_data = OrderedDict()
        if field.get('editable') == 'no': field_data['editable'] = False
        if field.get('multilanguage') == 'yes': field_data['multilanguage'] = True

        tags = [t.get('k') for t in field.findall('tag')]
        tags.extend([alt.get('k') for alt in field.findall('alt')])
        if tags: field_data['tags'] = tags

        value_node = field.find('value')
        if value_node is not None:
            if value_node.get('type'): field_data['value_type'] = value_node.get('type')
            if value_node.get('many') == 'yes': field_data['allow_multiple'] = True
            options = [opt.get('value') for opt in value_node.findall('option')]
            if options: field_data['options'] = options

        config["fields"][field_name] = field_data

    # Parse Field Groups
    for group in editor_node.find('fields').findall('field_group'):
        group_name = group.get('name')
        fields = [ref.get('name') for ref in group.findall('field_ref')]
        config["groups"][group_name] = {"fields": fields}

    # Parse Types, preserving order
    for type_node in editor_node.find('types').findall('type'):
        type_id = type_node.get('id')
        type_data = OrderedDict()
        type_data["id"] = type_id # Add id inside the object

        if type_node.get('can_add') == 'no': type_data['can_add'] = False
        if type_node.get('editable') == 'no': type_data['editable'] = False
        if type_node.get('group'): type_data['group'] = type_node.get('group')
        if type_node.get('priority'): type_data['priority'] = type_node.get('priority')

        includes = [inc.get('field') for inc in type_node.findall('include[@field]')]
        if includes: type_data['include'] = includes

        include_groups = [inc.get('group') for inc in type_node.findall('include[@group]')]
        if include_groups: type_data['include_groups'] = include_groups

        config["types"].append(type_data)

    return config

def main():
    parser = argparse.ArgumentParser(description="Generate a unified editor.json from editor.config and mapcss-mapping.csv.")
    parser.add_argument('--mapcss', required=True, help="Path to mapcss-mapping.csv")
    parser.add_argument('--xml_config', required=True, help="Path to the source editor.config XML")
    parser.add_argument('--output', required=True, help="Path for the output unified editor.json")
    args = parser.parse_args()

    print("Generating base config from editor.config XML...")
    config_data = parse_editor_config_xml(args.xml_config)

    print("Generating OSM tag mapping from mapcss-mapping.csv...")
    tag_mapping = get_tag_mapping_from_csv(args.mapcss)

    print("Merging tag mapping into config...")

    # Add tags to existing types
    existing_types = {t['id'] for t in config_data['types']}
    for type_obj in config_data['types']:
        om_type = type_obj['id']
        if om_type in tag_mapping:
            type_obj['tags'] = tag_mapping[om_type]

    # Add new types from mapping-mapcss.csv that were not in the XML(editor.config)
    # not addable , not editable by default ( using editable=false tag )
    for om_type, tags in tag_mapping.items():
        if om_type not in existing_types:
            new_type_obj = OrderedDict()
            new_type_obj["id"] = om_type
            new_type_obj["tags"] = tags
            new_type_obj["editable"] = False
            config_data['types'].append(new_type_obj)

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, 'w', encoding='utf-8') as f:
        json.dump(config_data, f, indent=2, ensure_ascii=False)

    print(f"Successfully generated unified editor.json at: {os.path.relpath(args.output)}")

if __name__ == '__main__':
    main()
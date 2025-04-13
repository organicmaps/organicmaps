"""The script checks that an OSM extract with continents obtained
with the osmfilter command
osmfilter "${PLANET}" \
    --keep="( place=continent )" \
    --keep-tags="all place= name= name:en= wikidata=" \
    --ignore-dependencies --drop-version \
    --out-osm -o="continents.osm"
coincides with a reference extract. Reference extract can be obtained
in json format from OSM extract with xml_to_json() function.
In case when differences found the errors are logged and non-zero
exit code is returned.

"""

import argparse
import json
import logging
import sys
from lxml import etree


ESSENTIAL_TAGS = ('place', 'name', 'name:en', 'wikidata')

# Admissible shift of continent label
SHIFT_THRESHOLD = 1.0  # degrees


def xml_to_json(xml_file):
    continents = {}  # id in the form 'n123' =>
                     #   { 'id': id,
                     #     'tags': {k1: v1, ...},
                     #     'labels': [member_ids] }
    for event, element in etree.iterparse(xml_file,
                                          tag=('node', 'way', 'relation')):
        el_id = f"{element.tag[0]}{element.get('id')}"
        tags = dict.fromkeys(ESSENTIAL_TAGS)
        continent = {'id': el_id, 'tags': tags}
        if element.tag == 'node':
            continent['coords'] = {c: float(element.get(c))
                                        for c in ('lat', 'lon')}
        for sub in element:
            if sub.tag == 'tag':
                key = sub.get('k')
                if key in ESSENTIAL_TAGS:
                    tags[key] = sub.get('v')
            elif sub.tag == 'member':
                if sub.get('role') == 'label':
                    label_id = f"{sub.get('type')[0]}{sub.get('ref')}"
                    continent.setdefault('labels', []).append(label_id)
        if continent['tags']['place'] == 'continent':
            continents[el_id] = continent
    return continents


def is_the_same_continent(cont1_data, cont2_data):
    return all(cont1_data['tags'][tag] == cont2_data['tags'][tag]
               for tag in ESSENTIAL_TAGS)


def is_continent_shifted(current_continent, reference_continent):
    assert current_continent['id'] == reference_continent['id']
    if current_continent['id'][0] != 'n':
        return False
    for coord in ('lat', 'lon'):
        cur_coord = current_continent['coords'][coord]
        ref_coord = reference_continent['coords'][coord]
        if abs(cur_coord - ref_coord) >= SHIFT_THRESHOLD:
            return True
    return False


def check_continent_relation_labels(continents, continent_rel_id):
    """The function checks that a continent relation has no more than one
    'label' member of type 'node' with coinciding relevant tags.
    """
    assert continent_rel_id[0] == 'r', ("A relation expected in "
                                        "check_continent_relation_labels")
    cont_data = continents[continent_rel_id]
    labels = cont_data.get('labels', [])
    errors = []
    if len(labels) > 1:
        name = cont_data['tags']['name:en']
        errors.append(f"More than one label for continent relation "
                      f"{continent_rel_id} '{name}'")
        for label_id in labels:
            if label_id[0] != 'n':
                errors.append(f"Label {label_id} is not a node "
                              f"in continent relation {continent_rel_id}")
            elif label_id not in continents:
                errors.append(f"Label {label_id} for continent relation "
                              f"{continent_rel_id} '{name}' "
                              f"is not a continent itself")
            else:
                if not is_the_same_continent(continents[label_id],
                                             cont_data):
                    errors.append(f"Continent relation {continent_rel_id} "
                                  f"and its label {label_id} "
                                  "have different essential tags")
    return errors



def check_continent_node_membership(continents, continent_node_id):
    """The function checks that a node continent is bound
        to the corresponding relation through 'label' membership.
    """
    assert continent_node_id[0] == 'n', ("A node expected in "
                                         "check_continent_node_membership()")
    errors = []
    for cont_id, cont_data in continents.items():
        continent_rel_id, continent_rel_data = cont_id, cont_data
        if (cont_id[0] == 'r' and
            is_the_same_continent(continents[continent_node_id],
                                  continent_rel_data) and
            continent_node_id not in continent_rel_data.get('labels', [])
        ):
            errors.append(f"Node {continent_node_id} represents the same "
                          f"continent as relation {continent_rel_id} "
                          "but is not its label")
    return errors


def check_continents_consistency(continents):
    errors = []
    for cont_id, cont_data in continents.items():
        if cont_id[0] == 'r':
            cont_errors = check_continent_relation_labels(continents, cont_id)
            errors.extend(cont_errors)
        elif cont_id[0] == 'n':
            cont_errors = check_continent_node_membership(continents, cont_id)
            errors.extend(cont_errors)
    return errors


def compare_continents_with_reference(current_continents, reference_continents):
    errors = []
    for cont_id in set(current_continents) - set(reference_continents):
        name = current_continents[cont_id]['tags']['name:en']
        errors.append(f"Continent {cont_id} '{name}' is absent in "
                      "reference data")
    for cont_id in set(reference_continents) - set(current_continents):
        name = reference_continents[cont_id]['tags']['name:en']
        errors.append(f"Reference continent {cont_id} '{name}' is absent in "
                      "current data")
    for cont_id in set(reference_continents) & set(current_continents):
        current_continent = current_continents[cont_id]
        reference_continent = reference_continents[cont_id]
        if not is_the_same_continent(current_continent,
                                     reference_continent):
            errors.append(f"Continent {cont_id} has different tags "
                          "in current and reference data")
        elif is_continent_shifted(current_continent, reference_continent):
            errors.append(f"Continent {cont_id} was significantly shifted "
                          "relative to the reference one")
    return errors


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    log_levels = [
        name.lower()
        for number, name in logging._levelToName.items()
        if number > 0
    ]
    parser.add_argument('-L', '--log-level', choices=log_levels,
                        default='error', help='log level')
    parser.add_argument('-c', '--continents-current', required=True,
                        help='Path to *.osm with current extract of continents')
    parser.add_argument('-r', '--continents-reference', required=True,
                        help='Path to *.json with reference continents')
    parser.add_argument('-o', '--output', required=False,
                        help='File name to output current continents as json')

    options = parser.parse_args()

    log_level_name = options.log_level.upper()
    logging.basicConfig(level=getattr(logging, log_level_name),
                        format='%(levelname)-8s  %(message)s')

    try:
        with open(options.continents_reference) as f:
            reference_continents = json.load(f)
        reference_data_errors = check_continents_consistency(
            reference_continents
        )
        if reference_data_errors:
            logging.error("Reference continent data is inconsistent:")
            for error in reference_data_errors:
                logging.critical("    " + error)
    except Exception as e:
        logging.critical(f"Cannot load reference continent data: {repr(e)}")
        sys.exit(1)

    try:
        current_continents = xml_to_json(options.continents_current)

        if options.output:
            with open(options.output, 'w') as f:
                json.dump(current_continents, f, indent=2, ensure_ascii=False)

        current_consistency_errors = check_continents_consistency(
            current_continents
        )
        if current_consistency_errors:
            logging.error("Current continent data is inconsistent:")
            for error in current_consistency_errors:
                logging.error("    " + error)
        reference_comparison_errors = compare_continents_with_reference(
            current_continents, reference_continents
        )
        if reference_comparison_errors:
            logging.error("Current continent data differs "
                          "from the reference one:")
            for error in reference_comparison_errors:
                logging.error("    " + error)
    except Exception as e:
        logging.critical(f"Cannot process current continent data: {repr(e)}")
        sys.exit(1)


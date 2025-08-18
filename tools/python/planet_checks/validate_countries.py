"""
Required countries and country whitelist can be taken here:
http://testdata.mapsme.cloud.devmail.ru/planet_checks/

The script verifies that OSM dump contains all countries from a reference file
and produces errors/warnings in case something has changed. No output provided
CRITICAL log level is set means the planet may be used quite safely for maps
generation.

One can extract countries with their capitals with the following overpass-query:
(
  relation["admin_level"="2"]["type"="boundary"]["boundary"="administrative"];
  nwr(r:"admin_centre");
  nwr(r:"label");
  nwr[place=country];
  nwr[place][capital=yes];
  nwr[place][capital=2];
);
out;

or with osmfilter utility:

/path/to/osmfilter "${PLANET}" \
  --keep-nodes="( place=country ) OR ( place= AND ( capital=yes OR capital=2 ) )" \
  --keep-ways="( place=country ) OR ( place= AND ( capital=yes OR capital=2 ) )" \
  --keep-relations="( place=country ) OR ( place= AND ( capital=yes OR capital=2 ) ) \
    OR ( type=boundary AND boundary=administrative AND admin_level=2 )" \
  --keep-tags="all place= capital= name= name:en= type= boundary= admin_level=" \
  --ignore-dependencies \
  --drop-version \
  --out-osm \
  -o="countries-filtered.osm"

Note that, due to recursion queries for labels and admin_centres,
overpass-query generates a bit more data than osmfilter of planet.
Without --drop-dependencies option osmfilter would yield 30 Gi, with the option -
2.7 Mb. Had all capitals in OSM have correct [place~"*"][capital~"yes|2"]
tags, the overpass-api and osmfilter outputs would be the same.
"""

import argparse
import json
import logging
import sys

try:
    from lxml import etree
except ImportError:
    import xml.etree.ElementTree as etree


class ValidationError(Exception):
    """The exception is thrown if countries validation failed."""


PLACE_TAGS = ('place', 'name', 'name:en')
BOUNDARY_TAGS = ('name', 'name:en', 'type', 'boundary', 'admin_level')
COUNTRY_PROPERTIES = ('boundary', 'label', 'capital')


def update_tag_value(tags, tag_xml_element):
    """Updates 'tags' dict with tag_xml_element attributes."""
    tag_name = tag_xml_element.get('k')
    if tag_name in tags:
        tag_value = tag_xml_element.get('v')
        tags[tag_name] = tag_value


def places_generator(osm_filename):
    """osm_filename is an *.osm extract of countries and capitals."""

    for event, element in etree.iterparse(osm_filename):
        feature_type = element.tag
        if feature_type not in ('node', 'way', 'relation'):
            continue
        tags = dict.fromkeys(PLACE_TAGS)
        for child in element:
            if child.tag == 'tag':
                update_tag_value(tags, child)
        if tags['place'] is None:
            continue
        feature = {
            'id': f"{feature_type[0]}{element.get('id')}",
            'tags': tags
        }
        yield feature

        # If we don't need xml document tree it makes sense to clear
        # elements to save memory.
        element.clear()


def boundaries_generator(osm_filename):
    """osm_filename is an *.osm extract of countries and capitals.
    If 'place' tag is on boundary, the feature is yielded
    both by this generator and by 'places_generator'.
    """

    for event, element in etree.iterparse(osm_filename):
        feature_type = element.tag
        if feature_type != 'relation':
            continue
        tags = dict.fromkeys(BOUNDARY_TAGS)
        admin_centres = []
        labels = []
        for child in element:
            if child.tag == 'tag':
                update_tag_value(tags, child)
            elif child.tag == 'member':
                role = child.get('role')
                target_list = (admin_centres if role == 'admin_centre' else
                                      labels if role == 'label' else
                                      None)
                if target_list is not None:
                    target_list.append({attr: child.get(attr)
                                            for attr in ('type', 'ref')})
        if (tags['type'] != 'boundary' or
                tags['boundary'] != 'administrative' or
                tags['admin_level'] != '2'):
            continue
        feature = {
            'id': f"{feature_type[0]}{element.get('id')}",
            'tags': tags,
            'admin_centres': admin_centres,
            'labels': labels
        }
        yield feature
        element.clear()


def extract_places(countries_osm_file):
    """Returns OSM features with 'place' tag, separately for place=country
    and all other places.
    """

    # id => { 'id': el_id, 'name': name, 'name:en': name:en, 'place': place }
    country_places = {}

    # id => { 'id': el_id, 'name': name, 'name:en': name:en, 'place': place }
    noncountry_places = {}

    # { (place, name:en): [ids] }
    places_by_name = {}

    for place in places_generator(countries_osm_file):
        el_id = place['id']
        name, nameen, place = (
            place['tags'][tag] for tag in ('name', 'name:en', 'place')
        )
        if not name and not nameen:
            logging.error(f"Place {el_id} without name and name:en")
            continue
        elif not nameen:
            logging.warning(f"Place {el_id} '{name}' without name:en")
        else:
            places_by_name.setdefault((place, nameen),[]).append(el_id)
        target_dict = (country_places if place == 'country' else
                       noncountry_places)
        target_dict[el_id] = {
            'id': el_id,
            'name': name,
            'name:en': nameen,
            'place': place
        }
        if (place != 'country' and
            place not in ('city', 'town', 'hamlet',
                          'village', 'municipality')
        ):
            logging.warning(f"Not-settlement capital place '{place}' "
                            f"{el_id}")

    double_places = [(k, v) for k, v in places_by_name.items() if len(v) > 1]
    if double_places:
        double_places_str = json.dumps(
            sorted(double_places, key=lambda t: t[0][1]),
            ensure_ascii=False,
            indent=2
        )
        logging.info(f"Double places:\n{double_places_str}")

    return country_places, noncountry_places


def find_capital(boundary, noncountry_places):
    name, nameen = (boundary['tags'][tag] for tag in ('name', 'name:en'))
    display_name = nameen or name
    el_id = boundary['id']
    num_admin_centres = len(boundary['admin_centres'])
    if num_admin_centres == 1:
        admin_centre = boundary['admin_centres'][0]
        if admin_centre['type'] != 'node':
            logging.info(f"Not node as admin_centre in rel {el_id} "
                         f"'{display_name}'")
        ref = f"{admin_centre['type'][0]}{admin_centre['ref']}"
        if ref not in noncountry_places:
            logging.error(f"admin_centre {ref} for rel {el_id} "
                          f"'{display_name}' is not found "
                          "among non-country places")
        else:
            return noncountry_places[ref]
    else:
        logging.error(
            f"More than one admin_centre in rel {el_id} '{display_name}'"
            if num_admin_centres > 1 else
            f"No admin_centre set for boundary {el_id} '{display_name}'"
        )
    return None


def find_single_label(boundary, country_places, used_country_places):
    """Given the boundary has only one label member, validates the label
    and, if it is good, returns corresponding country place."""
    name, nameen = (boundary['tags'][tag] for tag in ('name', 'name:en'))
    display_name = nameen or name
    el_id = boundary['id']
    label = boundary['labels'][0]
    if label['type'] != 'node':
        logging.warning(f"Not node as label in rel {el_id} '{display_name}'")
    else:
        ref = f"{label['type'][0]}{label['ref']}"
        if ref not in country_places:
            logging.error(f"Label node {ref} is not found in "
                          f"country_places for {el_id} '{display_name}'")
        else:
            if ref in used_country_places:
                logging.error(f"Country place node {ref} is used "
                              "more than in one country boundary")
            return country_places[ref]
    return None


def find_unbound_label(boundary, country_places):
    """Finds label for country boundary in case boundary relation
    doesn't contain label member."""
    nameen = boundary['tags']['name:en']
    if not nameen:
        return None
    fit_country_places = {k: v for k, v in country_places.items()
                          if v['name:en'] == nameen}
    num_fit_country_places = len(fit_country_places)
    if num_fit_country_places == 1:
        label = list(fit_country_places.values())[0]
        return label
    else:
        logging.error(
            f"country_places with the same name: {fit_country_places}"
            if num_fit_country_places > 1 else
            f"No label found for rel_id {boundary['id']} '{nameen}'"
        )
        return None


def find_label(boundary, country_places, used_country_places):
    name, nameen = (boundary['tags'][tag] for tag in ('name', 'name:en'))
    display_name = nameen or name
    el_id = boundary['id']
    num_labels = len(boundary['labels'])
    if num_labels == 1:
        label = find_single_label(boundary, country_places, used_country_places)
        if label:
            used_country_places.add(label['id'])
        return label
    if num_labels > 1:
        logging.error(f"More than one label in rel {el_id} '{display_name}'")
        return None
    logging.warning(f"No label set for country {el_id} '{display_name}'")
    unbound_label = find_unbound_label(boundary, country_places)
    if not unbound_label:
        return None
    used_country_places.add(unbound_label['id'])
    return unbound_label


def make_country_billet(boundary_id, name, nameen):
    return {
        'boundary': {
            'id': boundary_id,
            'name': name,
            'name:en': nameen
        },
        'capital': None,
        'label': None
    }


def extract_country(boundary, countries_by_name, country_places,
                    noncountry_places, used_country_places):
    el_id = boundary['id']
    name, nameen, type_, admin_level = (
        boundary['tags'][tag] for tag in ('name', 'name:en',
                                          'type', 'admin_level')
    )
    if not name and not nameen:
        logging.error(f"Boundary without name and name:en, rel_id {el_id}")
        return None
    if not name:
        logging.warning(f"Country without name, rel_id {el_id}")
    if not nameen:
        logging.warning(f"Country without name:en, rel_id {el_id}")
    if 'place' in boundary['tags']:
        logging.warning(f"Place '{boundary['tags']['place']}' on "
                        f"boundary relation {el_id} '{nameen or name}'")

    if nameen and nameen in countries_by_name:
        logging.error(f"Duplicate country names: {el_id} "
                      f"{countries_by_name[nameen]['boundary']['id']}")
        return None

    country = make_country_billet(el_id, name, nameen)
    if nameen:
        countries_by_name[nameen] = country
    country['capital'] = find_capital(boundary, noncountry_places)
    country['label'] = find_label(boundary, country_places, used_country_places)
    return country


def extract_boundaries(countries_osm_file, country_places, noncountry_places):
    """Reads *.osm file with countries and capitals and logs many
    validity checks with different level (lower than CRITICAL). Returns a
    dict of found countries.
    }
    """

    # Structure of 'osm_countries' dict is described in
    # doc-string for 'validate_countries' function.
    osm_countries = {}

    # "used" means used as label in some country boundary relation
    used_country_places = set()

    # name:en => the same object as in 'osm_countries' dict
    countries_by_name = {}

    for boundary in boundaries_generator(countries_osm_file):
        country = extract_country(boundary, countries_by_name, country_places,
                                  noncountry_places, used_country_places)
        if country is not None:
            boundary_id = country['boundary']['id']
            osm_countries[boundary_id] = country
            nameen = country['boundary']['name:en']
            if nameen:
                countries_by_name[nameen] = country

    unused_country_places = set(country_places.keys()) - used_country_places
    if unused_country_places:
        unused_country_places_str = json.dumps(
            sorted(
                [country_places[c_id] for c_id in unused_country_places],
                key=lambda t: t['name:en'] or t['name'] or t['id']
            ),
            ensure_ascii=False,
            indent=2
        )
        logging.info(f"Unused country places:\n{unused_country_places_str}")

    return osm_countries


def extract_countries(countries_osm_file):
    """ Returns a dict of found countries in the form:
    boundary_id: {
        'boundary': {'id': str, 'name': str, 'name:en': str},
        'label':    {'id': str, 'name': str, 'name:en': str, 'place': str},
        'capital':  {'id': str, 'name': str, 'name:en': str, 'place': str}
    }"""

    country_places, noncountry_places = extract_places(countries_osm_file)

    osm_countries = extract_boundaries(countries_osm_file,
                                       country_places,
                                       noncountry_places)
    return osm_countries


def is_country_equal_to_reference(country, ref_country):
    equal = True
    country_name = ref_country['boundary']['name:en']
    for property in COUNTRY_PROPERTIES:
        if country[property] is None:
            logging.critical(f"'{property}' is empty "
                             f"for country {country_name}")
            equal = False
        else:
            for key in ref_country[property].keys():
                if country[property][key] != ref_country[property][key]:
                    msg = (
                        f"Different '{key}' of {property} "
                        f"for country {country_name}"
                    )
                    if key in ('name:en', 'place'):
                        logging.critical(msg)
                        equal = False
                    else:
                        logging.warning(msg)
    return equal


def map_reference_countries_to_osm(osm_countries, reference_countries):
    """For each country name from reference_countries
    find corresponding country from OSM, or set None if not found.
    """

    # Country name:en => osm country object
    ref2osm_country_mapping = dict.fromkeys(reference_countries.keys())

    for country_name, reference_country in reference_countries.items():
        reference_country_id = reference_country['boundary']['id']
        fit_country = osm_countries.get(reference_country_id, None)
        if (not fit_country or not (
                fit_country['label'] and
                fit_country['label']['name:en'] == country_name)):
            fit_country = None
            fit_countries = {k: v for k, v in osm_countries.items()
                             if v['label'] and
                                v['label']['name:en'] == country_name}
            if len(fit_countries) == 0:
                logging.critical(f"Country '{country_name}' not found")
            elif len(fit_countries) > 1:
                logging.critical("Found more than one country "
                        f"'{country_name}': {list(fit_countries.keys())}")
            else:
                fit_country = list(fit_countries.values())[0]
        if fit_country:
            ref2osm_country_mapping[country_name] = {
                'osm_country': fit_country,
                'is_equal': is_country_equal_to_reference(fit_country,
                                                          reference_country)
            }

    return ref2osm_country_mapping


def is_country_in_list(country, names):
    """Does country have any name from 'names' list among its name/name:en tags
    of boundary/label.
    """
    return any(
                any(
                    country[property].get(name_tag) in names
                        for name_tag in ('name', 'name:en')
                ) for property in ('label', 'boundary') if country[property]
    )


def validate_countries(osm_countries,
                       reference_countries,
                       whitelist_countries):
    """
    osm_countries: dict returned by extract_countries.

    reference_countries: dict with reference countries; its structure is
    described in doc-string to 'read_reference_data' function.

    whitelist_countries: list of country names (english or local)
    which may be found or not in planet, indifferently.

    The function checks that all countries from reference list also present in
    OSM. It informs if its boundary/label/capital ID/place/name/name:en was
    changed. Also informs about appearance of new countries besides those from
    whitelist.
    """

    ref2osm_country_mapping = map_reference_countries_to_osm(
                                osm_countries, reference_countries)

    lost_countries_detected = (None in ref2osm_country_mapping.values())

    bad_countries_detected = any(
        data['is_equal'] == False
        for country_name, data in ref2osm_country_mapping.items()
        if data is not None
    )

    matched_osm_country_ids = set(
        data['osm_country']['boundary']['id']
        for country_name, data in ref2osm_country_mapping.items()
        if data is not None
    )
    superfluous_countries = [
        country
        for osm_boundary_id, country in osm_countries.items()
        if country['boundary']['id'] not in matched_osm_country_ids
    ]
    unknown_country_ids = []
    for country in superfluous_countries:
        country_is_in_whitelist = is_country_in_list(country,
                                                     whitelist_countries)
        if not country_is_in_whitelist:
            unknown_country_ids.append(country['boundary']['id'])
    if unknown_country_ids:
        for country_id in unknown_country_ids:
            logging.critical(f"Unknown country with boundary id {country_id}")

    errors = []
    if lost_countries_detected:
        errors.append("Not all reference countries found in OSM")
    if bad_countries_detected:
        errors.append("Not all reference countries matched with OSM")
    if unknown_country_ids:
        errors.append("Unknown countries found")

    if errors:
        error_msg = '; '.join(errors)
        raise ValidationError(error_msg)


def read_reference_data(reference_countries_file, whitelist_file):
    """Reference countries file must be a json file with a dict with items
    of the form:
    "Andorra": {
        "boundary": {
          "id": "r9407",
          "name": "Andorra",
          "name:en": "Andorra"
        },
        "label": {
          "id": "n148874198",
          "name": "Andorra",
          "name:en": "Andorra",
          "place": "country"
        },
        "capital": {
          "id": "n58957648",
          "name": "Andorra la Vella",
          "name:en": "Andorra la Vella",
          "place": "town"
        }
    }

    Whitelist is a list of country names (english or local)
    which may be found or not in planet, indifferently.
    """

    with open(reference_countries_file) as f:
        reference_countries = json.load(f)

    # Ensure that reference country list contains full info.
    for country_name, country in reference_countries.items():
        for property in COUNTRY_PROPERTIES:
            for name_tag in ('name', 'name:en'):
                if not country[property].get(name_tag):
                    raise ValidationError(f"No {name_tag} for {property} "
                                          f"for country {country_name} "
                                          f"in reference country list")

    with open(whitelist_file) as f:
        whitelist_countries = set(f.read().splitlines())

    # Ensure that no reference country also belongs to whitelist.
    for country_name, country in reference_countries.items():
        country_is_in_whitelist = is_country_in_list(country,
                                                     whitelist_countries)
        if country_is_in_whitelist:
            raise ValidationError(
                f"Country {country_name} "
                "is in reference list and whitelist simultaneously"
            )

    return reference_countries, whitelist_countries


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    log_levels = [
        name.lower()
        for number, name in logging._levelToName.items()
        if number > 0
    ]
    parser.add_argument('-L', '--log-level', choices=log_levels,
                        default='critical', help='log level')
    parser.add_argument('-x', '--xml', required=True,
                        help='Path to *.osm with countries/capitals')
    parser.add_argument('-r', '--required-countries', required=True,
                        help='Path to json file with required countries. '
                             'Keys are name:en of country labels')
    parser.add_argument('-w', '--whitelist',
                        help='Path to txt file with country names '
                             'which may be ignored')
    parser.add_argument('-o', '--output', required=False,
                        help='File name to output found countries as json')
    options = parser.parse_args()

    log_level_name = options.log_level.upper()
    logging.basicConfig(level=getattr(logging, log_level_name),
                        format='%(levelname)-8s  %(message)s')

    try:
        osm_countries = extract_countries(options.xml)
        if options.output:
            with open(options.output, 'w') as f:
                json.dump(osm_countries, f, ensure_ascii=False, indent=2)

        reference_countries, whitelist_countries = read_reference_data(
            options.required_countries,
            options.whitelist
        )

        validate_countries(osm_countries,
                           reference_countries,
                           whitelist_countries)
    except ValidationError as e:
        logging.critical(e)
        sys.exit(1)
    except Exception as e:
        logging.critical("", exc_info=1)
        sys.exit(1)

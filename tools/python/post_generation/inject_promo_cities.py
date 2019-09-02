import json
import logging
import os
import re
import sys

from multiprocessing import Pool

from mwm import mwm


class PromoCities(object):
    def __init__(self, cities, mwm_path, types_path, osm2ft_path):
        self.cities = cities
        self.mwm_path = mwm_path
        self.types_path = types_path
        self.osm2ft_path = osm2ft_path

    def find(self, leaf_id):
        result = []
        ft2osm = load_osm2ft(self.osm2ft_path, leaf_id)
        with open(os.path.join(self.mwm_path, leaf_id + ".mwm"), "rb") as f:
            mwm_file = mwm.MWM(f)
            mwm_file.read_header()
            mwm_file.read_types(self.types_path)
            for feature in mwm_file.iter_features(metadata=True):
                osm_id = ft2osm.get(feature["id"], None)
                types = feature["header"]["types"]

                if "sponsored-promo_catalog" not in types or osm_id not in self.cities:
                    continue

                city = {
                    "id": osm_id,
                    "count_of_guides": self.cities[osm_id],
                    "types": []
                }

                for t in types:
                    if t.startswith("place"):
                        city["types"].append(t)

                if not city["types"]:
                    logging.error(f"Incorrect types for sponsored-promo_catalog "
                                  f"feature osm_id {osm_id}")
                    sys.exit(3)

                result.append(city)

        return result

    @staticmethod
    def choose_best_city(proposed_cities):
        def key_compare(city):
            return city["count_of_guides"], score_types(city["types"])

        return max(proposed_cities, key=key_compare)


def place_type_to_int(t):
    if t == "place-town":
        return 1
    if t == "place-city":
        return 2

    m = re.match(r"^place-city-capital?(-(?P<admin_level>\d+)|)$", t)
    if m:
        admin_level = int(m.groupdict("1")["admin_level"])
        if 1 <= admin_level <= 12:
            return 14 - admin_level
    return 0


def score_types(types):
    return max([place_type_to_int(t) for t in types])


def load_cities(path):
    with open(path) as f:
        cities_list = json.load(f)

    cities = {}
    for city in cities_list["data"]:
        cities[city["osmid"]] = city["paid_bundles_count"]

    return cities


def load_osm2ft(osm2ft_path, mwm_id):
    osm2ft_name = os.path.join(osm2ft_path, mwm_id + ".mwm.osm2ft")
    if not os.path.exists(osm2ft_name):
        logging.error(f"Cannot find {osm2ft_name}")
        sys.exit(3)
    with open(osm2ft_name, "rb") as f:
        return mwm.read_osm2ft(f, ft2osm=True, tuples=False)


def get_nodes(node):
    def _get_nodes(node, mwm_nodes):
        if "g" in node:
            for item in node["g"]:
                _get_nodes(item, mwm_nodes)
        else:
            mwm_nodes.append(node)

    mwm_nodes = []
    _get_nodes(node, mwm_nodes)
    return mwm_nodes


def inject_into_leafs(node, cities):
    nodes = get_nodes(node)
    with Pool() as pool:
        proposed_cities_list = pool.map(cities.find, (n["id"] for n in nodes),
                                        chunksize=1)
    for idx, proposed_cities in enumerate(proposed_cities_list):
        if not proposed_cities:
            continue
        node = nodes[idx]
        best = cities.choose_best_city(proposed_cities)
        node["top_city_geo_id"] = best["id"]
        if best["id"] < 0:
            node["top_city_geo_id"] += (1 << 64)


def inject_promo_cities(countries_json, promo_cities_path, mwm_path, types_path,
                        osm2ft_path):
    cities = PromoCities(load_cities(promo_cities_path), mwm_path, types_path,
                         osm2ft_path)
    inject_into_leafs(countries_json, cities)

import json
import logging
import os
import re
import sys
from multiprocessing import Pool

from mwm import Mwm
from mwm.ft2osm import read_osm2ft


class PromoIds(object):
    def __init__(self, countries, cities, mwm_path, types_path, osm2ft_path):
        self.countries = countries
        self.cities = cities
        self.mwm_path = mwm_path
        self.types_path = types_path
        self.osm2ft_path = osm2ft_path

    def inject_into_country(self, country):
        nodes = self._get_nodes(country)
        with Pool() as pool:
            proposed_ids = pool.map(self._find, (n["id"] for n in nodes), chunksize=1)

        countries_ids = [
            ids for node_ids in proposed_ids for ids in node_ids["countries"]
        ]
        if countries_ids:
            country["top_countries_geo_ids"] = countries_ids

        for idx, node_ids in enumerate(proposed_ids):
            if not node_ids["cities"]:
                continue
            node = nodes[idx]
            best = self._choose_best_city(node_ids["cities"])
            node["top_city_geo_id"] = best["id"]
            if best["id"] < 0:
                node["top_city_geo_id"] += 1 << 64

    def _find(self, leaf_id):
        result = {"countries": [], "cities": []}
        ft2osm = load_osm2ft(self.osm2ft_path, leaf_id)

        for feature in Mwm(os.path.join(self.mwm_path, leaf_id + ".mwm")):
            osm_id = ft2osm.get(feature.index(), None)
            types = feature.readable_types()

            if "sponsored-promo_catalog" in types and osm_id in self.cities:
                city = self._get_city(osm_id, types)
                result["cities"].append(city)

            if "place-country" in types and osm_id in self.countries:
                result["countries"].append(osm_id)

        return result

    @staticmethod
    def _get_nodes(root):
        def __get_nodes(node, mwm_nodes):
            if "g" in node:
                for item in node["g"]:
                    __get_nodes(item, mwm_nodes)
            else:
                mwm_nodes.append(node)

        mwm_nodes = []
        __get_nodes(root, mwm_nodes)
        return mwm_nodes

    def _get_city(self, osm_id, types):
        city = {"id": osm_id, "count_of_guides": self.cities[osm_id], "types": []}

        for t in types:
            if t.startswith("place"):
                city["types"].append(t)

        if not city["types"]:
            logging.error(
                f"Incorrect types for sponsored-promo_catalog "
                f"feature osm_id {osm_id}"
            )
            sys.exit(3)

        return city

    def _choose_best_city(self, proposed_cities):
        def key_compare(city):
            return city["count_of_guides"], self._score_city_types(city["types"])

        return max(proposed_cities, key=key_compare)

    def _score_city_types(self, types):
        return max(self._city_type_to_int(t) for t in types)

    @staticmethod
    def _city_type_to_int(t):
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


def load_promo_ids(path):
    with open(path) as f:
        root = json.load(f)

    ids = {}
    for item in root["data"]:
        ids[item["osmid"]] = item["paid_bundles_count"]

    return ids


def load_osm2ft(osm2ft_path, mwm_id):
    osm2ft_name = os.path.join(osm2ft_path, mwm_id + ".mwm.osm2ft")
    if not os.path.exists(osm2ft_name):
        logging.error(f"Cannot find {osm2ft_name}")
        sys.exit(3)
    with open(osm2ft_name, "rb") as f:
        return read_osm2ft(f, ft2osm=True, tuples=False)


def inject_promo_ids(
    countries_json,
    promo_cities_path,
    promo_countries_path,
    mwm_path,
    types_path,
    osm2ft_path,
):
    promo_ids = PromoIds(
        load_promo_ids(promo_countries_path),
        load_promo_ids(promo_cities_path),
        mwm_path,
        types_path,
        osm2ft_path,
    )
    for country in countries_json["g"]:
        promo_ids.inject_into_country(country)

# Produces countries.txt from hierarchy.txt
#
# Hierarchy.txt format:
#
# Sample lines:
# Iran;Q794;ir;fa
#  Iran_South;Q794-South
#
# Number of leading spaces mean hierarchy depth. In above case, Iran_South is inside Iran.
# Then follows a semicolon-separated list:
# 1. MWM file name without extension
# 2. Region name template using wikidata Qxxx codes and predefined strings
# 3. Country ISO code (used for flags in the legacy format)
# 4. Comma-separated list of language ISO codes for the region

import base64
import hashlib
import json
import os.path
import re


class CountryDict(dict):
    def __init__(self, *args, **kwargs):
        dict.__init__(self, *args, **kwargs)
        self.order = ["id", "n", "v", "c", "s", "sha1_base64", "rs", "g"]

    def __iter__(self):
        for key in self.order:
            if key in self:
                yield key
        for key in dict.__iter__(self):
            if key not in self.order:
                yield key

    def iteritems(self):
        for key in self.__iter__():
            yield (key, self.__getitem__(key))


def get_mwm_hash(path, name):
    filename = os.path.join(path, f"{name}.mwm")
    h = hashlib.sha1()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            h.update(chunk)
    return str(base64.b64encode(h.digest()), "utf-8")


def get_mwm_size(path, name):
    filename = os.path.join(path, f"{name}.mwm")
    return os.path.getsize(filename)


def collapse_single(root):
    for i in range(len(root["g"])):
        if "g" in root["g"][i]:
            if len(root["g"][i]["g"]) == 1:
                # replace group by a leaf
                if "c" in root["g"][i]:
                    root["g"][i]["g"][0]["c"] = root["g"][i]["c"]
                root["g"][i] = root["g"][i]["g"][0]
            else:
                collapse_single(root["g"][i])


def get_name(leaf):
    if "n" in leaf:
        return leaf["n"].lower()
    else:
        return leaf["id"].lower()


def sort_tree(root):
    root["g"].sort(key=get_name)
    for leaf in root["g"]:
        if "g" in leaf:
            sort_tree(leaf)


def parse_old_vs_new(old_vs_new_csv_path):
    oldvs = {}
    if not old_vs_new_csv_path:
        return oldvs

    with open(old_vs_new_csv_path) as f:
        for line in f:
            m = re.match(r"(.+?)\t(.+)", line.strip())
            assert m
            if m.group(2) in oldvs:
                oldvs[m.group(2)].append(m.group(1))
            else:
                oldvs[m.group(2)] = [m.group(1)]
    return oldvs


def parse_borders_vs_osm(borders_vs_osm_csv_path):
    vsosm = {}
    if not borders_vs_osm_csv_path:
        return vsosm

    with open(borders_vs_osm_csv_path) as f:
        for line in f:
            m = re.match(r"(.+)\t(\d)\t(.+)", line.strip())
            assert m
            if m.group(1) in vsosm:
                vsosm[m.group(1)].append(m.group(3))
            else:
                vsosm[m.group(1)] = [m.group(3)]
    return vsosm


def parse_countries_synonyms(countries_synonyms_csv_path):
    countries_synonyms = {}
    if not countries_synonyms_csv_path:
        return countries_synonyms

    with open(countries_synonyms_csv_path) as f:
        for line in f:
            m = re.match(r"(.+)\t(.+)", line.strip())
            assert m
            if m.group(1) in countries_synonyms:
                countries_synonyms[m.group(1)].append(m.group(2))
            else:
                countries_synonyms[m.group(1)] = [m.group(2)]
    return countries_synonyms


def hierarchy_to_countries(
    old_vs_new_csv_path,
    borders_vs_osm_csv_path,
    countries_synonyms_csv_path,
    hierarchy_path,
    target_path,
    version,
):
    def fill_last(last, stack):
        name = last["id"]
        if not os.path.exists(os.path.join(target_path, f"{name}.mwm")):
            return
        last["s"] = get_mwm_size(target_path, name)
        last["sha1_base64"] = get_mwm_hash(target_path, name)
        if last["s"] >= 0:
            stack[-1]["g"].append(last)

    oldvs = parse_old_vs_new(old_vs_new_csv_path)
    vsosm = parse_borders_vs_osm(borders_vs_osm_csv_path)
    countries_synonyms = parse_countries_synonyms(countries_synonyms_csv_path)
    stack = [CountryDict(v=int(version), id="Countries", g=[])]
    last = None
    with open(hierarchy_path) as f:
        for line in f:
            m = re.match("( *)(.+)", line)
            assert m
            depth = len(m.group(1))
            if last is not None:
                lastd = last["d"]
                del last["d"]
                if lastd < depth:
                    # last is a group
                    last["g"] = []
                    stack.append(last)
                else:
                    fill_last(last, stack)
            while depth < len(stack) - 1:
                # group ended, add it to higher group
                g = stack.pop()
                if len(g["g"]) > 0:
                    stack[-1]["g"].append(g)
            items = m.group(2).split(";")
            last = CountryDict({"id": items[0], "d": depth})
            if items[0] in oldvs:
                last["old"] = oldvs[items[0]]
            if items[0] in vsosm:
                last["affiliations"] = vsosm[items[0]]
            if items[0] in countries_synonyms:
                last["country_name_synonyms"] = countries_synonyms[items[0]]

    # the last line is always a file
    del last["d"]
    fill_last(last, stack)
    while len(stack) > 1:
        g = stack.pop()
        if len(g["g"]) > 0:
            stack[-1]["g"].append(g)

    collapse_single(stack[-1])
    return stack[-1]

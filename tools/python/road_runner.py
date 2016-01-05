#!/usr/bin/python
# -*- coding: utf-8 -*-

import json
import os
import sys
import urllib2

road_delta = 50

def get_way_ids(point1, point2, server):
    url = "http://{0}/wayid?z=18&loc={1},{2}&loc={3},{4}".format(server, point1[0], point1[1], point2[0], point2[1])
    request = urllib2.urlopen(url)
    data = json.load(request)
    if "way_ids" in data:
        return data["way_ids"]
    return []

def each_to_each(points):
    result = []
    for i in range(len(points)):
        for j in range(len(points) - i - 1):
            result.append((points[i], points[j + i + 1]))
    return result

def load_towns(path):
    result = []
    with open(path, "r") as f:
        for line in f:
            data = line.split(";")
            result.append((float(data[0]), float(data[1])))
    return result


if len(sys.argv) < 3:
    print "road_runner.py <intermediate_dir> <osrm_addr>"
    exit(1)

towns = load_towns(os.path.join(sys.argv[1], "towns.csv"))
print "Have {0} towns".format(len(towns))

tasks = each_to_each(towns)
filtered = []
for p1, p2 in tasks:
    if (p1[0]-p2[0])**2 + (p1[1]-p2[1])**2 < road_delta:
        filtered.append((p1,p2))
tasks = filtered

way_ids = {}
for i, task in enumerate(tasks):
    if not i % 1000:
        print i,"/", len(tasks)
    ids = get_way_ids(task[0], task[1], sys.argv[2])
    for id in ids:
        if id in way_ids:
            way_ids[id] += 1
        else:
            way_ids[id] = 1

with open(os.path.join(sys.argv[1], "ways.csv"),"w") as f:
    for way_id in way_ids.keys():
        print >> f, "{0};world_level".format(way_id)

print "All done."

#!/usr/bin/env python3

# Tool shows the following information about classes
#  - zooms range, for classes with a style
#  - classes without a style

import csv
import sys

import drules_struct_pb2


def GetAllClasses(mapping_path):
    class_order = []
    for row in csv.reader(open(mapping_path), delimiter=';'):
        if not row or row[0].startswith('#'):
            continue
        cl = row[0].replace("|", "-")
        if row[2] != "x":
            class_order.append(cl)
    class_order.sort()
    return class_order


def GetClassesZoomRange(drules_path):
    drules = drules_struct_pb2.ContainerProto()
    with open(drules_path, 'rb') as f:
        drules.ParseFromString(f.read())
    result = {}
    for rule in drules.cont:
        name = str(rule.name)
        zooms = [-1, -1]
        for elem in rule.element:
            if elem.scale >= 0:
                if zooms[0] == -1 or elem.scale < zooms[0]:
                    zooms[0] = elem.scale
                if zooms[1] == -1 or elem.scale > zooms[1]:
                    zooms[1] = elem.scale
        if zooms[0] != -1:
            if name in result:
                zooms[0] = min(zooms[0], result[name][0])
                zooms[1] = max(zooms[1], result[name][1])
            result[name] = zooms
    return result


def ShowZoomInfo(mapping_path, drules_path):
    classes_order = GetAllClasses(mapping_path)
    classes_zooms = GetClassesZoomRange(drules_path)

    classes_with_style = []
    classes_without_style = []

    for c in classes_order:
        if c in classes_zooms:
            classes_with_style.append(c)
        else:
            classes_without_style.append(c)

    classes_with_style.sort()
    classes_without_style.sort()

    print(f"Classes with a style: {len(classes_with_style)}")
    for c in classes_with_style:
        print(f"\t{c} - [{str(classes_zooms[c][0])}, {str(classes_zooms[c][1])}]")
    print()
    print(f"Classes without a style ({len(classes_without_style)}):")
    for c in classes_without_style:
        print(f"\t{c}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage:")
        print(f"{sys.argv[0]} <mapcss_mapping_file> <drules_proto_file>")
        exit(-1)

    mapping_path = sys.argv[1]
    drules_path = sys.argv[2]
    ShowZoomInfo(mapping_path, drules_path)

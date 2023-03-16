# Tool shows following information about classes
#  - zooms range, for classes with a style
#  - classes without a style

# If path to the protobuf EGG is specified then apply it before import drules_struct_pb2
import os
import sys
PROTOBUF_EGG_PATH = os.environ.get("PROTOBUF_EGG_PATH")
if PROTOBUF_EGG_PATH:
    # another version of protobuf may be installed, override it
    for i in range(len(sys.path)):
        if -1 != sys.path[i].find("protobuf-"):
            sys.path[i] = PROTOBUF_EGG_PATH
    sys.path.append(PROTOBUF_EGG_PATH)

import sys
import csv
import drules_struct_pb2

def GetAllClasses(mapping_path):
    class_order = []
    for row in csv.reader(open(mapping_path), delimiter=';'):
        cl = row[0].replace("|", "-")
        if row[2] != "x":
            class_order.append(cl)
    class_order.sort()
    return class_order

def GetClassesZoomRange(drules_path):
    drules = drules_struct_pb2.ContainerProto()
    drules.ParseFromString(open(drules_path).read())
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
                if result[name][0] < zooms[0]:
                    zooms[0] = result[name][0]
                if result[name][1] > zooms[1]:
                    zooms[1] = result[name][1]
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

    print "Classes with a style: (%s)" % (str(len(classes_with_style)))
    for c in classes_with_style:
        print "    %s - [%s, %s]" % (c, str(classes_zooms[c][0]), str(classes_zooms[c][1]))

    print ""
    print "Classes without a style (%s):" % (len(classes_without_style))
    for c in classes_without_style:
        print "    %s" % c

if __name__ == "__main__":

    if len(sys.argv) < 3:
        print "Usage:"
        print "drules_info.py <mapcss_mapping_file> <drules_proto_file>"
        exit(-1)

    mapping_path = sys.argv[1]
    drules_path = sys.argv[2]
    ShowZoomInfo(mapping_path, drules_path)

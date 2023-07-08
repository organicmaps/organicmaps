#!/usr/bin/env python
# -*- coding: utf-8 -*-
import csv
import os

import drules_struct_pb2
from webcolors.webcolors import hex_to_color_name
from operator import *
from copy import *

import sys
reload(sys)
sys.setdefaultencoding("utf-8")

DATADIR = "../../../data"
files = {
    "mapcss_mapping": os.path.join(DATADIR, 'mapcss-mapping.csv'),
    "drules_proto": os.path.join(DATADIR, 'drules_proto.bin'),
}

MINIMAL_WIDTH = .1
LAST_ZOOM = 17

FLAVOUR = set(["JOSM"])  # "JOSM"
FLAVOUR = []


def number(f):
    if float(int(float(f))) == float(str(f)):
        return int(float(f))
    return float(('%.2f' % float(f)).rstrip('0'))


def color_to_properties(color, prefix=""):
    if prefix:
        prefix += "-"
    colorvar = prefix + "color"
    opacityvar = prefix + "opacity"
    dd = {}
    dd[colorvar] = hex_to_color_name("#" + hex(int(color) % (0x1000000))[2:])  # leave only last 24 bits
    opacity = (255 - (int(color) >> 24)) / 255.
    if opacity != 1:
        dd[opacityvar] = number(opacity)
    return dd

print """
canvas {
    background-color: #f1eee8;
    default-lines: false;
    default-points: false;
}

way::* {
    linejoin: round;
    linecap: round;
    fill-opacity: 0;
    casing-linecap: none;
    text-position: line;
}

*::* {
    text-halo-color: white;
    text-anchor-horizontal: center;
    text-anchor-vertical: center;
}

node::* {
    text-anchor-vertical: top;
}

area::* {
    text-position: center;
    text-anchor-vertical: center;
}

area[landuse],
area[natural],
area[leisure],
area[place] {fill-position: background}

"""

if True:
    classificator = {}
    classificator_mapping = {}
    class_order = []
    for row in csv.reader(open(files['mapcss_mapping']), delimiter=';'):
        try:
            pairs = [i.strip(']').split("=") for i in row[1].split(',')[0].split('[')]
        except:
            print row
        classificator_mapping[row[0].replace("|", "-")] = row[1]
        kv = {}
        for i in pairs:
            if len(i) == 1:
                if i[0]:
                    if i[0][0] == "!":
                        kv[i[0][1:].strip('?')] = "no"
                    else:
                        kv[i[0].strip('?')] = "yes"
            else:
                kv[i[0].strip('?')] = i[1]
        classificator[row[0].replace("|", "-")] = kv
        if row[2] != "x":
            class_order.append(row[0].replace("|", "-"))
    class_order.sort()
    drules = drules_struct_pb2.ContainerProto()
    drules.ParseFromString(open(files["drules_proto"]).read())
    names = set()
    linejoins = {drules_struct_pb2.BEVELJOIN: "bevel", drules_struct_pb2.ROUNDJOIN: "round", drules_struct_pb2.NOJOIN: "none"}
    linecaps = {drules_struct_pb2.SQUARECAP: "square", drules_struct_pb2.ROUNDCAP: "round", drules_struct_pb2.BUTTCAP: "none"}
    deduped_sheet = {}
    for elem in drules.cont:
        visible_on_zooms = []
        if elem.name not in class_order and elem.element:
            print >> sys.stderr, elem.name, "rendered but not in classificator"
            continue
        names.add(elem.name)
        if not elem.element:
            print >> sys.stderr, elem.name, "is not rendered"
            continue
        for el in elem.element:
            etype = set()
            zoom = el.scale
            if zoom <= 0:
                continue
            # if zoom <= 1:
            #    zoom = -1
            visible_on_zooms.append(zoom)
            selector = classificator_mapping[elem.name]
            kvrules = [{}]
            tdashes = False
            for tline in el.lines:
                if tline.pathsym.name or tline.dashdot.dd:
                    tdashes = True
            if len(el.lines) == 2 and not tdashes:  # and not "bridge" in elem.name:
                """
                    line and casing, no dashes
                """
                etype.add("line")
                if el.lines[0].priority < el.lines[1].priority:
                    tline = el.lines[1]
                    tcasing = el.lines[0]
                elif el.lines[0].priority > el.lines[1].priority:
                    tline = el.lines[0]
                    tcasing = el.lines[1]
                else:
                    print >> sys.stderr, elem.name, "has two lines on same z"
                twidth = tline.width
                if twidth < MINIMAL_WIDTH:
                    print >> sys.stderr, elem.name, "has invisible lines on zoom", zoom
                else:
                    # tlinedashes =
                    kvrules[0]["width"] = number(twidth)
                    kvrules[0].update(color_to_properties(tline.color))
                    kvrules[0]["z-index"] = number(tline.priority)
                    tlinedashes = ",".join([str(number(t)) for t in tline.dashdot.dd])
                    if tlinedashes:
                        kvrules[0]["dashes"] = tlinedashes
                    if tline.HasField("cap"):
                        kvrules[0]["linecap"] = linecaps.get(tline.cap, 'round')
                    if tline.HasField("join"):
                        kvrules[0]["linejoin"] = linejoins.get(tline.join, 'round')
                tcasingwidth = (tcasing.width - tline.width) / 2
                if ("width" not in kvrules[0]) and tcasingwidth < MINIMAL_WIDTH:
                    print >> sys.stderr, elem.name, "has invisible casing on zoom", zoom
                else:
                    tcasingdashes = ",".join([str(number(t)) for t in tcasing.dashdot.dd])
                    if tlinedashes != tcasingdashes:
                        kvrules[0]["casing-dashes"] = tcasingdashes
                    kvrules[0]["casing-width"] = number(tcasingwidth)
                    kvrules[0].update(color_to_properties(tcasing.color, "casing"))
                    if tcasing.HasField("cap"):
                        kvrules[0]["casing-linecap"] = linecaps.get(tcasing.cap, 'round')
                    if tcasing.HasField("join"):
                        kvrules[0]["casing-linejoin"] = linejoins.get(tcasing.join, 'round')
            elif len(el.lines) > 0:
                """
                    do we have lines at all?
                """
                etype.add("line")
                for tline in el.lines:
                    tkv = {}
                    twidth = tline.width
                    tlinedashes = ",".join([str(number(t)) for t in tline.dashdot.dd])
                    if twidth < MINIMAL_WIDTH:
                        if not tline.pathsym.name:
                            print >> sys.stderr, elem.name, "has invisible lines on zoom", zoom
                    else:
                        tkv["width"] = number(twidth)
                        tkv.update(color_to_properties(tline.color))
                        tkv["z-index"] = number(tline.priority)
                        if tline.HasField("cap"):
                            tkv["linecap"] = linecaps.get(tline.cap, 'round')
                        if tline.HasField("join"):
                            tkv["linejoin"] = linejoins.get(tline.join, 'round')
                        if tline.dashdot.dd:
                            tkv["dashes"] = tlinedashes
                        for trule in kvrules:
                            if "width" not in trule and tkv["z-index"] == trule.get("z-index", tkv["z-index"]):
                                trule.update(tkv)
                                break
                        else:
                            kvrules.append(tkv)
                    if tline.pathsym.name:
                        kvrules[0]["pattern-image"] = tline.pathsym.name + ".svg"
                        kvrules[0]["pattern-spacing"] = number(tline.pathsym.step - 16)
                        kvrules[0]["pattern-offset"] = number(tline.pathsym.offset)
                        kvrules[0]["z-index"] = number(tline.priority)

            if el.area.color:
                etype.add("area")
                tkv = {}
                tkv["z-index"] = el.area.priority
                tkv.update(color_to_properties(el.area.color, "fill"))

                # if el.area.border.width:
                #    tline = el.area.border
                #    tkv["casing-width"] = str(tline.width)
                #    tkv.update(color_to_properties(tline.color, "casing"))
                #    if tline.dashdot.dd:
                #        tkv["casing-dashes"] = ",".join([str(t) for t in tline.dashdot.dd])
                if not kvrules[0]:
                    kvrules[0] = tkv
                else:
                    kvrules.append(tkv)

            if el.symbol.name:
                if el.symbol.apply_for_type == 0:
                    etype.add("node")
                    etype.add("area")
                elif el.symbol.apply_for_type == 1:
                    etype.add("node")
                elif el.symbol.apply_for_type == 2:
                    etype.add("area")
                kvrules[0]["icon-image"] = el.symbol.name + ".svg"

            if el.circle.radius:
                etype.add("node")
                kvrules[0]["symbol-shape"] = "circle"
                kvrules[0]["symbol-size"] = number(el.circle.radius)
                kvrules[0].update(color_to_properties(el.circle.color, "symbol-fill"))

            if el.caption.primary.height:
                etype.add("node")
                etype.add("area")
                kvrules[0].update(color_to_properties(el.caption.primary.color, "text"))
                if el.caption.primary.stroke_color:
                    kvrules[0].update(color_to_properties(el.caption.primary.stroke_color, "text-halo"))
                    kvrules[0]["text-halo-radius"] = 2
                kvrules[0]["text"] = "name"
                if "building" in selector:
                    kvrules[0]["text"] = "addr:housenumber"
                kvrules[0]["font-size"] = el.caption.primary.height
                if el.caption.primary.offset_y:
                    kvrules[0]["text-offset"] = el.caption.primary.offset_y

            if el.caption.secondary.height:
                tkv = {}
                etype.add("node")
                etype.add("area")
                tkv.update(color_to_properties(el.caption.secondary.color, "text"))
                if el.caption.secondary.stroke_color:
                    tkv.update(color_to_properties(el.caption.secondary.stroke_color, "text-halo"))
                    tkv["text-halo-radius"] = 2
                tkv["text"] = "int_name"
                tkv["font-size"] = el.caption.secondary.height
                if el.caption.primary.offset_y:
                    tkv["text-offset"] = el.caption.secondary.offset_y
                kvrules.append(tkv)

            if el.path_text.primary.height:
                etype.add("line")
                kvrules[0].update(color_to_properties(el.path_text.primary.color, "text"))
                if el.path_text.primary.stroke_color:
                    kvrules[0].update(color_to_properties(el.path_text.primary.stroke_color, "text-halo"))
                    kvrules[0]["text-halo-radius"] = 2
                kvrules[0]["text"] = "name"
                kvrules[0]["text-position"] = "line"
                if "building" in selector:
                    kvrules[0]["text"] = "addr:housenumber"
                kvrules[0]["font-size"] = el.path_text.primary.height

            if el.path_text.secondary.height:
                tkv = {}
                etype.add("line")
                tkv.update(color_to_properties(el.path_text.secondary.color, "text"))
                if el.path_text.secondary.stroke_color:
                    tkv.update(color_to_properties(el.path_text.secondary.stroke_color, "text-halo"))
                    tkv["text-halo-radius"] = 2
                tkv["text"] = "int_name"
                tkv["text-position"] = "line"
                tkv["font-size"] = el.path_text.secondary.height
                kvrules.append(tkv)

            tt = []
            if "[area?]" in selector:
                etype.discard("way")
                etype.discard("line")
                etype.add("area")
                selector = selector.replace("[area?]", "")
            if ("line" in etype) and ("JOSM" in FLAVOUR):
                etype.add("way")
                etype.discard("line")
            for tetype in etype:
                # lzoom = zoom
                # if zoom == LAST_ZOOM:
                    # lzoom = str(zoom)+"-"
                for tsel in selector.split(","):
                    tsel = tsel.strip()
                    # tt.append( "%(tetype)s|z%(lzoom)s%(tsel)s"%(locals()))
                    tt.append([tetype, zoom, zoom, tsel, ''])
            tl = 0
            for kvrul in kvrules:
                if not kvrul:
                    continue
                tsubpart = ""

                filt = {
                    #'z': [['z-index', 'fill-position'], []],
                    #'halo': [['text-halo-radius', 'text-halo-color'], []],
                    'text': [['font-size', 'text-offset', 'text', 'text-color', 'text-position', 'text-halo-radius', 'text-halo-color'], []],
                    #'casing': ['casing-width', 'casing-dashes', 'casing-color'],
                    'icon': [['icon-image', 'symbol-shape', 'symbol-size', 'symbol-fill-color'], ['node', 'area']],
                    'fill': [['fill-color', 'fill-opacity'], ['area']]
                }
                genkv = []
                for k, v in filt.iteritems():
                    f = {}
                    for vi in v[0]:
                        if vi in kvrul:
                            f[vi] = kvrul[vi]
                            del kvrul[vi]
                    if f:
                        genkv.append(f)
                genkv.append(kvrul)
                for kvrule in genkv:
                    tl += 1
                    # print selector
                    if (tl > 1) or ("bridge" in selector) or ("junction" in selector):
                        tsubpart = "::d%sp%s" % (elem.name.count("-"), tl)
                        if ("bridge" in selector):
                            kvrule['z-index'] = tl
                    if "dashes" in kvrule:
                        kvrule["linecap"] = "none"
                    if float(kvrule.get('z-index', 0)) < -5000:
                        kvrule['fill-position'] = 'background'
                        if "z-index" in kvrule:
                            del kvrule['z-index']
                        # kvrule['z-index'] = float(kvrule.get('z-index',0)) + 9962
                    if float(kvrule.get('z-index', 0)) > 10000:
                        kvrule['-x-kot-layer'] = 'top'
                        kvrule['z-index'] = float(kvrule.get('z-index', 0)) - 10000
                    if float(kvrule.get('fill-opacity', 1)) == 0:
                        for discard in ['fill-color', 'fill-opacity', 'z-index', 'fill-position']:
                            if discard in kvrule:
                                del kvrule[discard]
                    if float(kvrule.get('opacity', 1)) == 0:
                        for discard in ['color', 'width', 'z-index']:
                            if discard in kvrule:
                                del kvrule[discard]
                    if float(kvrule.get('z-index', 0)) == 0:
                        if 'z-index' in kvrule:
                            del kvrule['z-index']
                    key = kvrule.copy()
                    # if "z-index" in key:
                    #    del key["z-index"]
                    if not kvrule:
                        continue
                    key = (frozenset(key.items()))
                    minzoom = min([i[1] for i in tt])
                    if key not in deduped_sheet:
                        deduped_sheet[key] = {
                            "sel": [],
                            "maxz": kvrule.get('z-index', 0),
                            "minz": kvrule.get('z-index', 0),
                            "minzoom": minzoom,
                            "z": kvrule.get('z-index', 0),
                            "kv": {}
                        }
                    tt = deepcopy(tt)
                    for t in tt:
                        t[-1] = tsubpart
                    deduped_sheet[key]['sel'].extend(tt)
                    deduped_sheet[key]['maxz'] = max(deduped_sheet[key]['maxz'], kvrule.get('z-index', 0))
                    deduped_sheet[key]['minz'] = max(deduped_sheet[key]['minz'], kvrule.get('z-index', 0))
                    deduped_sheet[key]['z'] = number((deduped_sheet[key]['minz'] + deduped_sheet[key]['maxz']) / 2)
                    deduped_sheet[key]['minzoom'] = min(deduped_sheet[key]['minzoom'], minzoom)
                    deduped_sheet[key]['kv'] = kvrule
            # else:
            #    print >> sys.stderr, selector,  el
        skipped_unstyled_zooms = set(range(min(visible_on_zooms), max(visible_on_zooms) + 1)).difference(visible_on_zooms)
        if skipped_unstyled_zooms:
            print >> sys.stderr, elem.name, "has no styles available for zooms", ", ".join([str(i) for i in skipped_unstyled_zooms])
    # print len(deduped_sheet)
    dds = deduped_sheet.keys()
    dds.sort(lambda k, v: int(deduped_sheet[k]['minzoom'] - deduped_sheet[v]['minzoom']))

    allz = list(set([f['z'] for f in deduped_sheet.values()]))
    allz.sort()
    for tk in dds:
        tv = deduped_sheet[tk]
        tv['sel'].sort(key=itemgetter(0, 4, 3, 1, 2))

        def dedup_zooms(lst, item):
            if lst:
                lst[-1] = lst[-1][:]
                if lst[-1][0] == item[0] and lst[-1][3] in item[3] and lst[-1][4] == item[4] and lst[-1][2] >= item[2] and lst[-1][1] <= item[1]:
                    return lst
                if lst[-1][0] == item[0] and lst[-1][3] == item[3] and lst[-1][4] == item[4] and lst[-1][2] == (item[1] - 1):
                    lst[-1][2] = item[2]
                    return lst
            lst.append(item)
            return lst
        tv['sel'] = reduce(dedup_zooms, tv['sel'], [])

        def format_string(i):
            i = i[:]
            dash = '-'
            zmark = '|z'
            if i[2] == LAST_ZOOM:
                i[2] = ''
            if i[1] == i[2]:
                i[2] = ''
                dash = ''
            if i[1] <= 1:
                i[1] = ''
            if i[1] == i[2] and i[1] == '':
                zmark = ''
                dash = ''
            return "%s%s%s%s%s%s%s" % (i[0], zmark, i[1], dash, i[2], i[3], i[4])

        tv['sel'] = [format_string(i) for i in tv['sel']]
        print (",\n").join([i for i in tv['sel']])
        print "{"
        kvrule = tv['kv']
        kvrule['z-index'] = allz.index(tv['z'])
        for k, v in kvrule.iteritems():
            v = str(v)
            if k == "z-index" and str(number(v)) == "0":
                continue
            # elif k == "z-index":
            #    v = str(2000 - int(v))
            if " " in v or ":" in v or not v:
                v = '"' + v + '"'
            print "  " + k + ":\t" + str(v) + ";"
        print "}"
    for i in names.symmetric_difference(class_order):
        print >> sys.stderr, i, "in classificator but not rendered"

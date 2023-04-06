#!/usr/bin/env python3

import sys
import drules_struct_pb2

if len(sys.argv) < 2:
    print('Usage: {} <drules_proto.bin>'.format(sys.argv[0]))
    sys.exit(1)

drules = drules_struct_pb2.ContainerProto()
drules.ParseFromString(open(sys.argv[1], mode='rb').read())

dr_priorities = {}
dr_unique = {}
min_vis_zooms = {}
overlays_min_vis_zooms = {}

for rule in drules.cont:
    for zoom in rule.element:
        if zoom.scale:

            def add_result(dr_type, dr_type_name):
                if repr(dr_type) != '':
                    dr_type_id = (rule.name, dr_type_name, apply_if, dr_type.priority)

                    if dr_type_id in dr_priorities:
                        dr_priorities[dr_type_id]['zooms'].add(zoom.scale)
                    else:
                        new_dr_type = {'type': dr_type_name, 'clname': rule.name, 'if': apply_if, 'priority': dr_type.priority, 'zooms': set([zoom.scale])}
                        dr_priorities[dr_type_id] = new_dr_type

                        dr_type_unique = dr_type_id[:3]
                        if dr_type_unique in dr_unique:
                            dr_unique[dr_type_unique] += 1
                        else:
                            dr_unique[dr_type_unique] = 1

                        cur_zoom = zoom.scale
                        if dr_type_name in ('icon', 'text', 'ptxt', 'shld'):
                            if rule.name in overlays_min_vis_zooms:
                                overlays_min_vis_zooms[rule.name] = min(overlays_min_vis_zooms[rule.name], cur_zoom)
                            else:
                                overlays_min_vis_zooms[rule.name] = cur_zoom

                        if rule.name in min_vis_zooms: #todo: apply_if specific???
                            min_vis_zooms[rule.name] = min(min_vis_zooms[rule.name], cur_zoom)
                        else:
                            min_vis_zooms[rule.name] = cur_zoom
                return

            apply_if = ''
            if zoom.apply_if:
                for cond in zoom.apply_if:
                    if apply_if != '':
                        apply_if += ', '
                    apply_if += str(cond)
                apply_if = 'if[{}]'.format(apply_if)

            nline = 1
            for line in zoom.lines:
                add_result(line, 'line{}'.format(nline))
                nline += 1
            add_result(zoom.area, 'area')
            add_result(zoom.symbol, 'icon')
            add_result(zoom.caption, 'text')
            add_result(zoom.path_text, 'ptxt')
            add_result(zoom.shield, 'shld')

def compound_priority_sort_key(dr_type_id):
    zoom_key = 0
    depth_layer = 0
    if dr_type_id[1] == 'area' or dr_type_id[1][:4] == 'line':
        depth_layer = 1
    else:
        if use_min_vis_zoom_sort:
            if dr_type_id[1] == 'shld' and use_shields_hardcode:
                # Shields are hardcoded to minVisZoom = 10 ATM.
                zoom_key = 10
            else:
                zoom_key = vis_zooms[dr_type_id[0]]
    # depth layer, zoom, reverse priority, clname, type, apply_if
    return (depth_layer, zoom_key, 100000 - dr_type_id[3], dr_type_id[0], dr_type_id[1], dr_type_id[2])

def prettify_zooms(zooms):
    zooms = sorted(zooms)
    first = zooms.pop(0)
    prev = first
    result = ''
    for zoom in zooms:
        if zoom == prev + 1:
            prev = zoom
        else:
            if first == prev:
                zrange = '{}'.format(first)
            else:
                zrange = '{}-{}'.format(first, prev)
            if result != '':
                result += ', '
            result += zrange
            first = zoom
            prev = zoom
    if first == prev:
        zrange = '{}'.format(first)
    else:
        zrange = '{}-{}'.format(first, prev)
    if result != '':
        result += ', '
    result += zrange
    return result

display_priorities = False
display_inconsistent_zooms = False
use_overlays_min_vis_zooms = False
use_min_vis_zoom_sort = True
use_shields_hardcode = True

vis_zooms = min_vis_zooms
if use_overlays_min_vis_zooms:
    vis_zooms = overlays_min_vis_zooms

for dr_type_id in sorted(dr_priorities.keys(), key = compound_priority_sort_key):
    if dr_type_id[1] == 'area' or dr_type_id[1][:4] == 'line':
        depth_layer = "GEOM"
        zoom_key = ''
    else:
        depth_layer = "OVRL"
        if dr_type_id[1] == 'shld' and use_shields_hardcode:
            # Shields are hardcoded to minVisZoom = 10 ATM.
            zoom_key = '/z{}'.format(10)
        else:
            zoom_key = '/z{}'.format(vis_zooms[dr_type_id[0]])
        if use_overlays_min_vis_zooms:
            original_min_zoom = min_vis_zooms[dr_type_id[0]]
            if dr_type_id[1] == 'shld':
                original_min_zoom = 10
            if original_min_zoom != vis_zooms[dr_type_id[0]]:
                zoom_key += ' !z{}'.format(original_min_zoom)
    clname = dr_type_id[0]
    if dr_type_id[2]:
        clname = '{}-{}'.format(clname, dr_type_id[2])
    output = '{}{}'.format(depth_layer, zoom_key)
    if display_priorities:
        output += '/{}'.format(dr_type_id[3])
    output += '\t {}\t {:<30}\t z[{}]'.format(dr_type_id[1], clname, prettify_zooms(dr_priorities[dr_type_id]['zooms']))
    print(output)


if display_inconsistent_zooms:
    print("\n=== INCONSISTENT ZOOMS ===\n")
    for dr_type_id in sorted(dr_priorities.keys()):
        dr_type_unique = dr_type_id[:3]
        if dr_unique[dr_type_unique] > 1:
            #print('{0} - {1} - z[{3}] : {2}'.format(*dr_type_id, prettify_zooms(dr_priorities[dr_type_id]['zooms'])))
            #apply_if = '\tif[{}]'.format(dr_type_id[2]) if dr_type_id[2] else ''
            print('{0[0]:<30}\t {0[1]}\t {0[2]}\t z[{1}]\t {0[3]}'.format(dr_type_id, prettify_zooms(dr_priorities[dr_type_id]['zooms'])))

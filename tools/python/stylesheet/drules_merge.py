#!/usr/bin/env python3
# Compares two drules files and produces a merged result.
# Also prints differences (missing things in drules1) to stdout.
import sys, re
import copy, collections
import drules_struct_pb2

def read_drules(drules):
  """Parses the structure and extracts elements for lowest and highest zooms
  for each rule."""
  result = {}

  for rule in drules.cont:
    zooms = [None, None]
    for elem in rule.element:
      if elem.scale >= 0:
        if zooms[1] is None or elem.scale > zooms[1].scale:
          zooms[1] = elem
        if zooms[0] is None or elem.scale < zooms[0].scale:
          zooms[0] = elem

    if zooms[0] is not None:
      name = str(rule.name)
      if name in result:
        if result[name][0].scale < zooms[0].scale:
          zooms[0] = result[name][0]
        if result[name][1].scale > zooms[1].scale:
          zooms[1] = result[name][1]
      result[name] = zooms
  return result

def zooms_string(z1, z2):
  """Prints 'zoom N' or 'zooms N-M'."""
  if z2 != z1:
    return "zooms {}-{}".format(min(z1, z2), max(z1, z2))
  else:
    return "zoom {}".format(z1)

def add_missing_zooms(dest, typ, source, target, high):
  """Checks zoom ranges for source and target, and appends as much sources to
  the dest as needed."""
  if high:
    scales = (target.scale + 1, source.scale + 1)
  else:
    scales = (source.scale, target.scale)

  if scales[1] < scales[0]:
    print("{}: missing {} {}".format(typ, 'high' if high else 'low', zooms_string(scales[1], scales[0] - 1)))
    for z in range(scales[1], scales[0]):
      fix = copy.deepcopy(source)
      fix.scale = z
      dest[typ].append(fix)
  elif scales[1] > scales[0]:
    print("{}: extra {} {}".format(typ, 'high' if high else 'low', zooms_string(scales[0], scales[1] - 1)))

def create_diff(zooms1, zooms2):
  """Calculates difference between zoom dicts, and returns a tuple:
  (add_zooms_low, add_zooms_high, add_types), for missing zoom levels
  and missing types altogether. Zooms are separate to preserve sorting
  order in elements."""
  add_elements_low = collections.defaultdict(list)
  add_elements_high = collections.defaultdict(list)
  seen = set(zooms2.keys())
  for typ in zooms1:
    if typ in zooms2:
      seen.remove(typ)
      add_missing_zooms(add_elements_low,  typ, zooms1[typ][0], zooms2[typ][0], False)
      add_missing_zooms(add_elements_high, typ, zooms1[typ][1], zooms2[typ][1], True)
    else:
      print("{}: not found in the alternative style; {}".format(typ, zooms_string(zooms1[typ][0].scale, zooms1[typ][1].scale)))

  add_types = []
  for typ in sorted(seen):
    print("{}: missing completely; {}".format(typ, zooms_string(zooms2[typ][0].scale, zooms2[typ][1].scale)))
    cont = drules_struct_pb2.ClassifElementProto()
    cont.name = typ
    for z in range(zooms2[typ][0].scale, zooms2[typ][1].scale + 1):
      fix = copy.deepcopy(zooms2[typ][0])
      fix.scale = z
      cont.element.extend([fix])
    add_types.append(cont)

  return (add_elements_low, add_elements_high, add_types)

def apply_diff(drules, diff):
  """Applies diff tuple (from create_diff) to a drules set."""
  d2pos = 0
  result = drules_struct_pb2.ContainerProto()
  for rule in drules.cont:
    typ = str(rule.name)
    # Append rules from diff[2] which nas name less than typ
    while d2pos < len(diff[2]) and diff[2][d2pos].name < typ:
      result.cont.extend([diff[2][d2pos]])
      d2pos += 1
    fix = drules_struct_pb2.ClassifElementProto()
    fix.name = typ
    if typ in diff[0]:
      fix.element.extend(diff[0][typ])
    if rule.element:
      fix.element.extend(rule.element)
    if typ in diff[1]:
      fix.element.extend(diff[1][typ])
    result.cont.extend([fix])
  result.cont.extend(diff[2][d2pos:])
  return result

if __name__ == '__main__':
  if len(sys.argv) <= 3:
    print(f'Usage: {sys.argv[0]} <drules1.bin> <drules2.bin> [drules3.bin ...] <drules_out.bin> [drules_out.txt]')
    sys.exit(1)

  drules_out_txt = None
  if sys.argv[-1].endswith('txt'):
    drules_out_txt = sys.argv[-1]
    sys.argv = sys.argv[:-1]
  drules_out_bin = sys.argv[-1]
  drules_list = sys.argv[1:-1]

  print(f'Merging {drules_list} into {drules_out_bin}({drules_out_txt if drules_out_txt else "no text output"})')

  drules_merged = drules_struct_pb2.ContainerProto()
  drules_merged.ParseFromString(open(drules_list[0], mode='rb').read())
  merged = drules_merged

  for drules_path in drules_list[1:]:
    drules = drules_struct_pb2.ContainerProto()
    drules.ParseFromString(open(drules_path, mode='rb').read())
    zooms1 = read_drules(merged)
    zooms2 = read_drules(drules)
    diff = create_diff(zooms1, zooms2)
    merged = apply_diff(merged, diff)

  with open(drules_out_bin, 'wb') as f:
    f.write(merged.SerializeToString())
  if drules_out_txt:
    with open(drules_out_txt, 'wb') as f:
      f.write(str(merged).encode('utf-8'))

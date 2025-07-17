from __future__ import print_function
from pytraffic import (RoadSegmentId,
                       SegmentSpeeds,
                       load_classificator,
                       generate_traffic_keys,
                       generate_traffic_values_from_binary,
                       generate_traffic_values_from_list)
import argparse

parser = argparse.ArgumentParser(description='Example usage of pytraffic.')
parser.add_argument("--path_to_classificator", dest="path_to_classificator",
                    help="Path to the directory that contains classificator.txt.")
parser.add_argument("--path_to_mwm", dest="path_to_mwm", help="Path to the target mwm file.")
parser.add_argument("--path_to_keys", dest="path_to_keys",
                    help="Path to serialized key data (i.e. the \"traffic\" section).")

options = parser.parse_args()
if not options.path_to_classificator or not options.path_to_mwm or not options.path_to_keys:
  parser.print_help()
  exit()

load_classificator(options.path_to_classificator)

keys = [
  RoadSegmentId(0, 0, 0),
  RoadSegmentId(1, 0, 0),
  RoadSegmentId(1, 0, 1),
]

fid, idx, dir = keys[2].fid, keys[2].idx, keys[2].dir
print(fid, idx, dir)

keys_from_mwm = generate_traffic_keys(options.path_to_mwm)

seg_speeds = SegmentSpeeds(1.0, 2.0, 3.0)
ws, wrs, w = seg_speeds.weighted_speed, seg_speeds.weighted_ref_speed, seg_speeds.weight
print(ws, wrs, w)

mapping = {
  RoadSegmentId(0, 0, 0): SegmentSpeeds(1.0, 2.0, 3.0),
  RoadSegmentId(1, 0, 1): SegmentSpeeds(4.0, 5.0, 6.0),
}

buf1 = generate_traffic_values_from_list(keys, mapping)

with open(options.path_to_keys, "rb") as bin_data:
  buf2 = generate_traffic_values_from_binary(bin_data.read(), {})

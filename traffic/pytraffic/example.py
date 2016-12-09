from pytraffic import RoadSegmentId, SegmentSpeeds, load_classificator, generate_traffic_keys, generate_traffic_values
import argparse

parser = argparse.ArgumentParser(description='Example usage of pytraffic.')
parser.add_argument("--path_to_classificator", dest="path_to_classificator", help="Path to the directory that contains classificator.txt.")
parser.add_argument("--path_to_mwm", dest="path_to_mwm", help="Path to the target mwm file.")

options = parser.parse_args()
if not options.path_to_classificator or not options.path_to_mwm:
  parser.print_help()
  exit()

load_classificator(options.path_to_classificator)

keys = [
  RoadSegmentId(0, 0, 0),
  RoadSegmentId(1, 0, 0),
  RoadSegmentId(1, 0, 1),
]

keys_from_mwm = generate_traffic_keys(options.path_to_mwm)

mapping = {
  RoadSegmentId(0, 0, 0):SegmentSpeeds(1.0, 2.0, 3.0),
  RoadSegmentId(1, 0, 1):SegmentSpeeds(4.0, 5.0, 6.0),
}

buf = generate_traffic_values(keys, mapping)

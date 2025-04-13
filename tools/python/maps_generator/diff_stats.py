import argparse

from maps_generator.generator.statistics import diff
from maps_generator.generator.statistics import read_types


def get_args():
    parser = argparse.ArgumentParser(
        description="This script prints the difference between old_stats.json and new_stats.json."
    )
    parser.add_argument(
        "--old",
        default="",
        type=str,
        required=True,
        help="Path to old file with map generation statistics.",
    )
    parser.add_argument(
        "--new",
        default="",
        type=str,
        required=True,
        help="Path to new file with map generation statistics.",
    )
    return parser.parse_args()


def main():
    args = get_args()
    old = read_types(args.old)
    new = read_types(args.new)
    for line in diff(new, old):
        print(";".join(str(x) for x in line))


if __name__ == "__main__":
    main()

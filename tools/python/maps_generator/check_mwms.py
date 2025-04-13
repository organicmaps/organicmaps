import argparse
import sys

from maps_generator.checks.default_check_set import CheckType
from maps_generator.checks.default_check_set import MwmsChecks
from maps_generator.checks.default_check_set import get_mwm_check_sets_and_filters
from maps_generator.checks.default_check_set import run_checks_and_print_results


def get_args():
    parser = argparse.ArgumentParser(
        description="This script checks mwms and prints results."
    )
    parser.add_argument(
        "--old", type=str, required=True, help="Path to old mwm directory.",
    )
    parser.add_argument(
        "--new", type=str, required=True, help="Path to new mwm directory.",
    )
    parser.add_argument(
        "--categories", type=str, required=True, help="Path to categories file.",
    )
    parser.add_argument(
        "--checks",
        action="store",
        type=str,
        nargs="*",
        default=None,
        help=f"Set of checks: {', '.join(c.name for c in MwmsChecks)}. "
        f"By default, all checks will run.",
    )
    parser.add_argument(
        "--level",
        type=str,
        required=False,
        choices=("low", "medium", "hard", "strict"),
        default="medium",
        help="Messages level.",
    )
    parser.add_argument(
        "--output",
        type=str,
        required=False,
        default="",
        help="Path to output file. stdout by default.",
    )
    return parser.parse_args()


def main():
    args = get_args()

    checks = {MwmsChecks[c] for c in args.checks} if args.checks else None
    s = get_mwm_check_sets_and_filters(
        args.old, args.new, checks, categories_path=args.categories
    )
    run_checks_and_print_results(
        s,
        CheckType[args.level],
        file=open(args.output, "w") if args.output else sys.stdout,
    )


if __name__ == "__main__":
    main()

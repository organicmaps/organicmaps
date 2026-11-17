#!/usr/bin/env python3
"""
Script to sort categories.txt alphabetically within each section.

Rules:
- Keep the first non-comment line of each block as its type or group header
- Keep en: as the first translation
- Sort all other language lines alphabetically
- Keep preceding comments with their translations
- Keep header comments and trailing comments in place
- Preserve empty lines between sections
"""


def parse_and_sort_categories(input_file: str, output_file: str) -> None:
    """Parse categories.txt and sort each section alphabetically."""

    with open(input_file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    result = []
    i = 0

    while i < len(lines):
        line = lines[i].rstrip("\n")
        result.append(line)
        i += 1

        # Handle comments and empty lines
        if line.lstrip().startswith("#") or not line.strip():
            continue

        # Block position identifies headers: OSM type names can themselves contain colons.
        translations = []
        comments = []
        while i < len(lines) and lines[i].strip():
            line = lines[i].rstrip("\n")
            i += 1
            if line.lstrip().startswith("#"):
                comments.append(line)
            else:
                translations.append((line, comments))
                comments = []

        translations.sort(
            key=lambda entry: (not entry[0].startswith("en:"), entry[0].split(":", 1)[0]),
        )
        for translation, leading_comments in translations:
            result.extend(leading_comments)
            result.append(translation)
        result.extend(comments)

    # Write the result
    with open(output_file, "w", encoding="utf-8") as f:
        for line in result:
            f.write(line + "\n")

    print(f"✓ Sorted categories written to {output_file}")


def main():
    """Main entry point."""
    import sys
    import os
    import argparse

    parser = argparse.ArgumentParser(
        description="Sort categories.txt alphabetically within each section.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Sort and create a new file (default behavior)
  python3 sort_categories.py

  # Sort in-place (overwrites the original file)
  python3 sort_categories.py --in-place

  # Specify custom input and output files
  python3 sort_categories.py -i input.txt -o output.txt
        """,
    )

    parser.add_argument(
        "-i",
        "--input",
        default="data/categories.txt",
        help="Input file path (default: data/categories.txt)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="data/categories_sorted.txt",
        help="Output file path (default: data/categories_sorted.txt)",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Sort the file in-place (overwrites the input file) without creating a backup",
    )
    parser.add_argument(
        "--in-place-backup",
        action="store_true",
        help="Sort the file in-place (overwrites the input file) and creates a backup",
    )

    args = parser.parse_args()

    input_file = args.input
    output_file = (
        args.output if (not args.in_place and not args.in_place_backup) else input_file
    )

    # Check if input file exists
    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found")
        print("Usage: Run this script from the root of the Organic Maps repository")
        return 1

    # Create backup if sorting in-place with a backup
    if args.in_place_backup:
        backup_file = input_file + ".bak"
        import shutil

        shutil.copy2(input_file, backup_file)
        print(f"✓ Created backup: {backup_file}")

    parse_and_sort_categories(input_file, output_file)

    if not args.in_place:
        print("\nTo replace the original file, run:")
        print(f"  mv {output_file} {input_file}")
        print("\nOr run the script with --in-place flag:")
        print(f"  python3.12 sort_categories.py --in-place")

    return 0


if __name__ == "__main__":
    import sys

    sys.exit(main())

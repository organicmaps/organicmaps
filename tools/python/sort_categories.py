#!/usr/bin/env python3
"""
Script to sort categories.txt alphabetically within each section.

Rules:
- Keep comments (lines starting with #)
- Keep the first line of each section (starts with @ or doesn't have a colon)
- Keep the en: line immediately after the first line
- Sort all other language lines alphabetically
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

        # Handle comments and empty lines
        if line.startswith("#") or line == "":
            result.append(line)
            i += 1
            continue

        # Check if this is the start of a section
        # Section starts with @ or doesn't contain a colon
        if line.startswith("@") or ":" not in line:
            # This is the first line of a section
            section_first_line = line
            i += 1

            # Collect all lines in this section until we hit an empty line or EOF
            section_lines = []
            en_line = None

            while i < len(lines):
                current_line = lines[i].rstrip("\n")

                # Empty line marks the end of the section
                if current_line == "":
                    break

                # Comments within a section (shouldn't happen, but let's handle it)
                if current_line.startswith("#"):
                    section_lines.append(current_line)
                    i += 1
                    continue

                # Check if this is the en: line
                if current_line.startswith("en:"):
                    en_line = current_line
                    i += 1
                    continue

                # Regular language line
                section_lines.append(current_line)
                i += 1

            # Sort the section lines alphabetically by language code
            section_lines.sort(key=lambda x: x.split(":")[0])

            # Build the section output
            result.append(section_first_line)

            # Add en: line right after the first line (if it exists)
            if en_line:
                result.append(en_line)

            # Add sorted lines
            result.extend(section_lines)

            # Don't increment i here since the while loop already did
            continue

        # If we get here, it's an unexpected line format
        # Just add it as-is to be safe
        result.append(line)
        i += 1

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
        print("Usage: Run this script from the root of the omim repository")
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

# Validate committed border geometry

Run this from the repository root after setting up `border_work` and
`border_python` as described in [README.md](README.md). Choose the commit that
precedes the data changes under review. Extracting it leaves the working tree
unchanged:

```sh
border_baseline_revision=96b1cbc2e0d1d39459689cd91bf4e3fd5cde0d1e
mkdir "$border_work/geometry-baseline"
git archive "$border_baseline_revision" data/borders data/packed_polygons.bin \
  libs/storage/country_decl.hpp |
  tar -x -C "$border_work/geometry-baseline"

border_bounds_format() {
  if grep -q 'Int64ToRectObsolete' "$1"; then
    echo legacy
  elif grep -q 'SaveBoundRect' "$1"; then
    echo wrapped
  else
    echo 'Unsupported CountryDef serialization in comparison revision.' >&2
    return 1
  fi
}
border_current_format=$(border_bounds_format libs/storage/country_decl.hpp) &&
border_baseline_format=$(border_bounds_format \
  "$border_work/geometry-baseline/libs/storage/country_decl.hpp") &&
"$border_python" tools/python/borders/check_border_geometry.py \
  --source data/borders --packed data/packed_polygons.bin \
  --baseline-source "$border_work/geometry-baseline/data/borders" \
  --baseline-packed "$border_work/geometry-baseline/data/packed_polygons.bin" \
  --bounds-format "$border_current_format" \
  --baseline-bounds-format "$border_baseline_format" \
  --minimum-area 0.01 --report "$border_work/geometry-comparison.json"
```

Use a fresh output directory for another baseline. Replace the example revision
with the relevant full commit SHA; comparing a file with itself cannot detect
regressions introduced by that file. For an uncommitted candidate binary, pass
its path to `--packed` while retaining the matching source directory.

The command reads every source polygon, serialized outer path and country
rectangle. It uses stored 30-bit Mercator coordinates directly. Source polygons
and outer paths are each compared with their own baseline. Membership coverage
uses the outer paths. Display data is not decoded or checked.

Checks include new invalid rings and contacts, uncovered baseline area, new
overlap between established region pairs or increased coverage multiplicity,
and area newly excluded by country rectangles. Source and outer-path coverage
changes and malformed serialization fail the check. Bounds serialization has
no version marker: `legacy` selects signed values with longitude in
`[-180, 180]`; `wrapped` selects unsigned values in `[-360, 360]`. The example
selects each format from its matching revision's `CountryDef` header. Both
`--bounds-format` options default to `wrapped`; explicitly select the format
when checking a binary from another revision. The reader's three longitude
shifts preserve regions crossing the antimeridian. Packed region names must
match the source filenames. Valid region splits and renames can pass when global
coverage and overlap multiplicity are preserved.

The JSON report records input hashes, decoded counts, inherited invalid
geometry and regressions. Exit status is zero for a comparison that introduces
no detected defects, or one for a regression. Invalid input or an unsupported
format also fails. Areas use the audit's approximate latitude correction; the
example threshold is 0.01 square metres. Inherited defects are reported and are
not a certificate that the complete dataset is valid. Overlap entries describe
region pairs and can cover the same area; their areas are not a unique-area sum.

Contacts within a source ring remain spatially strict. In an already invalid
packed ring, proper crossings and point touches must each match a distinct
baseline contact of the same kind within 0.003570556640625 Mercator degrees
(`2 * 360 * 1.3 / (256 << 10)`, twice the outer-path scale-10 simplification
epsilon). This is a conservative inspection policy for inherited contacts,
not a theoretical bound on intersection movement. New contact kinds or counts,
distant replacements, and any spatial growth of retraced lines fail. A repair can
leave point contacts on a previously retraced line; those contacts are inherited.

Between different rings, internal shared line segments are excluded from
contact-location checks only when both outer rings are individually valid and
their intersection has exactly zero area. Isolated point contacts, contacts
involving holes or invalid rings, and interior overlaps remain spatially strict.
Gap and region-overlap checks still apply to all rings. Area comparisons subtract
coverage without rounding coordinates or applying a distance tolerance, including
mixed point/line contacts and near-collinear source boundaries.

The `Border tools` workflow runs the synthetic regression tests and this full
comparison in both dependency environments. It extracts the pull request's
base commit or the push event's previous commit. A manual run requires an
explicit baseline commit SHA. Changes to the source polygons, committed binary,
tools or workflow trigger the checks. The comparison report is uploaded even
when the geometry check fails. Open the `Compare source polygons and packed
paths` step for phase progress and failure categories with affected filenames;
the `border-geometry-current` and `border-geometry-legacy` artifacts contain all
findings in `border-geometry.json`.

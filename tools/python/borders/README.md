# Manually maintain source borders

The `.poly` files in `data/borders` define offline map coverage. They include
administrative borders, artificial download divisions and offshore coverage.
Keep filenames, coverage and shared junctions when updating them. A changed
boundary must use the same line in every affected neighbour.

These tools work offline. They do not fetch current OpenStreetMap boundaries.
They read every `.poly` in an input directory and write candidates separately.
Use the complete border set for transformations so neighbouring owners are
available. Output directories must be empty or absent, and must not contain or
be contained by their input directory.

## Set up a working directory

The tools do not split or rename files. Finish any manual edits, splits or
renames before taking the baseline below.
Keep the complete updated border set in `data/borders`, including every neighbour
of a changed region. Review the manual changes separately: the later baseline
comparison checks the tools' changes and requires matching filenames, titles
and ring names in the baseline and candidate.

Run the commands from the repository root in the same shell. Python 3.10 or
later is required. `requirements.txt` installs NumPy 1.23+ and Shapely 2.0+.
Alignment also requires GEOS 3.10+; standard Shapely wheels include GEOS.

```sh
border_work=$(mktemp -d /tmp/om-borders.XXXXXX)
python3 -m venv "$border_work/venv"
border_python="$border_work/venv/bin/python"
"$border_python" -m pip install -r tools/python/borders/requirements.txt

mkdir "$border_work/00-baseline"
cp data/borders/*.poly "$border_work/00-baseline/"
```

Keep this baseline unchanged through every pass. The other child directories
below are distinct outputs; the tools create them. A new run should use a new
working directory. The standard flow needs no existing JSON input; the tools
write their JSON reports automatically. Edge exclusions are optional and are
described below.

## Tools and arguments

Positional arguments come first in these examples. All tools accept `--help`.

| Tool | Arguments and purpose | Output report |
| --- | --- | --- |
| `repair_backtracks.py` | `INPUT OUTPUT`: cancel exact zero-area retraces. | `backtrack-repair-report.json` |
| `align_border_points.py` | `INPUT OUTPUT`: insert nearby existing source points into unmatched edges. | `junction-report.json` |
| `reconcile_borders.py` | `INPUT OUTPUT`: reuse one existing path for nearly identical shared arcs. | `normalization-report.json` |
| `maintain_borders.py` | `INPUT OUTPUT`: Douglas–Peucker simplification of canonical shared arcs. | `report.json` |
| `audit_borders.py` | `DIRECTORY --report JSON`: inspect geometry; add `--baseline DIRECTORY` to compare. | The requested JSON path |

Alignment, reconciliation and simplification accept `--epsilon EPSILON` and
repeatable `--preserve-file NAME`. Names are input filenames, with or without
`.poly`; unknown names fail. Preservation excludes alignment donors and targets,
locks reconciliation groups involving the file, and protects its simplification
arcs in all owners. Backtrack cleanup has no epsilon or preservation flag.

Alignment additionally accepts repeatable `--allow-invalid-file NAME` and
`--exclude-edges JSON`. The invalid-file exception is limited to single-ring
files: replacements must introduce no new local intersections and retain the
existing validity reason. Use it only after inspecting the affected geometry,
then compare the complete candidate against the original baseline.

Audit accepts repeatable `--group PREFIX` and `--minimum-area SQUARE_METERS`.
`--report` is required. Groups affect enclosed-hole union checks only; validity,
metadata, neighbour overlaps and baseline coverage losses always cover the
whole directory.

## Run the required passes once

Choose audit groups to list existing enclosed holes in the changed areas. The
commands below illustrate France and Germany; extend the groups for other
affected regions. A prefix such as `France_` selects its download regions;
`Andorra` selects a single-file country. These groups do not limit which files
the transformations process. After splitting or renaming regions, adjust the
`--group` prefixes and `--preserve-file` names to match the new input. You can
omit `--group` to skip the enclosed-hole listing; baseline coverage and overlap
checks still inspect every file. The commands are chained so a failed pass or
audit stops the flow. Inspect its error and report before rerunning with fresh
output directories.

```sh
"$border_python" tools/python/borders/audit_borders.py "$border_work/00-baseline" \
  --group France_ --group Germany_ --minimum-area 0.01 \
  --report "$border_work/before.json" &&

"$border_python" tools/python/borders/repair_backtracks.py \
  "$border_work/00-baseline" "$border_work/01-backtracks" &&

"$border_python" tools/python/borders/align_border_points.py \
  "$border_work/01-backtracks" "$border_work/02-aligned" --epsilon 1e-5 &&

"$border_python" tools/python/borders/reconcile_borders.py \
  "$border_work/02-aligned" "$border_work/03-matched" --epsilon 1e-5 &&

"$border_python" tools/python/borders/maintain_borders.py \
  "$border_work/03-matched" "$border_work/04-candidate" --epsilon 1e-5 \
  --preserve-file Bermuda --preserve-file 'Cape Verde' \
  --preserve-file 'Faroe Islands' --preserve-file Malta &&

"$border_python" tools/python/borders/audit_borders.py "$border_work/04-candidate" \
  --baseline "$border_work/00-baseline" \
  --group France_ --group Germany_ --minimum-area 0.01 \
  --report "$border_work/after.json" &&

"$border_python" -m unittest discover -s tools/python/borders -p 'test_*.py'
```

Skip unnecessary passes by giving the next tool the preceding output or the
baseline directly. Backtrack cleanup comes first because it can make an invalid
ring eligible for later passes without moving its boundary. Do not repeatedly
simplify generated outputs: each run can add another epsilon of displacement.
Review each stage's report, especially rollbacks and unresolved matches.

The four offshore outlines are preserved because DP at this tolerance removed
formerly covered area from them. The October 2026 review restored their input
coordinates. The source references and additional alignment controls for that
run are recorded below.

Backtrack cleanup cancels `A-B-A`, or horizontal/vertical partial reversals with
`B` outside `A-C`. These lines remain collinear in Mercator. Every changed file
must retain exactly the same projected filled geometry, preserving its areal
intersection with every neighbour. The writer retains original coordinate text,
metadata and blank lines; a new closing record copies existing source text.
Sloped partial longitude/latitude retraces and other crossings remain for source review.

Alignment never averages coordinates. It inserts exact points already shared
by regions, or points whose donor ring also contains both target edge endpoints.
Ambiguous matches and points near endpoints are excluded. Reconciliation matches
paths between exact common endpoints and retains the path with more distinct
vertices, within a continuous distance bound. Density does not prove accuracy
against current OpenStreetMap. Invalid alignment targets and reconciliation
groups touching invalid files are excluded by default; invalid files can still
donate alignment points unless preserved. Invalidating replacements are rolled
back.

## Optional reviewed edge exclusions

Create your own JSON file when an alignment insertion causes a regression.
Use an actual input filename and the two exact original longitude/latitude
endpoints of the edge, for example by copying them from `junction-report.json`.
The format is a JSON array; replace the illustrative values below:

```json
[
  {
    "file": "Region.poly",
    "endpoints": [[12.345, 45.678], [12.346, 45.679]],
    "reason": "Reviewed insertion would change neighbouring coverage."
  }
]
```

Save it as `$border_work/excluded-edges.json`, then add
`--exclude-edges "$border_work/excluded-edges.json"` to alignment. Use a fresh
output directory when rerunning that pass and all subsequent passes. Excluded
edges still count when rejecting ambiguous matches. Absent historical edges
are reported as unmatched exclusions; unknown filenames fail. Choose exclusions
from your own input and audit for each maintenance run.

## Precision and validation

The default epsilon is **1e-5 in Organic Maps' degrees-scaled spherical
Mercator coordinates**, about 1.11 m at the equator multiplied by
`cos(latitude)` on the ground. It matches `kMwmPointAccuracy`; the historical
source simplification tolerance is undocumented. Alignment, reconciliation and
DP each have this bound, so their combined displacement can be up to `3e-5`.
Source-backed administrative updates are separate changes and need their own
coverage and junction proofs.

C++ `GeneratePackedBorders()` uses display tolerances
`360 * 1.3 / (256 * 2^zoom)`: 0.0017852783203125 at zoom 10 and
0.00714111328125 at zoom 8. Those are not source maintenance tolerances. The
packed-border simplifier uses near-optimal dynamic programming; the Python
simplifier uses Douglas–Peucker.

The simplifier interns exact vertices and undirected edges across all files.
Branches and changes in edge ownership fix junctions. Each common arc is
simplified once and reused in every owner; closed shared rings also normalize
rotation and direction. Invalid source rings protect their vertices in every
neighbour. Unmatched edges connected to shared topology are preserved. If a
valid ring or file would become invalid, its arcs are locked in every owner.
This preserves already matching shared lines; it does not repair all mismatched
input borders.

Geometry checks use projected vertices and straight Mercator segments.
Straight longitude/latitude segments can differ on long edges.
Reports convert locations back to longitude/latitude; square-metre areas use an
approximate squared-cosine latitude correction. The audit measures invalid rings
using the original edges' even/odd fill: a twice-traversed loop has no filled
interior. These measurements never change the source coordinates or mark an
invalid ring as valid.

With `--baseline`, audit exits nonzero for newly invalid rings/files, changed
metadata, new overlap or formerly covered parts above the area threshold,
or any unclosed candidate ring. The default area threshold is 1 m²; the example
uses 0.01 m². Existing invalid rings, overlaps and holes are reported even when
comparison succeeds. Some overlaps and enclaves are intentional download
coverage, so inspect the source before changing them.

`introduced_uncovered_parts` lists each baseline file's lost coverage after
subtracting every candidate region that occupies it. This check includes gaps
open to the exterior and runs without `--group`. Area reassigned to a neighbour
is still covered and does not count as a loss. Inspect the reported bounds and
representative point for every loss; simplification of an offshore outline can
also remove formerly covered area.

Group checks list enclosed holes and their newly uncovered parts. `--group '*'`
selects all files for this additional diagnostic, but its union can be expensive.
The tools do not unwrap the antimeridian. A successful baseline comparison proves
no coverage regressions above the threshold in this coordinate plane; existing
gaps and administrative accuracy still require source review. Wider discrepancies
and current boundary updates require verified OpenStreetMap or other authoritative
source geometry, applied to every neighbour while retaining curated download divisions.

The [border polygon issue #3815](https://github.com/organicmaps/organicmaps/issues/3815)
tracks related work. These maintenance passes do not resolve every remaining
border issue.

The `Border tools` GitHub Actions workflow runs the documented unittest command
when these tools or `data/borders` change. It tests Python 3.12 with NumPy 1.26.4 /
Shapely 2.0.7 and current NumPy 2.x / Shapely 2.x releases.

## October 2026 source updates and review

The maintenance refreshed selected shared administrative arcs in Germany,
Canada, the US and Australia. Every owner reused the same canonical arc;
surrounding download cuts and junctions were preserved. The tools above do not
fetch or reproduce these manual source replacements.

| Updated boundary | OpenStreetMap references | Source retrieval |
| --- | --- | --- |
| Oldenburg / Detmold, Germany | [Niedersachsen 62771](https://www.openstreetmap.org/relation/62771), [Nordrhein-Westfalen 62761](https://www.openstreetmap.org/relation/62761); [map query](https://api.openstreetmap.org/api/0.6/map?bbox=8.15,52.07,8.19,52.08) | 2026-10-06 |
| Northwest Territories / Nunavut, Canada | [NWT 391220](https://www.openstreetmap.org/relation/391220), [Nunavut 390840](https://www.openstreetmap.org/relation/390840) | 2026-10-06; relation versions 136 and 280 |
| Manitoba / Saskatchewan, Canada | [Manitoba 390841](https://www.openstreetmap.org/relation/390841), [Saskatchewan 391178](https://www.openstreetmap.org/relation/391178) | Original date not retained; relation identities verified on 2026-10-09 |
| Alabama / Mississippi, US | [Alabama 161950](https://www.openstreetmap.org/relation/161950), [Mississippi 161943](https://www.openstreetmap.org/relation/161943); [common maritime way 166852354](https://www.openstreetmap.org/way/166852354) | Original date not retained; both relations' outer-way membership verified on 2026-10-09 |
| New South Wales / Victoria, Australia | [NSW 2316593](https://www.openstreetmap.org/relation/2316593), [Victoria 2316741](https://www.openstreetmap.org/relation/2316741) | 2026-10-06; relation identities verified on 2026-10-09 |

Coordinates in these notes are longitude/latitude. The German arc keeps ties
`(8.134489, 52.07274)` and `(8.201414, 52.08753)`, with their short connectors
to the fetched chain bounded by `1e-5` Mercator degrees. The Australian arc
keeps ties `(142.0315, -34.12785)` and `(142.0228, -34.1249)` and follows five
connected common outer ways tagged with DCS NSW / VicMap sources. The Canadian
NWT / Nunavut common ways were split at fixed download-cut junctions before DP.
The US offshore arc retains its artificial tie and approximately 134.64 m
connector to the common source way.

Raw OpenStreetMap snapshots, original version/hash manifests and one-off
replacement scripts were not retained. These references and controls therefore
cannot replay the manual updates byte for byte. New API replies are fresh
source data, not the original snapshots. Fetch and review source geometry in
every affected owner when rebuilding an update; geometry audits alone do not
establish administrative accuracy.

The reviewed alignment preserved `Canada_Nunavut_North.poly` and
`Greenland.poly`, allowed the single-ring invalid Vendée target, and excluded
14 reviewed edges whose insertions changed coverage or overlaps. These
maintenance-specific exclusions are not distributed with the tools. For a
separately prepared input containing source updates, create an exclusion file
from its alignment report and audit as described above, then use these controls
with the setup variables:

```sh
"$border_python" tools/python/borders/align_border_points.py \
  "$border_work/source-updated" "$border_work/review-aligned" --epsilon 1e-5 \
  --preserve-file Canada_Nunavut_North --preserve-file Greenland \
  --allow-invalid-file 'France_Pays de la Loire_Vendee' \
  --exclude-edges "$border_work/excluded-edges.json"
```

The French offshore seam needed a separate exact-point insertion.
`France_Pays de la Loire_Loire-Atlantique_Saint-Nazaire.poly` used a direct edge
from `(-2.395322, 47.10881)` to `(-2.813762, 46.91302)`, while Vendée used the
intermediate point **`(-2.779441, 46.92908)`**. Inserting that same point into
Saint-Nazaire makes both owners use the same two Mercator segments, with no
lost coverage, new overlap or new intersections.

The review restored four isolated valid offshore outlines after DP removed
approximately 47,091 m² of coverage: Cape Verde 33,883 m², Faroe Islands
10,540 m², Bermuda 1,583 m² and Malta 1,084 m². They have no shared edges or
junctions; restoration adds 59 coordinate records. Preserve them during DP as
shown above. The 1,159-file / 4,467-pair baseline audit with even/odd measurements
reports no new coverage loss, overlap or invalid geometry at 0.01 m². Existing
invalid rings decreased from 248 to 96; remaining invalid geometry and border
mismatches need source review. All 72 tests pass on both dependency combinations
used by CI.

To compare the current data with the unchanged source tree before this
maintenance, extract its baseline commit into a separate directory:

```sh
mkdir "$border_work/commit-baseline"
git archive 96b1cbc2e0 data/borders |
  tar -x -C "$border_work/commit-baseline"

"$border_python" tools/python/borders/audit_borders.py data/borders \
  --baseline "$border_work/commit-baseline/data/borders" --minimum-area 0.01 \
  --report "$border_work/review-audit.json"
```

## Apply a reviewed candidate

Apply only after the audit succeeds, the tests pass, and manual review covers
the changed boundaries and relevant junctions. Inspect the JSON reports and the
candidate `.poly` changes. If source files changed since the snapshot, start
again from the current source and revalidate.

The equality check below prevents copying over a changed source directory.
Only `.poly` files are copied; stage reports stay in the working directory.

```sh
diff -qr "$border_work/00-baseline" data/borders &&
  cp "$border_work/04-candidate/"*.poly data/borders/

git diff --stat -- data/borders
git diff -- data/borders
```

If you skipped passes or chose different output names, use your final validated
candidate directory in the copy command.

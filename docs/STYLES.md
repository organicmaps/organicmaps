# Map styling and icons

Here is the basic workflow to update styles:
1. Edit the styles file you want, e.g. [`Roads.mapcss`](../data/styles/default/include/Roads.mapcss)
2. Run the `tools/unix/generate_drules.sh` script
3. Test how your changes look in the app
4. Commit your edits and files changed by the script
5. Send a pull request!

Please prepend `[styles]` to your commit message and add [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to it.
Files changed by the script should be added as a separate `[styles] Regenerated` commit.

Please check [a list of current styling issues](https://github.com/organicmaps/organicmaps/issues?q=is%3Aopen+is%3Aissue+label%3AStyles)
and ["icons wanted" issues](https://github.com/organicmaps/organicmaps/issues?q=is%3Aopen+is%3Aissue+label%3AIcons+label%3A%22Good+first+issue%22).

An overview of currently used icons can be found in the [Wiki](https://github.com/organicmaps/organicmaps/wiki/Icons).

## Requirements

To work with styles first [clone the OM repository](INSTALL.md#getting-sources).

To run the `generate_symbols.sh` script install `optipng` also, e.g. for Ubuntu
```
sudo apt install optipng
```

If you use WSL on Windows 10 you might need to run [X Server](INSTALL.md#windows-10-wsl) before running `generate_symbols.sh`

## Files

Map styles are defined in text files located in `data/styles/default/include/`:
* Forests, rivers, buildings, etc. [`Basemap.mapcss`](../data/styles/default/include/Basemap.mapcss)
* Their text labels [`Basemap_label.mapcss`](../data/styles/default/include/Basemap_label.mapcss)
* Roads, bridges, foot and bicycle paths, etc. [`Roads.mapcss`](../data/styles/default/include/Roads.mapcss)
* Their text labels [`Roads_label.mapcss`](../data/styles/default/include/Roads_label.mapcss)
* Icons for POIs and other features [`Icons.mapcss`](../data/styles/default/include/Icons.mapcss)
* City-specific subway networks [`Subways.mapcss`](../data/styles/default/include/Subways.mapcss)
* Light (default) theme colors: [`light/colors.mapcss`](../data/styles/default/light/colors.mapcss)
* Dark/night theme colors: [`dark/colors.mapcss`](../data/styles/default/dark/colors.mapcss)
* Priorities of overlays (icons, captions..) [`priorities_4_overlays.prio.txt`](../data/styles/default/include/priorities_4_overlays.prio.txt)
* Priorities of lines and areas [`priorities_3_FG.prio.txt`](../data/styles/default/include/priorities_3_FG.prio.txt), [`priorities_2_BG-top.prio.txt`](../data/styles/default/include/priorities_2_BG-top.prio.txt), [`priorities_1_BG-by-size.prio.txt`](../data/styles/default/include/priorities_1_BG-by-size.prio.txt)

There is a separate set of these style files for the navigation mode in `data/styles/vehicle/`.

Icons are stored in [`data/styles/default/light/symbols/`](../data/styles/default/light/symbols/) and their dark/night counterparts are in [`data/styles/default/dark/symbols/`](../data/styles/default/dark/symbols/).

## How to add a new icon

1. Add an svg icon to `data/styles/default/light/symbols/` (and to `dark` too)
preferably look for icons in [collections OM uses already](../data/copyright.html#icons)
2. Add icon rendering/visibility rules into `data/styles/default/include/Icons.mapcss` and to "navigation style" `data/styles/vehicle/include/Icons.mapcss`
3. Run `tools/unix/generate_symbols.sh` to add new icons into skin files
4. Run `tools/unix/generate_drules.sh` to generate drawing rules for the new icons
5. [Test](#testing-your-changes) your changes

## How to add a new map feature / POI type

1. Add it into `data/mapcss-mapping.csv` (or better replace existing `deprecated` line) to make OM import it from OSM
2. If necessary merge similar tags in via `data/replaced_tags.txt`
3. Define a priority for the new feature type in e.g. [`priorities_4_overlays.prio.txt`](../data/styles/default/include/priorities_4_overlays.prio.txt) and/or other priorities files
4. Add a new icon (see [above](#how-to-add-a-new-icon)) and/or other styling (area, line..)
5. If a new POI should be OSM-addable/editable then add it to `data/editor.config`
6. Add new type translation into `data/strings/types_strings.txt`
7. Add search keywords into `data/categories.txt`
8. Run `tools/unix/generate_localizations.sh` to validate and distribute translations into iOS and Android
9. Add new or fix current classifier tests at `/generator/generator_tests/osm_type_tests.cpp` if you can
10. [Test](#testing-your-changes) your changes
11. Relax and wait for the next maps update :)

## Testing your changes

The most convenient way is using [the desktop app](INSTALL.md#desktop-app).

The desktop app also has a **Designer mode** that rebuilds the
currently edited style on demand, without restarting.

A Designer package for Linux and macOS is attached to every
[CMake workflow run](https://github.com/organicmaps/organicmaps/actions/workflows/build-cmake.yaml)
(open a run of `master` and scroll down to Artifacts).  Unpack it, run
`./designer.sh` and edit the MapCSS in its `data/styles/`, which is a copy of
this repository's.  It needs `python3`; Linux also needs compatible system
Qt 6 and C++ runtime libraries.  The macOS app and its
`generator_tool` and `style_tests` helpers share bundled Qt frameworks.
See the package's own `README.md`.  Run
`tools/unix/package_designer.sh <build-dir> [<output-parent-dir>]` to build a
package yourself. It replaces only the `OrganicMaps-Designer/` subdirectory
of the chosen parent (the build directory by default).

To run the Designer from a checkout instead:

```
cmake --preset debug
cmake --build --preset debug --target desktop generator_tool style_tests
# macOS:
./build/debug/OrganicMaps.app/Contents/MacOS/OrganicMaps --designer data/styles/default/light/style.mapcss
# Linux:
./build/debug/OrganicMaps --designer data/styles/default/light/style.mapcss
# Windows (PowerShell):
.\build\debug\OrganicMaps.exe --designer data/styles/default/light/style.mapcss
```

Launch it from the repository root: the writable dir then resolves to
`data/`, so rebuilt styles land where `generate_drules.sh` and
`generate_symbols.sh` put them. Re-run both scripts before committing: the
Designer skips the `drules_*.txt` dumps, `visibility.txt` and `optipng`, and
keeps unused `colors.txt`/`patterns.txt` entries. After discarding style edits,
reopen Designer or click **Build style** before **Recalculate geometry index**
to restore the indexes of git-ignored maps under `data/<version>/`.
Alternatively, delete or replace those reindexed map files; `git restore`
alone does not change their indexes.
Python 3 must be on `PATH` (`python` on Windows, `python3` elsewhere);
the drawing-rules writer (`libkomwm.py`)
is pure Python and needs no extra packages.
Designer is a runtime mode of the desktop app and needs no separate CMake flag.
Windows direct/Store runtime artifacts omit style sources and Designer helpers;
use a development checkout for style editing on Windows.

Pass any of the six supported `style.mapcss` files:

```
data/styles/{default,outdoors,vehicle}/{light,dark}/style.mapcss
```

Designer mode pins the map style to the file you opened and disables
the layer-menu Outdoors switch and the night-mode preference, which cannot
change the pinned style. Debug search commands cannot change that
style.  Edit any include file under
`data/styles/<type>/include/` (e.g. `Roads.mapcss`) and click **Build style**
to recompile and reapply the rules and symbol atlases live.
The window stays open.  Reload waits for tile readers to finish and rebuilds
renderer caches before returning to the GUI. Finish search requests before
**Build style**; simultaneous search and reload are unsupported. Type identities
stay fixed for the session because maps and type checkers retain them. Overlay
priority edits update rendering and preferred feature types immediately.
Designer changes the appearance of existing types; `classificator.txt`
and `types.txt` are fixed inputs. The compiler's copies stay in `out/` and are
never applied by Designer. To add or change types, close Designer, edit
`mapcss-mapping.csv` and run `tools/unix/generate_drules.sh` externally.
Regenerate affected maps from OSM to include new or reclassified features, then
reopen Designer. Do not edit the generated type-definition files manually.

Designer-only buttons:

- **Build style** — recompiles MapCSS via `libkomwm.py`, re-renders the symbol
  atlases in-process for the default family and reloads; results are written
  to the writable dir and the style's `out/`.  Startup recompiles the sources
  as well, so reopening Designer includes edits made between sessions.
  The atlases are pixel-identical to what `generate_symbols.sh` produces.
  The outdoors and vehicle families share the default family's atlases; build
  the corresponding default theme first when adding an icon used by another family.
  Enabling automatic geometry-index regeneration in Preferences makes a
  successful Build style close and relaunch Designer through the reindex step.
- **Recalculate geometry index** — closes the app, rebuilds
  `drules_merged.bin` (the zoom-range union of the light styles, which
  `generator_tool` indexes against), runs `generator_tool
  --generate_index=true` over maps directly inside the writable directory's
  version folders (including symlinked folders),
  then relaunches on success. World is copied into its dated writable map
  directory only when recalculation is requested. Indexing changes that copy,
  preserving bundled Worlds and the application bundle's signature. Newer
  bundled Worlds supersede older editable copies. Each map gets a separate
  temporary directory, including different versions with identical country names.
  Use this after widening a zoom range; narrowing one takes
  effect without it.  Features can only appear at zooms the map has geometry
  for, which `generator_tool --designer` widens by 3 levels when generating
  a map.
- **Debug style** — toggles drape's debug rect overlay.
- **Get statistics / Run tests** — invoke `drules_info.py` and `style_tests`.
- **Build phone package** — exports the currently edited style as a `styles/`
  folder to copy to a device (an existing one is replaced only after all rebuilt
  files are found and you confirm):
  - Android: `<storage>/Android/data/app.organicmaps/files/styles/`
  - iOS: Files → On My iPhone → Organic Maps → `styles/`

  Only the rebuilt `drules_<family>.bin` and `symbols/<dpi>/<theme>/`
  atlases are exported; the on-device StyleReader doesn't override
  `colors.txt`, `patterns.txt`, the classifier or feature mapping, so edits
  in those files still need a full app rebuild and reinstall.

To test on Android or iOS device either re-build the app or put
the compiled style files (e.g. `drules_default.bin`) into
a `styles/` subfolder of maps directory on the device
(e.g. `Android/data/app.organicmaps/files/styles/`).

Changing display zoom level for features (e.g. from z16- to z14-) might
not take effect until map's visibility/scale index is rebuilt:
1. [Build](INSTALL.md#desktop-app) the `generator_tool` binary
2. Put a map file, e.g. `Georgia.mwm` into the `data/` folder in the repository
3. Run `tools/unix/generate_drules.sh` so the merged indexing style includes your edits
4. Run from the repository root:
```
./build/debug/generator_tool --generate_index=true --output=Georgia --data_path=data --user_resource_path=data
```
5. The index of `Georgia.mwm` will be updated in place

A whole map needs to be [regenerated](MAPS.md) for the changes to take effect if:
* the visibility change crosses a geometry index boundary
* e.g. `area` style rules are added for a feature that didn't have them before
* a new feature type is added or the mapping of existing one is changed

## Technical details

Map style files syntax is based on [MapCSS/0.2](https://wiki.openstreetmap.org/wiki/MapCSS/0.2),
though the specification is not supported in full and there are OM-specific extensions to it.

The `tools/unix/generate_drules.sh` script uses a customized version of [Kothic](https://github.com/organicmaps/kothic)
stylesheet processor to compile MapCSS files into binary drawing rules files `data/drules_*.bin`.
The processor also produces text versions of these files (`data/drules_*.txt`) to ease debugging.

The `tools/unix/generate_symbols.sh` script assembles all icons into atlases for each density and theme
(`data/symbols/<dpi>/{light,dark}/symbols.png` and `symbols.xml`).

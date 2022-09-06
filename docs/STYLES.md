# Map styling and icons

Here is the basic workflow to update styles:
1. Edit the styles file you want, e.g. [`Roads.mapcss`](../data/styles/clear/include/Roads.mapcss)
2. Run the `tools/unix/generate_drules.sh` script
3. Test how your changes look in the app
4. Commit your edits and files changed by the script
5. Send a pull request!

Please prepend `[styles]` to your commit message and add [Developers Certificate of Origin](CONTRIBUTING.md#legal-requirements) to it.
Files changed by the script should be added as a separate `[styles] Regenerated` commit.

Please check [a list of current styling issues](https://github.com/organicmaps/organicmaps/issues?q=is%3Aopen+is%3Aissue+label%3AStyles)
and ["icons wanted" issues](https://github.com/organicmaps/organicmaps/issues?q=is%3Aopen+is%3Aissue+label%3AIcons+label%3A%22Good+first+issue%22).

## Requirements

To work with styles first [clone the OM repository](INSTALL.md#getting-sources).

Install a `protobuf` python package with `pip`
```
pip install protobuf
```
or with your OS package manager, e.g for Ubuntu 
```
sudo apt install python3-protobuf 
```

To run the `generate_symbols.sh` script install `optipng` also, e.g. for Ubuntu
```
sudo apt install optipng
```

## Files

Map styles are defined in text files located in `data/styles/clear/include/`:
* Forests, rivers, buildings, etc. [`Basemap.mapcss`](../data/styles/clear/include/Basemap.mapcss)
* Their text labels [`Basemap_label.mapcss`](../data/styles/clear/include/Basemap_label.mapcss)
* Roads, bridges, foot and bicycle paths, etc. [`Roads.mapcss`](../data/styles/clear/include/Roads.mapcss)
* Their text labels [`Roads_label.mapcss`](../data/styles/clear/include/Roads_label.mapcss)
* Icons for POIs and other features [`Icons.mapcss`](../data/styles/clear/include/Icons.mapcss)
* City-specific subway networks [`Subways.mapcss`](../data/styles/clear/include/Subways.mapcss)
* Light (default) theme colors: [`style-clear/colors.mapcss`](../data/styles/clear/style-clear/colors.mapcss)
* Dark/night theme colors: [`style-night/colors.mapcss`](../data/styles/clear/style-night/colors.mapcss)

There is a separate set of these style files for the navigation mode in `data/styles/vehicle/`.

Icons are stored in [`data/styles/clear/style-clear/symbols/`](../data/styles/clear/style-clear/symbols/) and their dark/night counterparts are in [`data/styles/clear/style-night/symbols/`](../data/styles/clear/style-night/symbols/).

## How to add a new icon

1. Add an svg icon to `data/styles/clear/style-clear/symbols/` (and to `style-night` too)
preferably look for icons in [collections OM uses already](../data/copyright.html#icons)
2. Add icon rendering/visibility rules into `data/styles/clear/include/Icons.mapcss` and to "navigation style" `data/styles/vehicle/include/Icons.mapcss`
3. Run `tools/unix/generate_symbols.sh` to add new icons into skin files
4. Run `tools/unix/generate_drules.sh` to generate drawing rules for the new icons
5. [Test](#testing-your-changes) your changes

## How to add a new map feature / POI type

1. Add it into `data/mapcss-mapping.csv` (or better replace existing `deprecated` line) to make OM import it from OSM
2. If necessary merge similar tags in via `data/replaced_tags.txt`
3. Add a new icon (see [above](#how-to-add-a-new-icon))
4. If a new POI should be OSM-addable/editable then add it to `data/editor.config`
5. Add new type translation into `data/strings/types_strings.txt`
6. Add search keywords into `data/categories.txt`
7. Run `tools/unix/generate_localizations.sh` to validate and distribute translations into iOS and Android
8. Add new or fix current classifier tests at `/generator/generator_tests/osm_type_tests.cpp` if you can
9. [Test](#testing-your-changes) your changes
10. Relax and wait for the next maps update :)

## Testing your changes

The most convenient way is using [the desktop app](INSTALL.md#desktop-app).
There is a Designer version of it also, which facilitates development
by rebuilding styles and symbols quickly. Though building of the Designer tool
is limited to macOS at the moment.

To test on Android or iOS device either re-build the app or put
the compiled style files (e.g. `drules_proto_clear.bin`) into
a `styles/` subfolder of maps directory on the device
(e.g. `Android/data/app.organicmaps/files/styles/`).

Changing display zoom level for features (e.g. from z16- to z14-) might
not take effect until map's visibility/scale index is rebuilt:
1. [Build](INSTALL.md#desktop-app) the `generator_tool` binary
2. Put a map file, e.g. `Georgia.mwm` into the `data/` folder in the repo
3. Run
```
../omim-build-release/generator_tool --generate_index=true --output="Georgia"
```
4. The index of `Georgia.mwm` will be updated in place

Sometimes (if the visibility change crosses a geometry index boundary)
a whole map needs to be [regenerated](MAPS.md) for the feature to become visible.

If e.g. `area` style rules are added for a feature that didn't have them before,
then the rules won't take effect until map files are regenerated.

## Technical details

Map style files syntax is based on [MapCSS/0.2](https://wiki.openstreetmap.org/wiki/MapCSS/0.2),
though the specification is not supported in full and there are OM-specific extensions to it.

The `tools/unix/generate_drules.sh` script uses a customized version of [Kothic](https://github.com/kothic/kothic)
stylesheet processor to compile MapCSS files into binary drawing rules files `data/drules_proto*.bin`.
The processor also produces text versions of these files (`data/drules_proto*.txt`) to ease debugging.

The `tools/unix/generate_symbols.sh` script assembles all icons into skin files in various resolutions (`data/resources-*/symbols.png` and `symbols.sdf`).

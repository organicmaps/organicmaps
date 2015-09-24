# MAPS.ME

http://maps.me

This is an application for [Android](https://play.google.com/store/apps/details?id=com.mapswithme.maps.pro)
and [iOS](https://itunes.apple.com/app/id510623322) devices for offline maps built from OpenStreetMap data.

## Compilation

You will need Qt 5.3 or later to build the project. On Mac OS X systems we recommend installing
XCode and [Homebrew](http://brew.sh/). See `build_omim.sh` script in `tools/unix` directory:
it builds debug and release versions of the desktop application and map generator tool, as well
as routing backend.

For detailed installation instructions and Android/iOS building process,
see [INSTALL.md](https://github.com/mapsme/omim/tree/master/docs/INSTALL.md).

Nightly builds for Android and iOS are published to [osmz.ru](http://osmz.ru/mwm/)
and Dropbox: [release](http://maps.me/release), [debug](http://maps.me/debug).

## Building maps

To create one or many map files, first build the project, then use `generate_mwm.sh` script from
`tools/unix` to create a single mwm file from pbf/o5m/bz2 source, or `generate_planet.sh`
to generate multiple countries at once from a planet o5m file. See detailed instructions
in [MWM.md](https://github.com/mapsme/omim/tree/master/docs/MWM.md).

## Map styles

MAPS.ME uses its own binary format for map styles, `drules_proto.bin`, which is compiled from
[MapCSS](http://wiki.openstreetmap.org/wiki/MapCSS) using modified Kothic library.
Feature set in MWM files depends on a compiled style, so make sure to rebuild maps after
releasing a style.

For development, use MAPS.ME Designer app along with its generator tool: these allow
for quick rebuilding of a style and symbols, and for producing a zoom-independent
feature set in MWM files.

See [STYLES.md](https://github.com/mapsme/omim/tree/master/docs/STYLES.md) for the
format description, instructions on building a style and some links.

## Development

You would need Qt 5 for development, most other libraries are included into the
repository, see `3party` directory. After cloning, the repository must be
initialized with `configure.sh`. The team uses mostly XCode and Qt Creator,
though these are not mandatory. We have an established
[coding style](https://github.com/mapsme/omim/blob/master/docs/cpp_coding_standard.txt).
Our pull request review process may be intimidating, but it ensures a consistent
code quality.

All contributors must sign a [Contributor Agreement](https://github.com/mapsme/omim/blob/master/docs/CLA.md),
so both ours and theirs rights are protected.

See [CONTRIBUTING.md](https://github.com/mapsme/omim/blob/master/docs/CONTRIBUTING.md)
for the repository initialization process, the description of all the directories
of this repository and other development-related information.

## Feedback

Please report bugs and suggestions to [the issue tracker](https://github.com/mapsme/omim/issues),
or by mail to bugs@maps.me.

## Authors and License

This source code is Copyright (C) 2011-2015 MAPS.ME, published under Apache Public License 2.0,
except third-party libraries. See [NOTICE](https://github.com/mapsme/omim/blob/master/NOTICE)
file for more information.

# MWM Files

MAPS.ME uses maps in its own vector format, MWM. It contains classified features sorted and simplified by zoom level.
For car routing, it needs a separate routing index in a `.mwm.routing` file. We build maps for the entire planet:

* [daily.mapswithme.com/direct/latest](http://direct.mapswithme.com/direct/latest/) - official maps bundled with releases.

## Building

For building maps, you need compiled Generator Tool and, for routing indices, OSRM backend.
See [INSTALL.md](INSTALL.md) for compilation instructions. We usually use this line:

    CONFIG=gtool omim/tools/unix/build_omim.sh -cro

Scripts described here require OSM C Tools, which are maintained as a submodule of the omim repository.

### A Single Region

Having built a generator tool, prepare a source file in pbf/o5m/bz2 format, and run:

    omim/tools/unix/generate_mwm.sh source.pbf

In some minutes it will create a similarly-named `.mwm` file in the same directory as the original file.
Specify `TARGET` variable for changing that (e.g. `TARGET=.`). The script runs `generator_tool` twice,
see `find_generator_tool.sh` script for an algorithm on how it finds it. All temporary files are created
with `mktemp` and then removed.

The resulting file won't have any coastlines, though MAPS.ME will overlay zoomed-in map with a low-quality
generalized coastline. To add a detailed coastline, you would need a `WorldCoasts.geom` file and
a [border polygon](http://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format) for a source
file area. Having that, run (and prepare to wait a bit longer):

    COASTS=WorldCoasts.geom BORDER=source.poly omim/tools/unix/generate_mwm.sh source.pbf

A car routing index will be built when you specify a second parameter: either a full path to a Lua script
with a routing profile, or any gibberish, in which case a default `car.lua` from omim repository
would be used. For example:

    omim/tools/unix/generate_mwm.sh source.pbf asdf

Inter-mwm navigation requires another index inside a `.mwm.routing` file. To build it, you would need
border polygons for not only the source region, but all regions neighbouring it. The source border polygon
must have the same name as the source file (e.g. `Armenia.poly` for `Armenia.pbf`), and in the target
directory shouldn't be a `borders` subdirectory. With all that, just use this line:

    BORDERS_PATH=/path/to/polygons omim/tools/unix/generate_mwm.sh source.pbf asd

### The Planet

To create a bunch of MWM files for the entire planet, splitting it by border polygons, we have
a different script, `generate_planet.sh`. It will print a short help when run without arguments.
The usual line we use is:

    TARGET=/opt/mwm/151231 omim/tools/unix/generate_planet.sh -a

This is a shortcut for following options:

* `-u` - update a planet file from osm.org (use `-U` when you need to download one, specify `PLANET`
variable if it's not in `$HOME/planet/planet-latest.o5m`).
* `-l` - filter and process coastlines, creating `WorldCoasts.geom` and `.rawgeom` files.
* `-w` - generate overview maps, `World.mwm` and `WorldCoasts.mwm`.
* `-r` - generate routing indices, `.mwm.routing` file for each `.mwm`.

All border polygons from `BORDERS_PATH` are processed into MWM files by default. You can
specify only required polygons in `REGIONS` variable, or set it to empty value, so no regular
MWM files are created. The whole process takes 15 hours on a 40-core server, so we suggest
you specify your e-mail address in a `MAIL` variable and get a mail when the generation
is finished.

If a previous run ended with an error, the next one will ignore arguments and continue with
the same arguments as the previous time. Set `-c` option to ignore the stored status.

Log files for each region and the entire process (`generate_planet.log`) are written to
`logs` subdirectory of the target. Intermediate data requires around 250 MB of space, and
to clean it during the process, specify `KEEP_INTDIR=` empty variable.

#### Steps

The planet generation process is divided in several steps, which are printed during the
script run, along with timestamps. To start generation from a specific step, specify it
in the `MODE` variable (and make sure you don't have stored status, or run with `-c`
option).

* Step 1 (`coast`): updating planet file.
* Step 2: filtering and processing coast lines. If there was a merge error, the script
prints way identifiers, waits 40 minutes and tries again from step 1.
* Step R: preparing `.osrm` files for routing indices. PBF files for each region are
cut out of the planet, then OSRM backend scripts process each of these. Can work
asynchronously if `ASYNC_PBF=1` variable is set.
* Step 3 (`inter`): generating intermediate data for the planet.
* Step 4 (`features`): generating features for each region, splitting the planet.
* Step 5 (`mwm`): building the resulting MWMs.
* Step 6 (`routing`): building `.mwm.routing` files out of MWMs and `.osrm` files.
* Step 7 (`resources`): updating resource and map lists.
* Step 8 (`test`): calling `test_planet.sh` to run routing tests.

### Variables

*todo*

### Testing

To test that the generator is not broken, you don't have to wait for 20 hours processing
the whole planet: instead download [a 150 MB extract](http://osmz.ru/mwm/islands/) with
some islands and the corresponding script `islands.sh` and put these into `omim/tools/unix`.
Run the script with options for `generate_planet`, e.g.

    omim/tools/unix/islands.sh -lwr

In a half an hour you'll get files for 4 regions in a `target` subdirectory. Note that
you can build both generator tool and OSRM backend with the `build_omim.sh` script.

## Format

*todo*

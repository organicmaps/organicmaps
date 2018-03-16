# MWM Files

MAPS.ME uses maps in its own vector format, MWM. It contains classified features sorted and simplified by zoom level.
It also can include a pre-calculated routing index for car routing. We build maps for the entire planet:

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
a [border polygon](https://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format) for a source
file area. Having that, run (and prepare to wait a bit longer):

    COASTS=WorldCoasts.geom BORDER=source.poly omim/tools/unix/generate_mwm.sh source.pbf

Inter-mwm navigation requires another index. To build it, you would need
border polygons for not only the source region, but all regions neighbouring it. The source border polygon
must have the same name as the source file (e.g. `Armenia.poly` for `Armenia.pbf`), and in the target
directory shouldn't be a `borders` subdirectory. With all that, just use this line:

    BORDERS_PATH=/path/to/polygons omim/tools/unix/generate_mwm.sh source.pbf

### The Planet

To create a bunch of MWM files for the entire planet, splitting it by border polygons, we have
a different script, `generate_planet.sh`. It will print a short help when run without arguments.
The usual line we use is:

    TARGET=/opt/mwm/151231 omim/tools/unix/generate_planet.sh -a

This is a shortcut for following options:

* `-u`: update a planet file from osm.org (use `-U` when you need to download one, specify `PLANET`
variable if it's not in `$HOME/planet/planet-latest.o5m`).
* `-l`: filter and process coastlines, creating `WorldCoasts.geom` and `.rawgeom` files.
* `-w`: generate overview maps, `World.mwm` and `WorldCoasts.mwm`.
* `-r`: include for each `.mwm` routing index and keep a non routing version as `.mwm.norouting`.

All border polygons from `BORDERS_PATH` are processed into MWM files by default. You can
specify only required polygons in `REGIONS` variable, or set it to empty value, so no regular
MWM files are created. The whole process takes 15 hours on a 40-core server, so we suggest
you specify your e-mail address in a `MAIL` variable and get a mail when the generation
is finished.

If a previous run ended with an error, the next one will ignore arguments and continue with
the same arguments as the previous time. Set `-c` option to ignore the stored status.

Log files for each region and the entire process (`generate_planet.log`) are written to
`logs` subdirectory of the target. Intermediate data requires around 320 GB of space, and
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
* Step 6 (`routing`): building routing indices out of *.osrm* files.
* Step 7 (`resources`): updating resource and map lists.
* Step 8 (`test`): calling `test_planet.sh` to run routing tests.

### Variables

Almost every default in the script can be redefined with environment variables.
You will have to use some of these, unless you are using our map-building servers.

* `GENERATOR_TOOL`: a location of `generator_tool` program. Example: `~/omim-build-debug/out/debug/generator_tool`.
* `BUILD_PATH`: a path to either `generator_tool` or its build directory. Example: `~/omim-build-debug`.
* `PLANET`: pa ath or name of the planet file. If there is no file, specify `-U` option,
and it will be downloaded. Should be in o5m format. Default is `~/planet/planet-latest.o5m`.
* `TARGET`: a target path for `mwm` and `routing` files. Some temporary subdirectories
will be created inside: `logs`, `borders` and `intermediate_data`.
* `MAIL`: comma-separated e-mail addresses, which will receive notifications about
finished or failed generation.
* `OSMCTOOLS`: a path to pre-compiled OSM C Tools: `osmconvert`, `osmupdate` and
`osmfilter`. If these were not found, it will compile sources from the repo.
* `OMIM_PATH`: a path to the omim repository root. Will be guessed when the
script is not moved from `omim/tools/unix`. It is needed for locating a data
directory (`omim/data`, can be overridden with `DATA_PATH`), generator tool
build path (see `BUILD_PATH`) and OSRM backend scripts (see `OSRM_PATH`).
* `OSRM_PATH`: a path to `omim/3party/osrm/osrm-backend`. Needed for searching
for a routing profile (`$OSRM_PATH/profiles.car.lua` by default, but can be
overridden with `PROFILE`) and for osrm executables (`$OSRM_PATH/build`,
or set `OSRM_BUILD_PATH`).
* `DATA_PATH`: a path to classificators and border polygons; the latter can
be redefined with `BORDERS_PATH`.
* `INTDIR`: a temporary directory that is created when the script starts, and
removed when `KEEP_INTDIR` is clean and the script ends. Contains `status` file
that keeps script arguments for resuming processing, and `osrm_done` file,
which is a flag for successful OSRM indices building process.
* `KEEP_INTDIR`: if empty (by default it is not), the temporary directory will
be deleted when the script finishes. Note that it might have `WorldCoasts.geom`
file, built on step 2, which is required for generating coastlines (step 4).
* `NODE_STORAGE` (or `NS`): where is a node cache kept: in memory (`mem`, which
is the default), or on disk (`map`). Tests show that for a complete world,
with `map` the process eats some hundreds of gigabytes and crashes, while with
`mem` the memory consumption is stable at around 40 GB.
* `ASYNC_PBF`: by default, pbf files for routing are built between steps 2 and 3,
but if this flag is set (e.g. to `1`), they are built asynchronously. But
it can fail due to low memory.
* `MERGE_INTERVAL`: delay in minutes between attempts to merge a coast line.
Default is 40.
* `REGIONS`: a list of `.poly` files for regions to be built. One for each line.
* `DELTA_WITH`: a path to an older map directory, to compare with the freshly
generated data in the testing step.
Can be empty. Example: `$(ls ../../data/borders/{UK*,Ireland}.poly)`.
* `OSRM_URL`: address of an OSRM server to calculate worldwide routes.
* `SRTM_PATH`: a path to `*.zip` files with SRTM data.
* `OSC`: a path to an osmChange file to apply after updating the planet.
* `BOOKING_FILE`: a path to hotels.csv with booking data.
* `BOOKING_USER` and `BOOKING_PASS`: user name and password for booking.com API
* `OPENTABLE_FILE`: a path to restaurants.csv with opentable data.
* `OPENTABLE_USER` and `OPENTABLE_PASS`: user name and password for opentable.com API
to download hotels data.

### Testing

To test that the generator is not broken, you don't have to wait for 20 hours processing
the whole planet: instead download [a 150 MB extract](http://osmz.ru/mwm/islands/) with
some islands and the corresponding script `islands.sh` and put these into `omim/tools/unix`.
Run the script with options for `generate_planet`, e.g.

    omim/tools/unix/islands.sh -lwr

In a half an hour you'll get files for 4 regions in a `target` subdirectory. Note that
you can build both generator tool and OSRM backend with the `build_omim.sh` script.

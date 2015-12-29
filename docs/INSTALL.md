# Building MAPS.ME

First, do not forget to initialize a cloned repository, see
[CONTRIBUTING.md](CONTRIBUTING.md).

## Desktop

You would need Clang, Boost and Qt 5. With that, just run `omim/tools/unix/build_omim.sh`.
It will build both debug and release versions to `omim/../omim-build-<target>`, as
well as OSRM backend for generating maps. Command-line switches are:

* `-r` to build a release version
* `-d` to build a debug version
* `-o` to build OSRM backend
* `-c` to delete target directories before building

To build a generator tool only, set `CONFIG=gtool` variable. To skip building tests,
use `CONFIG=no-tests`. If you have Qt installed in an unusual directory, use
`QMAKE` variable.

When using a lot of maps, increase open files limit, which is only 256 on Mac OS X.
Use `ulimit -n 2000`, put it into `~/.bash_profile` to apply it to all new sessions.
In OS X to increase this limit globally, add `limit maxfiles 2048 2048` to `/etc/launchd.conf`
and run

    echo 'ulimit -n 2048' | sudo tee -a /etc/profile

### Building Manually

The `build_omim.sh` script basically runs these commands:

    qmake omim.pro -spec linux-clang-libc++ CONFIG+=debug
    make -j <number_of_processes>

It will compile binaries to the `out` subdirectory of the current directory.
You might need to export `BOOST_INCLUDEDIR` variable with a path to Boost's
`include` directory.

To build the OSRM backend, create `omim/3party/osrm/osrm-backend/build`
directory, and from within it, run:

    cmake -DBOOST_ROOT=<where_is_your_Boost> ..
    make

### Ubuntu 14.04

Install Qt 5.5:

    sudo add-apt-repository ppa:beineri/opt-qt551-trusty
    sudo apt-get update
    sudo apt-get install qt55base

Set up the Qt 5.5 environment:

    source /opt/qt55/bin/qt55-env.sh

Do a git clone:

    git clone --depth=1 --recursive https://github.com/mapsme/omim.git
    cd omim
    echo | ./configure.sh

On Ubuntu 14.04, you'll need a PPA with an up-to-date version of libc++.
You shouldn't need this on newer versions.

    sudo add-apt-repository ppa:jhe/llvm-toolchain

Then:

    sudo apt-get install clang-3.6 libc++-dev libboost-iostreams-dev libglu1-mesa-dev
    sudo ln -s /usr/lib/llvm-3.6/bin/clang /usr/bin/clang
    sudo ln -s /usr/lib/llvm-3.6/bin/clang++ /usr/bin/clang++
    tools/unix/build_omim.sh

Prepend with `CONFIG=gtool` if only generator_tool is needed. You would need 1.5 GB of memory
to compile `stats` module.

The generated binaries appear in `omim-build-<flavour>/out/<flavour>/`.
Run tests from this directory with `../../../omim/tools/unix/run_tests.sh`.

To build and run OSRM binaries:

    sudo apt-get install libtbb2 libluabind0.9.1 liblua50 libstxxl1
    sudo apt-get install libtbb-dev libluabind-dev libstxxl-dev libosmpbf-dev libprotobuf-dev
    sudo apt-get install libboost-thread-dev libboost-system-dev libboost-program-options-dev
    sudo apt-get install libboost-filesystem-dev libboost-date-time-dev
    tools/unix/build_omim.sh -o

### Windows

We haven't compiled MAPS.ME on Windows in a long time, though it is possible. It is likely
some make files should be updated. If you succeed, please submit a tutorial.

See also [Android compilation instructions](android_toolchain_windows.txt) (also possibly outdated).

### Download maps

To browse maps in an application, you need first to download some. We maintain an archive
of all released versions of data at [direct.mapswithme.com](http://direct.mapswithme.com/direct/).
Place `.mwm` and `.mwm.routing` files to `~/Library/Application Support/MapsWithMe` for
a desktop version. Alternatively, you can put these into `omim/data`, but there
should be a soft link in a build directory: `build_omim.sh` creates it.

For an Android application, place maps into `/MapsWithMe` directory on a device. For
iOS devices, use iTunes.

`World.mwm` and `WorldCoasts.mwm` are low-zoom overview maps and should be placed
into a resource directory, e.g. `/Applications/MAPS.ME/Content/Resources` on Mac OS X.
Placing these into a maps directory should also work.

For instructions on making your own maps, see [MAPS.md](MAPS.md).

## Maps Generator

The generator tool is build together with the desktop app, but you can choose to skip
other modules. Use this line:

    CONFIG=gtool omim/tools/unix/build_omim.sh -ro

It is the preferable way to build a generator tool, for it can also build an OSRM
backend (`-o` option).

Dependencies for generator tool and OSRM backend:

* boost-iostreams
* glu1-mesa
* tbb
* luabind
* stxxl
* osmpbf
* protobuf
* lua

## Designer Tool

The designer tool resides in a branch `map-style-editor-new`. You would need
to check it out before following these steps.

### Building data

* Run `CONFIG="gtool map_designer" omim/tools/unix/build_omim.sh`.
* Generate data as usual (either with `generate_mwm.sh` or `generate_planet.sh`).
* For MAPS.ME employees, publish planet data to http://designer.mapswithme.com/mac/DATA_VERSION
(via a ticket to admins, from `mapsme4:/opt/mapsme/designers`).

### Building the tool

* Run `omim/tools/unix/build_designer.sh DATA_VER APP_VER`, where `DATA_VER` is the
latest data version published to `designer.mapswithme.com` (e.g. `150912`), and
`APP_VER` is an application version, e.g. `1.2.3`.
* If you got "hdiutil -5341" error, you would need to build a dmg package yourself:
find a line with `hdiutil` in the script, copy it to a console, and if needed, increase
`-size` argument value.
* The resulting dmg package will be put into `omim/out`.

## Android

* Install [Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html), place these somewhere
easy to type.

* Go to `omim/tools/android` and run `./set_up_android.py`. It would ask for absolute paths
to SDK and NDK. Or specify these in command line:

        --sdk /Users/yourusername/Library/Android/sdk \
        --ndk /Users/yourusername/Library/Android/ndk

* Go to `omim/android` and run `./gradlew clean assembleWebRelease`.
    * There are `Release`, `Beta` and `Debug` builds.
    * `assemble` produces apk files in `build/outputs/apk`, `install` installs one
        on a connected device, and `run` immediately starts it.
    * `Web` creates a full apk with World files, `Google` builds a store version
        without these, and there are also `Amazon`, `Samsung`, `Xiaomi`, `Yandex`
        and other targets.

* If you encounter errors, try updating your Android SDK:
    * In [SDK Manager](http://developer.android.com/tools/help/sdk-manager.html)
        (for Android Studio: Tools → Android → SDK Manager) enable fresh
        _build-tools_ and click on "Install packages..."
    * In command line, go to `android-sdk/tools`, then
        * `./android list sdk` for a list of packages to update.
        * `./android update sdk --no-ui --filter 1,2,3` to update chosen packages.

* To enable logging in case of crashes, after installing a debug version, run:
        adb shell pm grant com.mapswithme.maps.pro.debug android.permission.READ_LOGS
    After a crash, go to "Menu → Settings → Report a bug" and enter your e-mail to
    receive a log file.

## iOS

For XCode configuration instructions, see [CONTRIBUTING.md](CONTRIBUTING.md).

* Open `omim/iphone/Maps/Maps.xcodeproj` in XCode.
* Open "Product → Scheme → Edit Scheme", then "Info" and change build configuration to Simulator.
* Run the project (Product → Run).

If a script has trouble finding your Qt 5 installation, edit `omim/tools/autobuild/detect_qmake.sh`,
adding a path to `qmake` there.

## Map Servers

The "guest" configuration does not have any map server addresses built-in. That means, while your
application would work well with [downloaded maps](http://direct.mapswithme.com/direct/latest/),
it won't be able to download them by itself. To fix this, add some servers to
`DEFAULT_URLS_JSON` define in the `private.h` file. These servers should have mwm files
in `/maps/151231` paths, symlinked to `/ios`, `/android`, `/mac` and `/win` (depending on operating
systems from which your maps will be downloaded).

`151231` is a version number, which should be a six-digit integer, usually in form
`YYMMDD` of the date map data was downloaded. The version and file sizes of all mwm and
routing files should be put into `data/countries.txt` file.

Android application may also download some resources - fonts and World files - from the same
servers. It checks sizes of existing files via `external_resources.txt`, and if some of these
don't match, it considers them obsolete and downloads new resource files.

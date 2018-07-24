# Building MAPS.ME

First, do not forget to initialize a cloned repository, see
[CONTRIBUTING.md](CONTRIBUTING.md).

## Desktop

You would need Cmake, Boost and Qt 5. With that, just run `omim/tools/unix/build_omim.sh`.
It will build both debug and release versions to `omim/../omim-build-<target>`.
Command-line switches are:

* `-r` to build a release version
* `-d` to build a debug version
* `-c` to delete target directories before building
* `-s` to not build a desktop app, when you don't have desktop Qt libraries.
* `-p` with a path to where the binaries will be built.

After switches, you can specify a target (everything by default). For example,
to build a generator tool only, use `generator_tool`.  If you have Qt installed
in an unusual directory, use `QMAKE` variable. You can skip building tests
with `CMAKE_CONFIG=-DSKIP_TESTS` variable.

When using a lot of maps, increase open files limit, which is only 256 on Mac OS X.
Use `ulimit -n 2000`, put it into `~/.bash_profile` to apply it to all new sessions.
In OS X to increase this limit globally, add `limit maxfiles 2048 2048` to `/etc/launchd.conf`
and run

    echo 'ulimit -n 2048' | sudo tee -a /etc/profile

### Building Manually

The `build_omim.sh` script basically runs these commands:

    cmake <path_to_omim> -DCMAKE_BUILD_TYPE={Debug|Release}
    make [<target>] -j <number_of_processes>

It will compile binaries to the current directory. For a compiler, Clang
is preferred, but GCC 6+ would also work.

### Ubuntu 16.04

Install Qt 5, CMake and Clang:

    sudo apt-get update
    sudo apt-get install qtbase5-dev cmake
    sudo apt-get install clang libc++-dev libboost-iostreams-dev libglu1-mesa-dev

Do a git clone:

    git clone --depth=1 --recursive https://github.com/mapsme/omim.git
    cd omim
    echo | ./configure.sh

Then:

    tools/unix/build_omim.sh -r

Append `generator_tool` if only generator_tool is needed, or `desktop` if you need a desktop app.
You would need 1.5 GB of memory to compile the `stats` module.

The generated binaries appear in `omim-build-release`.
Run tests from this directory with `omim/tools/unix/run_tests.sh`.

### Fedora 27

Install dependencies:

    dnf install clang qt5-qtbase-devel boost-devel libstdc++-devel

Then do a git clone, run `configure.sh` and compile with linux-clang spec:

    SPEC=linux-clang tools/unix/build_omim.sh -r

### Windows

We haven't compiled MAPS.ME on Windows in a long time, though it is possible. It is likely
some make files should be updated. If you succeed, please submit a tutorial.

See also [Android compilation instructions](android_toolchain_windows.txt) (also possibly outdated).

### Download maps

To browse maps in an application, you need first to download some. We maintain an archive
of all released versions of data at [direct.mapswithme.com](http://direct.mapswithme.com/direct/).
Place `.mwm` files to `~/Library/Application Support/MapsWithMe` for
a desktop version.

For an Android application, place maps into `/MapsWithMe` directory on a device. For
iOS devices, use iTunes.

`World.mwm` and `WorldCoasts.mwm` are low-zoom overview maps and should be placed
into a resource directory, e.g. `/Applications/MAPS.ME/Content/Resources` on macOS.
Placing these into a maps directory should also work.

For instructions on making your own maps, see [MAPS.md](MAPS.md).

## Maps Generator

The generator tool is build together with the desktop app, but you can choose to skip
other modules. Use this line:

    omim/tools/unix/build_omim.sh -sr generator_tool

Dependencies for generator tool:

* boost-iostreams
* glu1-mesa
* protobuf

## Designer Tool

### Building the tool

The designer tool has been merged into the master branch. You can build a package for macOS with:

    omim/tools/unix/build_omim.sh -rt

* If you got "hdiutil -5341" error, you would need to build a dmg package yourself:
find a line with `hdiutil` in the script, copy it to a console, and if needed, increase
`-size` argument value.
* The resulting dmg package will be put into `omim/out`.

### Building data

* Build both the generator_tool and the designer tool
* Generate data as usual (either with `generate_mwm.sh` or `generate_planet.sh`).
* For MAPS.ME employees, publish planet data to http://designer.mapswithme.com/mac/DATA_VERSION
(via a ticket to admins, from `mapsme4:/opt/mapsme/designers`).

## Android

* Install [Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html), place these somewhere
easy to type.

* Go to `omim/tools/android` and run `./set_up_android.py`. It would ask for absolute paths
to SDK and NDK. Or specify these in command line:

        --sdk /Users/$(whoami)/Library/Android/sdk \
        --ndk /Users/$(whoami)/Library/Android/ndk

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

* Install [Homebrew](http://brew.sh/) and run `brew install carthage`.
* Open `xcode/omim.xcworkspace` in XCode.
* Select "xcMAPS.ME" product scheme for developing, or "MAPS.ME" to make a distribution package.
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

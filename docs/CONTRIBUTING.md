# MAPS.ME Development

## Initializing the Repository

The repository needs a lot of header and configuration files with private keys and such.
To initialize, run `configure.sh` from its root, and press "Enter" when asked for a
repository. The script will create two files, `private.h` and `android/secure.properties`,
which are required for compiling the project. If you have a private repository with
these files (and some other, like private keystores), pass the link like this:

    echo git@repository:org/omim-private.git | ./configure.sh

Without keys, downloading of maps won't work, as well as statistics and online routing
assisting. For android, you would need private keys to build a release version (but
a debug one can be built without keys).

## Setting up IDE

See [INSTALL.md](INSTALL.md) for command-line compilation instructions.

* Install XCode and Command Line Tools, then run XCode and click "I Agree".

### Qt Creator

* Download [Qt Creator](http://www.qt.io/download-open-source/) with Qt 5.
* In `Qt/5.5/clang_64/mkspecs/qdevice.pri` replace `10.10` with `10.11`, and
    add a line `QMAKE_MAC_SDK = macosx10.11` (see installed versions in
    `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/`)
* Start Qt Creator, open `omim.pro`, choose "qt" target and run the project.
* To build the project faster, open "Project Settings", find "Build Steps", then
    "Make Arguments" and put "-j8" there (without quotes).

Debugging may not work in Qt Creator. To enable it, try creating a Build & Run kit in
preferences with XCode's lldb as a debugger and a clang compiler.

### XCode

* Install [Homebrew](http://brew.sh/) and run `brew install qt5`.
* Run XCode, open `xcode/omim.xcworkspace`.
* Select "MapsMe" scheme and run the product.

## Coding Style

See [CPP_STYLE.md](CPP_STYLE.md).

## Directories

### Sources

* **anim** - core animation controller.
* **api** - external API of the application.
* **base** - 

*todo*

### Other

* **3party** - external libraries, sometimes modified.
* **android** - Android UI

*todo*

## Pull Requests

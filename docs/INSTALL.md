# Building

- [Desktop](#desktop-app)
- [Android](#android-app)
- [iOS](#ios-app)

## Desktop app

### Preparing

You need a Linux or a Mac machine to build OMaps.

 - We haven't compiled OMaps on Windows in a long time, though it is possible.
   It is likely some make files should be updated.
   If you succeed, please submit a tutorial.

Ensure that you have at least 20GB of free space.

Install Cmake, Boost, Qt 5 and other dependencies.

**Ubuntu 20.04**:

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    cmake \
    clang \
    python \
    qtbase5-dev \
    libqt5svg5-dev \
    libc++-dev \
    libboost-iostreams-dev \
    libglu1-mesa-dev \
    libsqlite3-dev \
    zlib1g-dev
```

**Fedora 33**:

```bash
sudo dnf install -y \
    clang \
    qt5-qtbase-devel \
    qt5-qtsvg-devel \
    boost-devel \
    libstdc++-devel \
    sqlite-devel
```

**macOS**:

```bash
brew install qt cmake
```

### Getting sources

Clone the repository:

```bash
git clone --recursive https://github.com/organicmaps/organicmaps.git
```

Default clone destination directory is organicmaps.

Update git submodules (sometimes doesn't work automatically):

```bash
git submodule update --init --recursive
```

Configure the repository as opensource build:

```bash
./configure.sh
```

or with private repository

```bash
./configure.sh <private-repo-name>
```

You can check usage `./configure.sh --help`

### Building

With all tools installed, just run `tools/unix/build_omim.sh`.
It will build both debug and release versions to `../omim-build-<buildtype>`.
Command-line switches are:

* `-r` to build a release version
* `-d` to build a debug version
* `-c` to delete target directories before building
* `-s` to not build a desktop app, when you don't have desktop Qt libraries.
* `-p` with a path to where the binaries will be built.

After switches, you can specify a target (everything by default). For example,
to build a generator tool release version only:

```
tools/unix/build_omim.sh -r generator_tool
```

Targets list can be viewed:

```bash
cmake --build ../omim-build-<buildtype> --target help
```

If you have Qt installed in an unusual directory, use `QT_PATH` variable (`SET (QT_PATH "your-path-to-qt")`). You can skip building tests
with `CMAKE_CONFIG=-DSKIP_TESTS` variable. You would need 1.5 GB of memory
to compile the `stats` module.

The `build_omim.sh` script basically runs these commands:

    cmake <path_to_omim> -DCMAKE_BUILD_TYPE={Debug|Release}
    make [<target>] -j <number_of_processes>


### Running

The generated binaries appear in `../omim-build-<buildtype>`.

Run `OMaps` binary from `../omim-build-<buildtype>`, for example, for release:

**Linux**:

```bash
../omim-build-release/OMaps -data_path ./data
```

or create `data` symlink in build dir to `organicmaps/data` directory and run

```bash
cd ../omim-build-release
ln -s ../organicmaps/data ./data
./OMaps -data_path ./data
```

**macOS**:

```bash
../omim-build-release/OMaps.app/Contents/MacOS/OMaps
```

When using a lot of maps, increase open files limit, which is only 256 on Mac OS X.
Use `ulimit -n 2000`, put it into `~/.bash_profile` to apply it to all new sessions.
In OS X to increase this limit globally, add `limit maxfiles 2048 2048` to `/etc/launchd.conf`
and run

    echo 'ulimit -n 2048' | sudo tee -a /etc/profile

### Testing

Compile all unit tests in Debug mode:

```bash
cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target all
```

Run all unit tests:

```bash
cd build
../tools/python/run_desktop_tests.py -f . -u ../data/ -d ../data/
```

To run a limited set of tests, use `-i` flag. To exclude some tests, use `-e` flag:

```bash
cd build
../tools/python/run_desktop_tests.py -f . -u ../data/ -d ../data/ -i base_tests,coding_tests
../tools/python/run_desktop_tests.py -f . -u ../data/ -d ../data/ -e routing_integration_tests
```

When developing, it is more convenient to use a symlink:

```bash
cd build
ln -s ../data/ data
./coding_tests
```

Some tests [are known to be broken](https://github.com/organicmaps/organicmaps/issues?q=is%3Aissue+is%3Aopen+label%3ATests).

## Android app

### Preparing

You need a Linux or a Mac machine to build OMaps for Android.

 - We haven't compiled OMaps on Windows in a long time, though it is possible.
   It is likely some make files should be updated.
   If you succeed, please submit a tutorial.

Ensure that you have at least 20GB of free space.

Install [Android Studio](https://developer.android.com/studio).

Install [Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html):

 - Run the Android Studio.
 - Open "SDK Manager" ("Tools" → "SDK Manager").
 - Choose "Android 10 (Q) API Level 29" SDK.
 - Choose "version "29" and click "OK".
 - Open "SDK Tools", choose "NDK (side by side)" and "CMake" and click "OK"

Alternatively, you can install only
[Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html) without
installing Android Studio.

### Getting sources

Clone the repository:

```bash
git clone --recursive https://github.com/organicmaps/organicmaps.git
```

Update git submodules (sometimes doesn't work automatically):

```
git submodule update --init --recursive
```

Configure the repository as opensource build:

```bash
./configure.sh

```

or with private repository

```bash
./configure.sh <private-repo-name>
```

Set Android SDK and NDK path:

```bash
# Linux
./tools/android/set_up_android.py --sdk $HOME/Android/Sdk
# MacOS
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
```

### Building

There is a matrix of different build variants:

- Type:
  * `Debug` is a debug version with all checks enabled.
  * `Beta` is a manual pre-release build.
  * `Release` is a fully optimized version for stores.

- Flavor:
  * `Web` is a light apk without any bundled maps.
  * `Google` is a full store version without low-zoom overview world map.
  * There are also `Amazon`, `Samsung`, `Xiaomi`, `Yandex`
        and other targets.

To run a debug version on your device/emulator:

```bash
(cd android; ./gradlew clean runWebDebug)
```

To compile a redistributable `.apk` for testing:

```bash
(cd android; ./gradlew clean assembleWebBeta)
ls -la ./android/build/outputs/apk/android-web-beta-*.apk
```

### Debugging

To enable logging in case of crashes, after installing a debug version, run:

```bash
adb shell pm grant app.omaps.debug android.permission.READ_LOGS
```

## iOS app

### Preparing

Building OMaps for iOS requires a Mac.

Ensure that you have at least 20GB of free space.

Install Command Line Tools:

 - Launch Terminal application on your Mac.
 - Type `git` in the command line.
 - Follow the instructions in GUI.

Install [Xcode](https://apps.apple.com/ru/app/xcode/id497799835?mt=12) from AppStore.

Enroll in the [Apple Developer Program](https://developer.apple.com/programs/).

### Getting sources

Clone the repository:

```bash
git clone --recursive https://github.com/organicmaps/organicmaps.git
```

Update git submodules (sometimes doesn't work automatically):

```
git submodule update --init --recursive
```

Configure the repository as opensource build:

```bash
./configure.sh
```

or with private repository

```bash
./configure.sh <private-repo-name>
```

Install [CocoaPods](https://cocoapods.org/):

```bash
brew install cocoapods
```

Install required pods for the project:

```bash
(cd iphone/Maps && pod install)
```

### Configuring Xcode

Set up your developer account and add certificates:

 - Run Xcode.
 - Click "Xcode" → "Preferences".
 - Open "Account" tab.
 - Enter account credentials from the previous step.
 - Click "Manage Certificates".
 - Click "+" and choose "Apple Development".
 - You may also need to register your Mac in your Apple Developer account.

Reconfigure the project to use your developer signing keys:

- Open `xcode/omim.xcworkspace` in Xcode.
- Click on "Maps" project.
- Open "Signing & Capabilities" tab.
- Choose your team and your signing certificate.

### Building and running

Open `xcode/omim.xcworkspace` in XCode.

Select "OMaps" product scheme.

 * Choose "Your Mac (Designed for iPad)" to run on Mac without using Simulator.
 * Choose arbitrary "iPhone *" or "iPad *" to run on Simulator.

Compile and run the project ("Product" → "Run").

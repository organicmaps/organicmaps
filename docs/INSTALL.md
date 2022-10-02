# Building

- [System requirements](#system-requirements)
- [Getting sources](#getting-sources)
- [Desktop](#desktop-app)
- [Android](#android-app)
- [iOS](#ios-app)

## System requirements

To build and run Organic Maps you'll need a machine with at least 4Gb of RAM and 20-30Gb of disk space depending on your target platform. Expect to download 5-10Gb of files.

For _Windows_ you need to have [Git for Windows](https://git-scm.com/download/win) installed and Git bash available in the PATH.

## Getting sources

First of all get the source code. The full Organic Maps sources repository is ~8.5Gb in size, there are various [clone options](#special-cases-options) to reduce download size to suit your needs.

For _Windows 10_ enable [symlinks](https://git-scm.com/docs/git-config#Documentation/git-config.txt-coresymlinks) support in git first:

```bash
git config --global core.symlinks true
```

Clone the repository including all submodules (see [Special cases options](#special-cases-options) below):

(if you plan to contribute and propose pull requests then use a web interface at https://github.com/organicmaps/organicmaps to fork the repo first and use your fork's url in the command below)

```bash
git clone --recurse-submodules https://github.com/organicmaps/organicmaps.git
```

Go into the cloned repo:
```bash
cd organicmaps
```

Configure the repository for an opensource build:

(if you plan to publish the app privately in stores check [special options](#special-cases-options))

```bash
bash ./configure.sh
```

For _Windows 10_: Use WSL to run `./configure.sh`, or, alternatively, run the following command from the
[Visual Studio Developer Command Prompt](https://docs.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022)
and follow instructions:

```bash
bash ./configure.sh # execute the script by using Ubuntu WSL VM
```

### Special cases options

If all you want is just a one-off build or your Internet bandwidth or disk space are limited then add following options to the `git clone` command:

- a `--filter=blob:limit=128k` option to make a _partial clone_ (saves ~4Gb), i.e. blob files over 128k in size will be excluded from the history and downloaded on-demand - should be suitable for a generic development use;

- a `--depth=1` option to make a _shallow copy_ (and possibly a `--no-single-branch` to have all branches not just `master`), i.e. omit history while retaining current commits only (saves ~4.5Gb) - suitable for one-off builds;

- a `--shallow-submodules` option to _shallow clone_ the submodules (save ~1.3Gb) - should be suitable for a generic development if no work on submodules is planned.

To be able to publish the app in stores e.g. in Google Play its necessary to populate some configs with private keys, etc.
Check `./configure.sh --help` to see how to copy the configs automatically from a private repo.

## Desktop app

### Preparing

You need a Linux or a Mac machine to build a desktop version of Organic Maps.

- We haven't compiled Organic Maps on Windows in a long time, though it is possible.
  It is likely some make files should be updated.
  If you succeed, please submit a tutorial.
  You'll need to have python3, cmake, ninja in the PATH and also to have Qt5 installed.

Ensure that you have at least 20GB of free space.

Install Cmake (**3.22.1** minimum), Boost, Qt 5 and other dependencies.

Installing *ccache* can speed up active development.

_Ubuntu 20.04, 22.04:_

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    clang \
    ninja-build \
    python3 \
    qtbase5-dev \
    libc++-dev \
    libfreetype-dev \
    libglu1-mesa-dev \
    libicu-dev \
    libqt5svg5-dev \
    libsqlite3-dev \
    zlib1g-dev
```

For Ubuntu 20.04 the version of `cmake` that ships with Ubuntu is too old; a more recent version can be installed using `snap`:

```bash
sudo snap install --classic cmake
```

For Ubuntu 22.04 `cmake` may be installed using `snap` as well, or alternatively by using `apt`:

```bash
sudo apt install -y cmake
```

_Fedora:_

```bash
sudo dnf install -y \
    clang \
    cmake \
    ninja-build \
    freetype-devel \
    libicu-devel \
    libstdc++-devel \
    qt5-qtbase-devel \
    qt5-qtsvg-devel \
    sqlite-devel
```

_macOS:_

```bash
brew install cmake ninja qt@5
```

### Building

To build a desktop app:
```bash
tools/unix/build_omim.sh -r desktop
```

The output binary will go into `../omim-build-release`.

Check `tools/unix/build_omim.sh -h` for more build options, e.g. to build a debug version.

Besides _desktop_ there are other targets like _generator_tool_, to see a full list execute:

```bash
tools/unix/build_omim.sh -d help
```

### Running

The generated binaries appear in `../omim-build-<buildtype>`.

A desktop app binary is `OMaps`. To run e.g. a release version:

_Linux:_

```bash
../omim-build-release/OMaps
```

_macOS:_

```bash
../omim-build-release/OMaps.app/Contents/MacOS/OMaps
```

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

### More options

To make the desktop app display maps in a different language add a `-lang` option, e.g. for russian language:
```bash
../omim-build-release/OMaps -lang ru
```

By default `OMaps` expects a repository's `data` folder to be present in the current working dir, add a `-data_path` option to override it.

Check `OMaps -help` for a list of all run-time options.

When running the desktop app with a lot of maps increase open files limit, which is only 256 on Mac OS X.
Use `ulimit -n 2000`, put it into `~/.bash_profile` to apply it to all new sessions.
In OS X to increase this limit globally, add `limit maxfiles 2048 2048` to `/etc/launchd.conf`
and run
```bash
echo 'ulimit -n 2048' | sudo tee -a /etc/profile
```

If you have Qt installed in an unusual directory, use `QT_PATH` variable (`SET (QT_PATH "your-path-to-qt")`). You can skip building tests
with `CMAKE_CONFIG=-DSKIP_TESTS` variable. You would need 1.5 GB of memory
to compile the `stats` module.

The `build_omim.sh` script basically runs these commands:
```bash
    cmake <path_to_omim> -DCMAKE_BUILD_TYPE={Debug|Release}
    <make or ninja> [<target>] -j <number_of_processes>
```

## Android app

### Preparing

Linux, Mac, or Windows should work to build Organic Maps for Android.

Ensure that you have at least 30GB of free space.

Install [Android Studio](https://developer.android.com/studio).

Install Android SDK and NDK:

- Run the Android Studio.
- Open "SDK Manager" (under "More Actions" in a welcome screen or a three-dot menu in a list of recent projects screen or "Tools" top menu item in an open project).
- Select "Android 13.0 (T) / API Level 33" SDK.
- Switch to "SDK Tools" tab.
- Check "Show Package Details" checkbox.
- Select "NDK (Side by side)" version **25.1.8937393**.
- Select "CMake" version **3.22.1**.
- Click "Apply" and wait for downloads and installation to finish.
- In the left pane menu select "Appearance & Behavior > System Settings > Memory Settings".
- Set "IDE max heap size" to 2048Mb or more (otherwise the Studio might get stuck on "Updating indexes" when opening the project).

Configure the repo with Android SDK and NDK paths:

_Linux:_
```bash
./tools/android/set_up_android.py --sdk $HOME/Android/Sdk
```

_macOS:_
```bash
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
```

_Windows 10:_ no action needed, should work out of the box.

In Android Studio open the project in `android/` directory.

Setup a virtual device to use [emulator](https://developer.android.com/studio/run/emulator) ("Tools > Device Manager") or [use a hardware device for debugging](https://developer.android.com/studio/run/device). If using an emulator, make sure to choose a system image with API Level 31 and ABI _x86_64_.

### Building

There is a matrix of different build variants:

- _Type_:
  - `Debug` is a debug version with all checks enabled.
  - `Beta` is a manual pre-release build for testing.
  - `Release` is a fully optimized version for app stores.

- _Flavor_:
  - `Web` is a light apk without any bundled maps.
  - `Google` is a full Google Play store version including a low-zoom overview world map.
  - `Fdroid` is a version for publishing on [F-Droid](https://f-droid.org/) open source apps store (no bundled maps and no Google services).
  - ...and other flavors like `Huawei`.

You can choose a build variant in Android Studio's "Build > Select Build Variant..." menu. There you can also choose a target architecture (Active ABI) like _x86_64_ (for e.g. emulator) or _arm64-v8a_ (many modern devices).

To build and run the app in the emulator or on a hardware device use a "Run > Run (android)" menu item.

To build a redistributable apk use a "Build > Build Bundle(s) / APK(s) > Build APK(s)" menu item. Generated apks are stored in `build/outputs/apk/`.

See also https://developer.android.com/studio/run.


### Debugging

To enable logging in case of crashes, after installing a debug version, run:

```bash
adb shell pm grant app.organicmaps.debug android.permission.READ_LOGS
```

### More options

#### Building from the command line

First configure `PATH` to prefer `cmake` from Android SDK/NDK instead of one installed in the system:

_Linux:_

```bash
export PATH=$HOME/Android/Sdk/cmake/3.22.1/bin:$PATH
```

_macOS:_

```bash
export PATH=$HOME/Library/Android/Sdk/cmake/3.22.1/bin:$PATH
```

Check if you have a system-wide Java runtime environment (JRE) installed:

```bash
java -version
```

If your system doesn't have a JRE installed or Java version is less than 11 (openjdk)
or you want command line builds to use a JRE version bundled with the Studio
then set `JAVA_HOME` environment variable:

_Linux:_

```bash
export JAVA_HOME=<path-to-android-studio-installation>/android-studio/jre
```

_macOS:_

```bash
export JAVA_HOME=<path-to-android-studio-installation>/Contents/jre/Contents/Home
```

Run the builds from the android subdirectory of the repo:
```bash
cd android
```

To build, install and run e.g. a _Web Debug_ version on your device/emulator:

```bash
./gradlew runWebDebug
```

Or to compile a redistributable _Fdroid Beta_ apk for testing:

```bash
./gradlew assembleFdroidBeta
```

Or to build _Release_ apks for all _Flavors_:

```bash
./gradlew assembleRelease
```

Run `./gradlew tasks` to see all possible build variants.

Intermediate files for each build (_Type_ + _Flavor_ + target arch) take ~3-4.5Gb of space.
To remove all intermediate build files run `./gradlew clean`.

By default the build will run for all 3 target architectures: _arm64-v8a_, _armeabi-v7a_ and _x86_64_. To speed up your build include only the arch you need by adding e.g. a `-Parm64` option to the gradle build command (other options are `-Parm32` for _armeabi-v7a_, `-Px64` for _x86_64_ and `-Px86` for _x86_).

To create separate apks for all target arches add a `-PsplitApk` option (by default all arches are combined in one "fat" apk).

Adding a `-Ppch` (use precompiled headers) option makes builds ~15% faster.

If a running build makes your computer slow and laggish then try lowering the priority of the build process by adding a `--priority=low` option and/or add a `-Pnjobs=<N>` option to limit number of parallel processes.

See also https://developer.android.com/studio/build/building-cmdline.

To add any of those options to in-studio builds list them in "Command-line Options" in "File > Settings... > Build, Execution, Deployment > Compiler"

#### Reduce resource usage

You can install
[Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html) without
Android Studio. Please make sure that SDK for API Level 33,
NDK version **25.1.8937393** and CMake version **3.22.1** are installed.

If you are low on RAM, disk space or traffic there are ways to reduce system requirements:
- in Android Studio enable "File > Power Save Mode";
- don't install Android Studio, run builds and emulator from command line;
- build only for target arches you actually need, e.g. `arm64`;
- for debugging use an older emulated device with low RAM and screen resolution, e.g. "Nexus S";
- make sure the emulator uses [hardware acceleration](https://developer.android.com/studio/run/emulator-acceleration);
- don't use emulator, debug on a hardware device instead.

#### Windows 10: Manual Boost library initialization

1. Install Visual Studio 2019 Community Edition.
2. Add cl.exe to your PATH (`C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat`).
3. Run `./configure.sh` from _Git (for Window) Bash_ and ignore all errors related to Boost.
4. Go to `./3party/boost`, run `./bootstrap.bat`, and then `b2 headers` to configure Boost.


## iOS app

### Preparing

Building Organic Maps for iOS requires a Mac.

Ensure that you have at least 20GB of free space.

Install Command Line Tools:

- Launch Terminal application on your Mac.
- Type `git` in the command line.
- Follow the instructions in GUI.

Install [Xcode](https://apps.apple.com/ru/app/xcode/id497799835?mt=12) from AppStore.

Enroll in the [Apple Developer Program](https://developer.apple.com/programs/) (you can run Organic Maps in Simulator without this step).

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

- Choose "Your Mac (Designed for iPad)" to run on Mac without using Simulator.
- Choose arbitrary "iPhone _" or "iPad _" to run on Simulator.

Compile and run the project ("Product" → "Run").

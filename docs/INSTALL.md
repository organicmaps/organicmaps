# Building

- [Getting sources](#getting-sources)
- [Desktop](#desktop-app)
- [Android](#android-app)
- [iOS](#ios-app)

## Getting sources

First of all get the source code. The full Organic Maps sources repository is ~8.5Gb in size, however its possible to reduce the size e.g. if full commit history is not required.

For _Windows 10_ enable [symlinks][git-symlinks] support in git first:

[git-symlinks]: https://git-scm.com/docs/git-config#Documentation/git-config.txt-coresymlinks

```bash
git config --global core.symlinks true
```

If you plan to contribute and propose pull requests then fork the repo https://github.com/organicmaps/organicmaps first.

Clone the main repository (~5.5Gb):

```bash
git clone https://github.com/organicmaps/organicmaps.git
```
or your fork of it:
```bash
git clone https://github.com/<your-github-name>/organicmaps.git
```

Add a `--filter=blob:limit=128k` option to above command to make a _partial clone_ of just ~1.2Gb, i.e. blob files over 128k in size will be excluded form the history and downloaded on-demand - should be suitable for a generic development use.

And/or add a `--depth=1` option to make a _shallow copy_ (and possibly a `--no-single-branch` to have all branches not just `master`), i.e. omit history retaining current commits only - suitable for one-off builds.

Go into the cloned repo:
```bash
cd organicmaps
```

Clone git submodules (~3Gb):

```bash
git submodule update --init --recursive
```

Add a `--depth=1` option to make a _shallow copy_ of just ~1.3Gb - should be suitable for a generic development if no work on submodules planned.

Configure the repository:

```bash
./configure.sh
```

**Windows 10:** Use WSL to run `./configure.sh`:

```bash
bash ./configure.sh # execute the script by using Ubuntu WSL VM
```

By default the repo is configured for an opensource build. Configure for a private build if private keys and configs are needed to publish the app in stores e.g. in Google Play (check options via `./configure.sh --help`):

```bash
./configure.sh <private-repo-name>
```

## Desktop app

### Preparing

You need a Linux or a Mac machine to build a desktop version of Organic Maps.

- We haven't compiled Organic Maps on Windows in a long time, though it is possible.
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
brew install cmake qt@5
```

### Building

With all tools installed, just run `tools/unix/build_omim.sh`.
It will build both debug and release versions to `../omim-build-<buildtype>`.
Command-line switches are:

- `-r` to build a release version
- `-d` to build a debug version
- `-c` to delete target directories before building
- `-s` to not build a desktop app, when you don't have desktop Qt libraries.
- `-p` with a path to where the binaries will be built.

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

Linux, Mac, or Windows should work to build Organic Maps for Android.

Ensure that you have at least 30GB of free space.

Install [Android Studio](https://developer.android.com/studio).

Install [Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html):

- Run the Android Studio.
- Open "SDK Manager" (under "More Actions" in a welcome screen or a three-dot menu in a list of recent projects screen or "Tools" top menu item in an open project).
- Select "Android 11.0 (R) / API Level 30" SDK.
- Switch to "SDK Tools" tab
- Check "Show Package Details" checkbox.
- Select "NDK (Side by side)" version **21.4.7075529**.
- Select "CMake" version **3.18.1**.
- Click "OK" and wait for downloads and installation to finish.

Alternatively, you can install only
[Android SDK](https://developer.android.com/sdk/index.html) and
[NDK](https://developer.android.com/tools/sdk/ndk/index.html) without
installing Android Studio. Please make sure that SDK for API Level 30,
NDK version **21.X.Y** and CMake version **3.18.XX** are installed.

Configure `PATH` to prefer `cmake` from Android SDK/NDK instead of
installed in the system:

**Linux**:

```bash
export PATH=$HOME/Android/Sdk/cmake/3.18.1/bin:$PATH
```

**macOS**:

```bash
export PATH=$HOME/Library/Android/Sdk/cmake/3.18.1/bin:$PATH
```

Check if you have a system-wide Java runtime environment (JRE) installed:

```bash
java -version
```

If your system doesn't have a JRE installed or Java version is less than 11 (openjdk) or you want command line builds to use a JRE version bundled with the Studio then set `JAVA_HOME` environment variable:

**Linux**:

```bash
export JAVA_HOME=*A-PATH-TO-YOUR-ANDROID-STUDIO-INSTALLATION*/android-studio/jre
```

**macOS**:

```bash
export JAVA_HOME=*A-PATH-TO-YOUR-ANDROID-STUDIO-INSTALLATION*/Contents/jre/Contents/Home
```

**Windows 10.** Alternative way is to initialize Boost manually:

1. Install Visual Studio 2019 Community Edition.
2. Add cl.exe to your PATH (`C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat`).
3. Run `./configure.sh` from _Git (for Window) Bash_ and ignore all errors related to Boost.
4. Go to `./3party/boost`, run `./bootstrap.bat`, and then `b2 headers` to configure Boost.

Set Android SDK and NDK path:

```bash
# Linux
./tools/android/set_up_android.py --sdk $HOME/Android/Sdk
# MacOS
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
# Windows 10
# no actions needed, should work out of the box
```

### Building

There is a matrix of different build variants:

- Type:

  - `Debug` is a debug version with all checks enabled.
  - `Beta` is a manual pre-release build.
  - `Release` is a fully optimized version for stores.

- Flavor:
  - `Web` is a light apk without any bundled maps.
  - `Google` is a full store version without low-zoom overview world map.
  - There are also `Amazon`, `Samsung`, `Xiaomi`, `Yandex`
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
adb shell pm grant app.organicmaps.debug android.permission.READ_LOGS
```

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

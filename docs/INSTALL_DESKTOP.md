## Desktop app

### Preparing

You need a Linux or a MacOS machine to build a desktop version of CoMaps. [Windows](#windows) users can use the [WSL](https://learn.microsoft.com/en-us/windows/wsl/) (Windows Subsystem for Linux) and follow ["Linux or Mac"](#linux-or-mac) steps described below.

### Linux or MacOS

Ensure that you have at least 20GB of free space.

Install Cmake (**3.22.1** minimum), Boost, Qt 6 and other dependencies.

Installing *ccache* can speed up active development.

#### Ubuntu

##### Fully supported versions

_Ubuntu 24.04 or newer:_

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    clang \
    cmake \
    ninja-build \
    python3 \
    qt6-base-dev \
    qt6-positioning-dev \
    libc++-dev \
    libfreetype-dev \
    libglvnd-dev \
    libgl1-mesa-dev \
    libharfbuzz-dev \
    libicu-dev \
    libqt6svg6-dev \
    libqt6positioning6-plugins \
    libqt6positioning6 \
    libsqlite3-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    zlib1g-dev
```

##### Workarounds for older Ubuntu versions

| Software  | Minimum version | Impacted Ubuntu release | Workaround                                                  |
| --------- | --------------- | ----------------------- | ----------------------------------------------------------- |
| CMake     | `3.22.1`        | `20.04` and older       | Install newer `cmake` from [PPA](https://apt.kitware.com/) or from `snap`<br> with `sudo snap install --classic cmake` |
| FreeType  | `2.13.1`        | `22.04` and older       | Install newer `libfreetype6` and `libfreetype-dev` from [PPA](https://launchpad.net/~reviczky/+archive/ubuntu/freetype) |
| GeoClue   | `2.5.7`         | `20.04` and older       | Install newer `geoclue-2.0` from [PPA](https://launchpad.net/~savoury1/+archive/ubuntu/backports) |
| Qt 6      | `6.4.0`         | `22.04` and older       | Build and install Qt 6.4 manually |


```bash
sudo add-apt-repository -y ppa:savoury1/qt-6-2
```

#### Linux Mint

Check which Ubuntu version is the `PACKAGE BASE` for your Linux Mint release [here](https://www.linuxmint.com/download_all.php),
and apply the [Ubuntu workarounds accordingly](#workarounds-for-older-ubuntu-versions).

#### Fedora

```bash
sudo dnf install -y \
    clang \
    cmake \
    ninja-build \
    freetype-devel \
    libicu-devel \
    libstdc++-devel \
    mesa-libGL-devel \
    libglvnd-devel \
    qt6-qtbase-devel \
    qt6-qtpositioning \
    qt6-qtpositioning-devel \
    qt6-qtsvg-devel \
    sqlite-devel
```

#### Alpine

```bash
sudo apk add \
    cmake \
    freetype-dev \
    g++ \
    icu-dev \
    mesa-gl \
    ninja-build \
    qt6-qtbase-dev \
    qt6-qtpositioning-dev \
    qt6-qtsvg-dev \
    samurai \
    sqlite-dev
```

#### macOS

```bash
brew install cmake ninja qt@6
```

### Windows

We haven't compiled CoMaps on Windows *natively* in a long time, somes adaptations is required to support Windows.
You'll need to have python3, cmake, ninja, and QT6 in the PATH, and Visual Studio 2022 or Visual Studio 2022 Build Tools installed. Use [Visual Studio Developer Command Prompt](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022) or generate Visual Studio project files with CMake to build the project.

However, it is possible to use the WSL (Windows Subsystem for Linux) to run GUI applications.

#### Windows 11 (WSL)

To run Linux GUI apps, you'll need to [install a driver](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps) matching your system. This enables a virtual GPU allowing hardware-accelerated OpenGL rendering.
- [Intel GPU Driver](https://www.intel.com/content/www/us/en/download/19344/intel-graphics-windows-dch-drivers.html)
- [AMD GPU Driver](https://www.amd.com/en/support)
- [NVIDIA GPU Driver](https://www.nvidia.com/Download/index.aspx?lang=en-us)

Once a GPU driver is installed and you have [built the app](#building-1) you should be able to [run](#running) it without any additional steps.

#### Windows 10 (WSL)

For Windows 10 you should do these steps (taken from [here](https://techcommunity.microsoft.com/t5/windows-dev-appconsult/running-wsl-gui-apps-on-windows-10/ba-p/1493242), check this blog post if you have any problems):
1. Download and install [VcXsrv Windows X Server](https://sourceforge.net/projects/vcxsrv/).
2. Run _XLaunch_ app to launch X Server. During settings make sure "Disable access control" checkbox is selected.
3. (optionally) Click "Save configuration" and save configuration to some file (for example to _config.xlaunch_). With this you will be able to quickly run the desktop app in the future.
4. When asked about firewall, allow access for both public and private networks.
5. Add this line:
    ```bash
    export DISPLAY=$(ip route|awk '/^default/{print $3}'):0.0
    ```
    to _/etc/bash.bashrc_ file.
6. Restart WSL.

Now when you want to run the desktop app you just need to first launch the X Server on Windows (for example, by running previously saved _config.xlaunch_ file) and then you should be able to [build](#building-1) and [run](#running) the app from WSL.

Running X Server is also required to run `generate_symbols.sh` script when you change icons for [styles](STYLES.md)


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

#### Build issues

- If you get "not enough memory" errors during builds, you may disable
  [CMake Unity Builds](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html) with `export UNITY_DISABLE=1`
  or by passing `-DUNITY_DISABLE=1` option to `cmake` invocation. Or you can reduce Unity build batch size from
  the default `50` to a lower value (`2`-`16`) with `export UNITY_BUILD_BATCH_SIZE=8`.
  Note that these changes may significantly increase the build time.

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
ctest -L "omim-test" --output-on-failure
```

To run a limited set of tests, use `-R <regex>` flag. To exclude some tests, use `-E <regex>` flag:

```bash
cd build
ctest -R "base_tests|coding_tests" --output-on-failure
ctest -L "omim-test" -E "base_tests|coding_tests" --output-on-failure
```

Some tests are known to be broken and disabled on CI.

### Test Coverage

To generate a test coverage report you'll need [gcovr](https://gcovr.com) and gcov tools installed.

Installing gcovr on Linux:
```bash
pip3 install gcovr
```

Installing gcovr on MacOS:
```bash
brew install gcovr
```

Installing gcov on Linux:
```bash
# If you're using GCC compiler
sudo apt-get install cpp

# If you're using Clang compiler
sudo apt-get install llvm
```

Installing gcov on MacOS:
```bash
# If you're using AppleClang compiler it should already be installed

# If you're using Clang compiler
brew install llvm
```

Steps to generate coverage report:

1. Configure cmake with `-DCOVERAGE_REPORT=ON` flag:
   ```bash
   cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
           -DCMAKE_CXX_FLAGS=-g1 -DCOVERAGE_REPORT=ON
   ```
2. Compile unit tests.
3. Run unit tests.
4. Run coverage report generation:
   ```bash
   cd build
   cmake --build . --target omim_coverage
   ```
5. Report can be found in the `build/coverage_report` folder.

### Debug commands

CoMaps has some "hidden" debug commands that you can trigger by entering them into the search box.

For example you can switch theme which is very useful for checking [styles](STYLES.md) changes.

There are also other commands for turning on/off isolines, anti-aliasing, etc. Check [DEBUG_COMMANDS.md](DEBUG_COMMANDS.md) to learn about them.

### More options

To make the desktop app display maps in a different language add a `-lang` option, e.g. for the Russian language:

```bash
../omim-build-release/OMaps -lang ru
```

By default `OMaps` expects a repository's `data` folder to be present in the current working directory, add a `-data_path` option to override it.

Check `OMaps -help` for a list of all run-time options.

When running the desktop app with lots of maps, increase the open files limit. In MacOS the default value is only 256.
Use `ulimit -n 2000`, put it into `~/.bash_profile` to apply it to all new sessions.
In MacOS to increase this limit globally, add `limit maxfiles 2048 2048` to `/etc/launchd.conf`
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

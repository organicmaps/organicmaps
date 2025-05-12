# Building

- [System requirements](#system-requirements)
- [Getting sources](#getting-sources)
- [Android](#android-app)
- [iOS](#ios-app)
- [Desktop](#desktop-app)
- [Map data and styles](#map-data-and-styles)

## System requirements

To build and run CoMaps you'll need a machine with at least 4Gb of RAM and 20-30Gb of disk space depending on your target platform. Expect to download 2-5Gb of files.

For _Windows_ you need to have [Git for Windows](https://git-scm.com/download/win) installed and Git bash available in the PATH.

## Getting sources

First of all get the source code. The full CoMaps sources repository is ~3Gb in size, there are various [clone options](#special-cases-options) to reduce the download size to suit your needs.

For _Windows_, it's necessary to enable symlink support:
1. Activate _Windows Development Mode_ to enable symlinks globally:
  - Windows 10: _Settings_ -> _Update and Security_ -> _For Developers_ -> _Activate Developer Mode_
  - Windows 11: _Settings_ -> _Privacy and Security_ -> _For Developers_ -> _Activate Developer Mode_
  - Press Win + R, run `ms-settings:developers` and _Activate Developer Mode_
2. Enable [symlinks](https://git-scm.com/docs/git-config#Documentation/git-config.txt-coresymlinks) support in git. The easiest way is to reinstall the latest [Git for Windows](https://git-scm.com/download/win) with the "Enable Symlinks" checkbox checked. If you don't want to reinstall Git, then you can add `-c core.symlinks=true` parameter to the clone command below to enable symlinks for the repository.

```bash
git config --global core.symlinks true
```

Clone the repository including all submodules (see [Special cases options](#special-cases-options) below):

(if you plan to contribute and propose pull requests then use a web interface at https://codeberg.org/comaps/comaps to fork the repository first and use your fork's URL in the command below)

```bash
git clone --recurse-submodules --shallow-submodules https://codeberg.org/comaps/comaps.git
```

Go into the cloned repository:
```bash
cd comaps
```

Configure the repository (make sure you have a working C++ build environment):

(if you plan to publish the app privately in stores check [special options](#special-cases-options))

```bash
bash ./configure.sh
```

For _Windows 10_:  You should be able to build the project by following either of these setup methods:

**Setup 1: Using WSL**
1. Install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) on your machine.
2. Install g++ by running the following command in WSL: `sudo apt install g++`
3. Run `./configure.sh` in WSL.

**Setup 2: Using Visual Studio Developer Command Prompt**
1. Install the [Visual Studio Developer Command Prompt](https://docs.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022) (make sure to choose the latest MSVC x64/x86 build tool and Windows 10/11 SDK as individual components while installing Visual Studio).
2. Run the following command and follow instructions:

```bash
"C:\Program Files\Git\bin\bash.exe" configure.sh # execute the script by using Developer Command Prompt
```

Download the latest `World.mwm` and `WorldCoast.mwm` files and put them into the `data/` dir.

Run the skins/textures generation script:
```bash
bash ./tools/unix/generate_symbols.sh
```

Now the repository is prepared to build a CoMaps app!

### Special cases options

If you're only doing a one-off build or your internet bandwidth or disk space is limited, add following options to the `git clone` command:

- a `--filter=blob:limit=128k` option to make a _partial clone_ (saves ~4Gb), i.e. blob files over 128k in size will be excluded from the history and downloaded on-demand - is suitable for generic development.

If you mistakenly did a `git clone` without checking out submodules, you can run `git submodule update --init --recursive --depth 1`.

To be able to publish the app in stores e.g. in Google Play its necessary to populate some configs with private keys, etc.

If you need Organic Maps and Maps.ME commits history (before the CoMaps fork) run:
```bash
git remote add om-historic https://codeberg.org/comaps/om-historic.git
git fetch --tags om-historic
git replace squashed-history historic-commits
```
It'll seamlessly replace the squashed first "Organic Maps sources as of 02.04.2025" commit with all prior commits which will work with all git commands as usual.
The `om-historic.git` repo is ~1Gb only as various historic blobs, bundled 3rd-party deps, etc. were removed from it.
If you really need them (e.g. to build a very old app version) then refer to full organicmaps.git repo please.

## Android app

### Preparing

Linux, MacOS, or Windows should work to build CoMaps for Android.

Ensure that you have at least 30GB of free space and Python 3 installed.

Install [Android Studio](https://developer.android.com/studio).

Run Android Studio and open the project in `android/` directory!
This is important, otherwise the following menus won't be visible.

Configure Android Studio:

- Open "SDK Manager" (under "More Actions" in a welcome screen or a three-dot menu in a list of recent projects screen or "Tools" top menu item in an open project).
- Switch to "SDK Tools" tab.
- Check "Show Package Details" checkbox.
- Select "CMake" version **3.22.1** or higher.
- Click "Apply" and wait for downloads and installation to finish.
- In the left pane menu select "Appearance & Behavior > System Settings > Memory Settings".
- Set "IDE max heap size" to 2048MB or more (otherwise the Studio might get stuck on "Updating indexes" when opening the project).

Configure the repository with Android SDK paths. You can do it either by [setting](https://developer.android.com/tools#environment-variables) a global environment variable pointing at your Android SDK:

```
ANDROID_HOME=<here is the absolute path to the root folder of your Android SDK installation>
```

or by running the following script, that creates `android/local.properties` file with the line `sdk.dir=<path to your Android SDK>` in it:

_Linux:_

```bash
./tools/android/set_up_android.py --sdk $HOME/Android/Sdk
```

_macOS:_

```bash
./tools/android/set_up_android.py --sdk $HOME/Library/Android/Sdk
```

_Windows 10:_ no action needed, should work out of the box.


### Set up the emulator

Set up a virtual device to use [emulator](https://developer.android.com/studio/run/emulator) ("Tools > Device Manager") or [use a hardware device for debugging](https://developer.android.com/studio/run/device).
For the emulator its recommended to choose the latest supported API Level system image. Use ABI _x86_64_ for Intel-based processors and _arm64-v8a_ for ARM-based processors (e.g. M1/M2 Macs).


### Building

There is a matrix of different build variants:

- _[Type](https://codeberg.org/comaps/comaps/src/commit/d02aefbf12a1a201090b40b395585e679b04c798/android/app/build.gradle#L278)_:
  - `Debug` is a debug version with all checks enabled.
  - `Beta` is a manual pre-release build for testing.
  - `Release` is a fully optimized version for app stores.

- _[Flavor](https://codeberg.org/comaps/comaps/src/commit/d02aefbf12a1a201090b40b395585e679b04c798/android/app/build.gradle#L179)_:
  - `Web` is a light APK without any bundled maps.
  - `Google` is a full Google Play store version including a low-zoom overview world map.
  - `Fdroid` is a version for publishing on the [F-Droid](https://f-droid.org/) open source apps store (no bundled maps;  FOSS microG services instead of Google's).
  - ...and other flavors like `Huawei`.

You can choose a build variant in Android Studio's "Build > Select Build Variant..." menu. There you can also choose a target architecture (Active ABI) like _x86_64_ (for e.g. emulator) or _arm64-v8a_ (many modern devices).
In order to build the Google variant, you need a special key which only the core developers have. For community members who want to contribute, the best selection is "fdroidBeta" or "fdroidDebug" depending on the use case.
The Active ABI can be set to "arm64-v8a".

To build and run the app in the emulator or on a hardware device use a "Run > Run (android)" menu item or press the Play / Debug button on the top right of the IDE.

To build a redistributable APK use a "Build > Build Bundle(s) / APK(s) > Build APK(s)" menu item. Generated APKs are stored in `build/outputs/apk/`.

See also https://developer.android.com/studio/run.


### Debugging

To enable logging in case of crashes, after installing a debug version, run:

```bash
adb shell pm grant app.organicmaps.debug android.permission.READ_LOGS
```


### Android Auto Development

Android Auto can be developed and tested without having a physical device by using [Desktop Head Unit (DHU)](https://developer.android.com/training/cars/testing/dhu). Go to Android Studio > Tools -> SDK Manager -> SDK Tools and enable "Android Auto Desktop Head Unit".

[Android Auto App](https://play.google.com/store/apps/details?id=com.google.android.projection.gearhead) is required for Auto functionality. The app should be installed from Google Play before connecting a phone to the Desktop Head Unit or a real car. Android Auto doesn't work on phones without Google Play Services.

To run Android Auto, connect the phone using USB cable and run the Desktop Head Unit with `--usb` flag:

```
~/Library/Android/sdk/extras/google/auto/desktop-head-unit --usb
```

```
[REDACTED]
[I]: Found device 'SAMSUNG SAMSUNG_Android XXXXXXXXX' in accessory mode (vid=18d1, pid=2d01).
[I]: Found accessory: ifnum: 0, rd_ep: 129, wr_ep: 1
[I]: Attaching to USB device...
[I]: Attached!
```

CoMaps icon will appear in the application list in DHU.

### More options

#### Building from the command line

First configure `PATH` to prefer `cmake` from the Android SDK instead of the default system install:

_Linux:_

```bash
export PATH=$HOME/Android/Sdk/cmake/3.22.1/bin:$PATH
```

_macOS:_

```bash
export PATH=$HOME/Library/Android/Sdk/cmake/3.22.1/bin:$PATH
```

Check if you have a system-wide Java Runtime Environment (JRE) installed:

```bash
java -version
```

If your system doesn't have a JRE installed or Java version is less than 17 (OpenJDK)
or you want command line builds to use a JRE version bundled with the Studio
then set the `JAVA_HOME` environment variable:

_Linux:_

```bash
export JAVA_HOME=<path-to-android-studio-installation>/android-studio/jre
```

_macOS:_

```bash
export JAVA_HOME=<path-to-android-studio-installation>/Contents/jre/Contents/Home
```

Run the builds from the android subdirectory of the repository:

```bash
cd android
```

To build, install and run e.g. a _Web Debug_ version on your device/emulator:

```bash
./gradlew runWebDebug
```

Or to compile a redistributable _Fdroid Beta_ APK for testing:

```bash
./gradlew assembleFdroidBeta
```

Or to build _Beta_ APKs for all _Flavors_:

```bash
./gradlew assembleBeta
```

Run `./gradlew tasks` to see all possible build variants.

Intermediate files for each build (_Type_ + _Flavor_ + target arch) take ~3-4.5Gb of space.
To remove all intermediate build files run `./gradlew clean`.

By default the build will run for all 3 target architectures: _arm64-v8a_, _armeabi-v7a_ and _x86_64_. To speed up your build include only the arch you need by adding e.g. a `-Parm64` option to the gradle build command (other options are `-Parm32` for _armeabi-v7a_, `-Px64` for _x86_64_ and `-Px86` for _x86_).

To create separate APKs for all target arches add a `-PsplitAPK` option (by default all arches are combined in one "fat" APK).

Adding a `-Ppch` (use precompiled headers) option makes builds ~15% faster.

If building makes your computer slow and laggy, then try lowering the priority of the build process by adding a `--priority=low` option and/or add a `-Pnjobs=<N>` option to limit the number of parallel processes.

See also https://developer.android.com/studio/build/building-cmdline.

To add any of those options to in-studio builds list them in "Command-line Options" in "File > Settings... > Build, Execution, Deployment > Compiler"

#### Reduce resource usage

If you are low on RAM, disk space or traffic there are ways to reduce system requirements:
- exclude the `cpp` folder from indexing - if you do not make any work on the C++ code, this will greatly improve the start-up performance and the ram usage of Android Studio; Click on the `Project` tab on the left, find the `cpp` folder (should be next to the `java` folder), right click on it and select `Mark Directory as` -> `Excluded` (red folder icon), then restart Android Studio;
- in Android Studio enable "File > Power Save Mode";
- disable the "Android NDK Support" plugin in "Settings -> Plugins" completely and use another IDE (Visual Studio Code, Qt Creator, etc.) for editing C++ code instead;
- don't install Android Studio, run builds and emulator from command line (download [command line tools](https://developer.android.com/studio#command-line-tools-only) first and then use the [`sdkmanager`](https://developer.android.com/tools/sdkmanager) tool to install all required packages: `platform-tools`, `build-tools`, a [right version](#preparing-1) of `cmake`, maybe `emulator`...; then [set env vars](https://developer.android.com/tools#environment-variables));
- build only for target arches you actually need, e.g. `arm64`;
- for debugging use an older emulated device with low RAM and screen resolution, e.g. "Nexus S";
- make sure the emulator uses [hardware acceleration](https://developer.android.com/studio/run/emulator-acceleration);
- don't use emulator, debug on a hardware device instead.

#### Alternatives for working with C++ code

Android Studio has issues in parsing the C++ part of the project, please let us know if you know how to resolve it. As a workaround, for working C++ suggestions, you may use:

- [Qt Creator](https://www.qt.io/product/development-tools)
- [Xcode](https://developer.apple.com/xcode/)
- [CLion](https://www.jetbrains.com/clion/)

For Xcode it is required to run `cmake . -g Xcode` to generate project files, while CLion and QT Creator can import CMakeLists.txt.

#### Enable Vulkan Validation

1. Download Vulkan Validation Layers
```bash
./tools/android/download_vulkan_validation_layers.py
```

2. Set `enableVulkanDiagnostics=ON` in `gradle.properties`.

If you build the app from command line, the parameter can be passed via command line.

E.g.
```
./gradlew -Parm64 -PenableVulkanDiagnostics=ON runGoogleDebug
```

#### Enable tracing
1. Set `enableTrace=ON` in `gradle.properties`.
2. Follow the guide https://perfetto.dev/docs/quickstart/android-tracing to set-up Perfetto
Example of command line for running system tracing:
```
./record_android_trace -a app.organicmaps.debug -o trace_file.perfetto-trace -t 30s -b 64mb sched freq idle am wm gfx view binder_driver hal dalvik camera input res memory
```

## iOS app

### Preparing

Building CoMaps for iOS requires a Mac.

Ensure that you have at least 20GB of free space.

After [getting all sources](#getting-sources), please make sure that Command Line Tools are installed:

```bash
xcode-select --install
```

Then, install [Xcode](https://apps.apple.com/app/xcode/id497799835?mt=12) from the App Store.

Enroll in the [Apple Developer Program](https://developer.apple.com/programs/) (you can run CoMaps in Simulator without this step).

### Configuring Xcode

Set up your developer account and add certificates:

- Open Xcode.
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
- Choose a unique bundle identifier (not app.organicmaps.debug) and your team.
- Select "Automatically manage signing".

If you want to run CoMaps on a real device, you have to remove the CarPlay entitlement. Open `iphone/Maps/OMaps-Debug.entitlements`
and remove the `com.apple.developer.carplay-maps` entry. Now you can sign your app again in the "Signing & Capabilities" tab. Testing CarPlay
on a real device requires [requesting entitlements from Apple](https://developer.apple.com/documentation/carplay/requesting_carplay_entitlements).

### Building and running

Open `xcode/omim.xcworkspace` in Xcode.

Select "OMaps" product scheme.

- Choose "Your Mac (Designed for iPad)" to run on Mac without using Simulator.
- Choose either "iPhone _" or "iPad _" to run in the Simulator.

Compile and run the project ("Product" → "Run").

## Desktop app

See [install_desktop](INSTALL_DESKTOP.md) to install and build Desktop app for Linux and Mac OS

## Map data and styles
See readme for the [map generator](MAPS.md) and [styles](STYLES.md) if you need to customize the map files and styles.

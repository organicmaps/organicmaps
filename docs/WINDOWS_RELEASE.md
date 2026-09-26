# Windows desktop release work

This document covers staging the Windows desktop runtime. Installers, signing,
package identities, the VCLibs dependency version, and update feeds are not
covered.

## Build and stage

Configure separate x64 MSVC/Qt build directories for each channel. Use the same
tested Qt installation for the build and `windeployqt`. Example:

```powershell
cmake --preset release -B build/windows-direct `
  -DBUILD_TESTING=OFF -DBUILD_DESIGNER=OFF `
  -DOMIM_WINDOWS_DISTRIBUTION=direct
cmake --build build/windows-direct --target desktop
cmake --install build/windows-direct --prefix dist/windows-direct `
  --component WindowsRuntime
```

Use a fresh staging prefix on each attempt. The install component puts `OrganicMaps.exe`
in the prefix root and immutable resources under `data/`, runs that Qt installation's
`windeployqt --release --no-compiler-runtime --no-translations`, and writes
`build-provenance.json` with file hashes, source commit, Qt version, and the map
catalog version. The resources shared with the macOS bundle are listed in
`qt/runtime_resources.txt`; the Windows stage also includes bundled ICU data.
`licenses/` contains the repository's license and notice files.
The installer must declare the retail `Microsoft.VCLibs.140.00.UWPDesktop`
framework dependency matching the toolchain; staging does not install it.

The Windows Release CI job also uploads `WindowsDirect-unsigned-stage-<run id>`
from this component. Download it from the workflow run's Artifacts section and
extract it on another Windows x64 machine. Run `OrganicMaps.exe` from the extracted
folder; this artifact is a staged folder, not an installer or MSIX. The test
machine needs a compatible Visual C++ runtime already installed because the
MSIX-oriented stage omits compiler runtime files. CI derives a numeric package
version from the source commit date and daily count. Do not distribute this
artifact as a release.

For a direct or Store distribution, Release, bundled third-party libraries,
and `BUILD_DESIGNER=OFF` are required. `tools/unix/version.sh windows_version`
generates `YYYY.MMDD.COUNT.0` for both channels, leaving the Store revision
component at zero. Development builds retain the existing writable resource
behavior.

## User data and channel changes

Direct and Store builds use separate roots below the user's local application
data directory: `OrganicMaps\WindowsDirect` and `OrganicMaps\WindowsStore`.
Settings, downloaded maps, and launch logs go there. Startup cleanup keeps the
current log and nine recent logs; logs held by other instances are retried on
the next launch. The application does not move or
delete older developer data under adjacent `data/` or
`%LOCALAPPDATA%\OrganicMaps`. Package virtualization can change the physical
location seen by a packaged app; inspect it on an installed test system.

Before switching channels or uninstalling, open Bookmarks in the old app, select
each category, and export it as KMZ. Verify each file exists. Import the KMZ files
in the new app and check their contents before removing the old installation.
Export/import does not transfer settings or downloaded maps; configure and
download those again. Keep the old installation and export files until the import
has been verified.

## Required Windows validation

On clean Windows 11 x64 and Windows 10 22H2 test systems, copy the staged folder
away from the checkout, remove Qt from `PATH`, and launch as a normal user with a
read-only application directory. Check map rendering, HTTPS regional download,
bookmark persistence after restart, and log placement. Repeat with a non-ASCII
profile and launch path. Record any missing resources in the staging allowlist.
Validate world/catalog compatibility with a real regional download.
The current Windows path code uses `A` APIs and fixed-size path buffers. Long
paths remain a follow-up; test non-ASCII paths within that limit.

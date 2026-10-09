# Pull request: неофициальный порт Organic Maps для ОС Аврора

## Title

`[aurora] Add the Aurora OS port`

## Body

Adds an unofficial port of Organic Maps to **Aurora OS**, built on top of the
existing Sailfish OS port.

### What it does

- Uses `libauroraapp` (`Aurora::Application`) instead of the deprecated
  `libsailfishapp`; Qt 5.6, Silica. Entry points and the main QML path are
  adapted (`auroraapp` resolves `qml/<package id>.qml`, the entry point is
  installed as `qml/organicmaps.qml`).
- Aurora packaging: CMake `-DAURORA=ON`, package id `app.organicmaps.organicmaps`,
  desktop file with the Aurora `[X-Application]` section, RPM spec
  (`rpm/app.organicmaps.organicmaps.spec`) that passes `rpm-validator` (regular
  profile). No Sailjail profile on Aurora.
- GCC 12 compatibility (Aurora SDK ships GCC 12.3; the Sailfish port targets
  GCC >= 13): `std::bitset` constexpr → bit mask in `lane_way`; missing
  `<optional>` include; `resize_and_overwrite` read fix in `drules_format` and
  `mwm_diff`; `3party/glaze` patch (see `aurora/patches/glaze-gcc12.patch`).
- HTTP fix: the Qt network worker is now reached via
  `QMetaObject::invokeMethod(..., Qt::QueuedConnection, ...)` and a real slot
  (`NetworkWorker::ProcessJob`); the previous `QTimer::singleShot(0, worker, ...)`
  from a non-Qt thread never delivered the job, so map downloads did not start.
- Branding/attribution: official icon is kept (no rebranding); the required
  “Map data © OpenStreetMap and Organic Maps” attribution and an
  unofficial-port notice are shown in About and the main menu. `LICENSE`,
  `NOTICE` and `DATA_LICENSE.txt` are shipped in the package.
- Offline voice guidance on Aurora (which has no system TTS): a Piper backend
  (`sailfish/voice_guide.*`, guarded by `OMIM_AURORA`) uses espeak-ng for
  phonemization and onnxruntime for the vocoder. Voice models and the
  `espeak-ng-data` live in a separate noarch package `app.organicmaps.voices`
  (`aurora/voices-package/`) under `/usr/share/common/app.organicmaps/voices`,
  discovered via `Aurora::Application::organizationPathTo("voices")`; without it
  the app still works, just without spoken instructions.

### Licensing

- Code: Apache-2.0.
- Map data: see `DATA_LICENSE.txt` (attribution required; white-labeling /
  branding removal requires written permission).
- This is an **unofficial** build, not affiliated with the Organic Maps team.

### Build / test

- Built with the Aurora SDK cross-toolchains for `armv7hl` and `aarch64`;
  `rpm-validator -p regular` passes for both.
- Map downloads verified on an Aurora device (a map was downloaded and
  registered successfully).
- Offline voice guidance (Piper, Russian and English, medium) verified on the
  same Aurora device.

Port author: Leonid Yurasov — https://gitflic.ru/project/ub3gad/maps

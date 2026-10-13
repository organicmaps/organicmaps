# Organic Maps for Sailfish OS

A Silica (Qt 5, QML) app on top of the core libraries. See [docs/INSTALL.md](../docs/INSTALL.md#sailfish-os-app) for
building and installing.

## Port specifics

- CMake builds it with `-DSAILFISH=ON`, and `-DSAILFISH_HARBOUR=ON` for the `harbour-organicmaps` package. The RPM
  spec is `rpm/organicmaps.spec`.
- The app code lives in `liborganicmaps.so`, installed in `/usr/share/<package>/lib`, and the executable only starts
  it: thread-local variables in the executable break the GL drivers of libhybris on aarch64.
- Both packages use the D-Bus name `app.organicmaps.organicmaps` and keep their data in
  `~/.local/share/app.organicmaps/organicmaps`, so only one can be installed.

## Debugging

Sailfish OS doesn't show QML errors in the journal. To see the log, run the app from an SSH session on the phone:

```bash
QT_LOGGING_TO_CONSOLE=1 /usr/bin/organicmaps
```

## Files and links

The launcher passes map links (`geo:`, `om:`, `ge0:`, ...) to the app's `openUrl` D-Bus method, see
`organicmaps.desktop.in`. To try a link:

```bash
gdbus call --session --dest app.organicmaps.organicmaps --object-path /app/organicmaps \
  --method app.organicmaps.organicmaps.openUrl "['geo:59.33,18.06?z=15']"
```

## Assets

- `icons/*/organicmaps.png`: the launcher icons, downscaled from `android/app/ic_launcher-playstore.png` and cut to
  the leaf shape.
- `icons/placepage/ic_wiki.svg` and `ic_wikimedia_commons.svg`: traced with potrace from the iOS
  `ic_placepage_wiki@3x.png` and the Android `ic_wikimedia_commons_white`, whose clip path Qt's SVG renderer
  ignores.
- The other icons are installed from the Android resources at build time, the vector drawables converted to SVG by
  `tools/android_vector_to_svg.py`.

# Building and using icu

To use icu code, define UCONFIG_USE_LOCAL and add 3 include search paths:

1. $(OMIM_ROOT)/3party/icu
2. $(OMIM_ROOT)/3party/icu/icu/icu4c/source/common
3. $(OMIM_ROOT)/3party/icu/icu/icu4c/source/i18n

Only necessary sources are included for bidi and transliteration.
Please add other sources if you need more functionality.

# How to build and update icudtXXl.dat file

After updating ICU submodule, please also update the data/icudtXXl.dat file (XX is an ICU version).

```bash
mkdir build && cd build
ICU_DATA_FILTER_FILE=../icu_filter.json ../icu/icu4c/source/./configure --disable-shared --enable-static --disable-renaming --disable-extras --disable-icuio --disable-tests --disable-samples --with-data-packaging=archive
make -j$(nproc)
cp data/out/icudt??l.dat ../../../data/
```

Don't forget to delete an old .dat file in the $(OMIM_ROOT)/data and update symlink in `android/assets/`
and all references in the code:

```
indexer/transliteration_loader.cpp
16:  char const kICUDataFile[] = "icudt69l.dat";

android/script/replace_links.bat
42:cp -r ../data/icudt69l.dat assets/

iphone/Maps/Maps.xcodeproj/project.pbxproj
453:		BB7626B61E85599C0031D71C /* icudt69l.dat in Resources */ = {isa = PBXBuildFile; fileRef = BB7626B41E8559980031D71C /* icudt69l.dat */; };
1316:		BB7626B41E8559980031D71C /* icudt69l.dat */ = {isa = PBXFileReference; lastKnownFileType = file; name = icudt69l.dat; path = ../../data/icudt69l.dat; sourceTree = "<group>"; };
3557:				BB7626B41E8559980031D71C /* icudt69l.dat */,
3841:				BB7626B61E85599C0031D71C /* icudt69l.dat in Resources */,

qt/CMakeLists.txt
132:  icudt69l.dat
```

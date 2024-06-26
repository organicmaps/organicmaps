#!/bin/bash

# Target localisation files
android_strings_xml=$(find android/app/src/main/res/values* -name "strings.xml" -type f)
iphone_strings=$(find iphone/Maps/LocalizedStrings/*.lproj -name "Localizable.strings" -type f)
iphone_infoplist_strings=$(find iphone/Maps/LocalizedStrings/*.lproj -name "InfoPlist.strings" -type f)
iphone_stringsdict=$(find iphone/Maps/LocalizedStrings/*.lproj -name "Localizable.stringsdict" -type f)

# Perform one last migration from source files
./tools/unix/generate_localizations.sh

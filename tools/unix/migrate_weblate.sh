#!/bin/bash

# Target localisation files
android_strings_xml=$(find android/app/src/main/res/values* -name "strings.xml" -type f)
iphone_strings=$(find iphone/Maps/LocalizedStrings/*.lproj -name "Localizable.strings" -type f)
iphone_infoplist_strings=$(find iphone/Maps/LocalizedStrings/*.lproj -name "InfoPlist.strings" -type f)
iphone_stringsdict=$(find iphone/Maps/LocalizedStrings/*.lproj -name "Localizable.stringsdict" -type f)

# Resolve any missing languages
langs=$(egrep '=' data/strings/strings.txt | cut -d "=" -f1 | sed "s/[[:space:]]//g" | egrep -v "comment|tags|ref" | cut -d ":" -f 1 | sort -u)
for lang in $langs; do
	# https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes
	[[ $lang = "en" ]] && continue  # Skip source language

	alang=${lang/he/iw}  # Hebrew
	alang=${alang/id/in}  # Indonesian
	alang=${alang/zh-Hans/zh}  # Chinese (Simplified)
	alang=${alang/zh-Hant/zh-TW}  # Chinese (Traditional)
	alang=${alang/-/-r}   # Region: e.g. en-rGB
	ls android/app/src/main/res/values-$alang/strings.xml >/dev/null 2>&1 || (echo $alang - Android missing; mkdir -p android/app/src/main/res/values-$alang)

	ilang=$lang
	ls iphone/Maps/LocalizedStrings/$ilang.lproj/Localizable.strings >/dev/null 2>&1 || (echo $ilang - iPhone missing; mkdir -p iphone/Maps/LocalizedStrings/$ilang.lproj)
done
echo -n "Twine: "
echo "$langs" | wc -l
echo -n "Android: "
echo "$android_strings_xml" | wc -l
echo -n "iPhone: "
echo "$iphone_strings" | wc -l

# Perform one last migration from source files
./tools/unix/generate_localizations.sh

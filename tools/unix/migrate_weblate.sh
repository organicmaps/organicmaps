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

# Adapt \n to incluce a line break like Weblate does
sed -i "" -E '/<string /s/\\n/\n\\n/g' $android_strings_xml
# Remove blank lines before <! SECTION...
sed -i "" -E '/^$/d' $android_strings_xml
# Remove EOF newlines
for xml_file in $android_strings_xml; do
	truncate -s -1  $xml_file
done

## Prepare iPhone files for Weblate

# Remove blank lines between translatable strings
sed -i "" -E '/^$/d' $iphone_strings $iphone_infoplist_strings
# Readd two blank line before header comments
sed -i "" -E '/^[/][*][*]/i \
\
\
' $iphone_strings $iphone_infoplist_strings
# Readd blank line before comments
sed -i "" -E '/^[/][*][^*]/i \
\
' $iphone_strings $iphone_infoplist_strings
# Add a blank line after comment headers
sed -i "" -E $'/^[/][*][*]/,+1{/^"/s/^"/\\\n"/g;}' $iphone_strings $iphone_infoplist_strings
sed -i "" '1,/^./{/^$/d;}' $iphone_strings $iphone_infoplist_strings # Drop spurious first line

# Remove blank lines
sed -i "" -E '/^$/d' $iphone_stringsdict
# Remove comments - these don't roundtrip in Weblate
sed -i "" -E '/^<!--/d' $iphone_stringsdict

# Remove 'other' translation form for languages that don't have it in Weblate
sed -i "" -E '/<key>other<\/key>/,+1d' iphone/Maps/LocalizedStrings/{be,pl,ru,uk}.lproj/Localizable.stringsdict

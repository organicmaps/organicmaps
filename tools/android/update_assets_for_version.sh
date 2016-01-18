#!/bin/bash
set -e -u

[ $# -lt 2 ] && echo "Usage: $0 <source> <dest>" && exit 1
SRC="$1"
DST="$2"

# Remove old links
rm -rf "$DST" || true
mkdir "$DST" || true

files=(copyright.html faq.html resources-default resources-ldpi_legacy resources-mdpi_legacy resources-hdpi_legacy resources-xhdpi_legacy resources-xxhdpi_legacy categories.txt classificator.txt
       resources-ldpi_dark resources-mdpi_dark resources-hdpi_dark resources-xhdpi_dark resources-xxhdpi_dark
       resources-ldpi_clear resources-mdpi_clear resources-hdpi_clear resources-xhdpi_clear resources-xxhdpi_clear
       resources-6plus_legacy resources-6plus_clear resources-6plus_dark
       types.txt fonts_blacklist.txt fonts_whitelist.txt languages.txt unicode_blocks.txt
       drules_proto_legacy.bin drules_proto_dark.bin drules_proto_clear.bin external_resources.txt packed_polygons.bin countries.txt sound-strings colors.txt patterns.txt)

for item in ${files[*]}
do
  ln -s "$SRC/$item" "$DST/$item"
done

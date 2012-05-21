#!/bin/bash
set -x -u

SRC=../../data/flags
DST=../../android/res

# *@2x.png
sobakaPos=7

for file in $SRC/*.png;
do
  filename=$(basename "$file")

  if [[ "$file" == *\@2x.png ]]; then
    strLen=${#filename}
    prefixName=${filename:0:`expr $strLen - $sobakaPos`}

    rm "$DST/drawable-xhdpi/${prefixName}.png"
	ln -s "../$SRC/$filename" "$DST/drawable-xhdpi/${prefixName}.png"
  else
    rm "$DST/drawable-mdpi/$filename"
    ln -s "../$SRC/$filename" "$DST/drawable-mdpi/$filename"
  fi
done

# rename do.png because of aapt - it doesn't support resource name "do"
mv "$DST/drawable-xhdpi/do.png" "$DST/drawable-xhdpi/do_hack.png"
mv "$DST/drawable-mdpi/do.png" "$DST/drawable-mdpi/do_hack.png"
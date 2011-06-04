#!/bin/bash


cat res_files.txt | while read line; do
  GUID=`'/c/Program Files/Microsoft SDKs/Windows/v7.1/Bin/Uuidgen.Exe'`
  echo "$GUID $line"
done

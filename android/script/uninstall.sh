#!/bin/bash

LITE=com.mapswithme.maps
PRO=com.mapswithme.maps.pro 

package=lp
serial=

while getopts s:p: opt; do
  case $opt in
  s)
      serial=$OPTARG
      ;;
  p)
      package=$OPTARG
      ;;
  esac
done
shift $((OPTIND - 1))
    
if [ -z $serial ]; then
    ID=""
else
    ID="-s $serial"
fi

if [[ $package == *l* ]]
then
    echo "uninstalling $LITE"
    adb $ID uninstall $LITE
fi

if [[ $package == *p* ]]
then 
    echo "uninstalling $PRO"
    adb $ID uninstall $PRO
fi

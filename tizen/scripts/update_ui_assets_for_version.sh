#!/bin/bash
set -x -u

DST=$1

# Remove old links
mkdir $DST
mkdir $DST/screen-density-high
ln -s ../../../../data/flags $DST/screen-density-high/flags
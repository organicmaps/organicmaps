#!/bin/bash

for file in data/*.mwm.*
do
	bzip2 -9 -k -v $file
done

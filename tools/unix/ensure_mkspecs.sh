#!/bin/bash
MY_PATH=`dirname $(stat -f %N $PWD"/"$0)`
sudo ln -sf $MY_PATH/../mkspecs/iphone* /usr/local/Qt4.7/mkspecs/
sudo ln -sf $MY_PATH/../mkspecs/common/iphone-* /usr/local/Qt4.7/mkspecs/common/

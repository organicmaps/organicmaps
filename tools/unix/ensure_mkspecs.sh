MY_PATH=`echo $0 | grep -o '.*/'`
sudo ln -sf $MY_PATH/../mkspecs/iphone* /usr/local/Qt4.7/mkspecs/
sudo ln -sf $MY_PATH/../mkspecs/common/iphone-* /usr/local/Qt4.7/mkspecs/common/

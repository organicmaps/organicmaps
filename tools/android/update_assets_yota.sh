./update_assets_for_version.sh ../../android/MapsWithMeLite/assets
./add_assets_mwm-ttf.sh ../../android/MapsWithMeLite/assets

./update_assets_for_version.sh ../../android/YoPme/assets
./add_assets_mwm-ttf.sh ../../android/YoPme/assets

SRC=../../../data
DST=../../android/YoPme/assets

rm -rf $DST/drules_proto.bin
ln -s $SRC/drules_proto-bw.bin $DST/drules_proto.bin

rm -rf $DST/resources-ldpi
rm -rf $DST/resources-mdpi
rm -rf $DST/resources-hdpi
rm -rf $DST/resources-xhdpi
rm -rf $DST/resources-xxhdpi
ln -s $SRC/resources-yota $DST/resources-mdpi


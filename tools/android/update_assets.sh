./update_assets_for_version.sh ../../android/MapsWithMePro/assets
./update_assets_for_version.sh ../../android/MapsWithMeLite/assets
./update_assets_for_version.sh ../../android/MapsWithMeLite.Samsung/assets

./update_assets_for_version.sh ../../android/YoPme/assets
SRC=../../../data
DST=../../android/YoPme/assets
rm -rf $DST/drules_proto.bin
ln -s $SRC/drules_proto-bw.bin $DST/drules_proto.bin

./update_flags.sh

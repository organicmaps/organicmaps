echo "Build drawing rules light"
python ../kothic/libkomwm.py  -s ../../data/styles/normal_light.mapcss -o ../../data/drules_proto

echo "Build drawing rules dark"
python ../kothic/libkomwm.py -s ../../data/styles/normal_dark.mapcss -o ../../data/drules_proto_dark

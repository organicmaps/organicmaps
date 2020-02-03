cd ..

rm -rf assets/
mkdir assets
cp ../data/categories.txt assets/
cp ../data/categories_cuisines.txt assets/
cp ../data/classificator.txt assets/
cp ../data/colors.txt assets/
cp ../data/copyright.html assets/
cp ../data/countries.txt assets/
cp ../data/drules_proto_dark.bin assets/
cp ../data/drules_proto_clear.bin assets/
cp ../data/drules_proto_vehicle_dark.bin assets/
cp ../data/drules_proto_vehicle_clear.bin assets/
cp ../data/editor.config assets/
cp ../data/external_resources.txt assets/
cp ../data/faq.html assets/
cp ../data/fonts_blacklist.txt assets/
cp ../data/fonts_whitelist.txt assets/
cp ../data/languages.txt assets/
cp ../data/packed_polygons.bin assets/
cp ../data/patterns.txt assets/
cp ../data/types.txt assets/
cp ../data/unicode_blocks.txt assets/
cp ../data/opening_hours_how_to_edit.html assets/
cp ../data/ugc_types.csv assets/
cp -r ../data/taxi_places/ assets/

cp -r ../data/resources-hdpi_dark/ assets/
cp -r ../data/resources-hdpi_clear/ assets/
cp -r ../data/resources-mdpi_dark/ assets/
cp -r ../data/resources-mdpi_clear/ assets/
cp -r ../data/resources-xhdpi_dark/ assets/
cp -r ../data/resources-xhdpi_clear/ assets/
cp -r ../data/resources-xxhdpi_dark/ assets/
cp -r ../data/resources-xxhdpi_clear/ assets/
cp -r ../data/resources-6plus_dark/ assets/
cp -r ../data/resources-6plus_clear/ assets/

cp -r ../data/sound-strings/ assets/
cp -r ../data/countries-strings/ assets/

cp -r ../data/icudt57l.dat assets/

cp -r ../data/local_ads_symbols.txt assets/

rm -rf flavors/mwm-ttf-assets
mkdir flavors\\mwm-ttf-assets
cp ../data/01_dejavusans.ttf flavors/mwm-ttf-assets/
cp ../data/02_droidsans-fallback.ttf flavors/mwm-ttf-assets/
cp ../data/03_jomolhari-id-a3d.ttf flavors/mwm-ttf-assets/
cp ../data/04_padauk.ttf flavors/mwm-ttf-assets/
cp ../data/05_khmeros.ttf flavors/mwm-ttf-assets/
cp ../data/06_code2000.ttf flavors/mwm-ttf-assets/
cp ../data/07_roboto_medium.ttf flavors/mwm-ttf-assets/
cp ../data/World.mwm flavors/mwm-ttf-assets/
cp ../data/WorldCoasts.mwm flavors/mwm-ttf-assets/

rm -rf res/values-zh-rHK/
mkdir res\\values-zh-rHK
cp res/values-zh-rTW/strings.xml res/values-zh-rHK/

rm -rf res/values-zh-rMO/
mkdir res\\values-zh-rMO
cp res/values-zh-rTW/strings.xml res/values-zh-rMO/

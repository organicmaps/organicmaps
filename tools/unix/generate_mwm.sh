#!/bin/bash
###########################
# Builds one specific MWM #
###########################

# Prerequisites:
#
# - The script should be placed in omim/tools/unix
# - Compiled generator_tool somewhere in omim/../build/out/whatever
# - For routing, compiled OSRM binaries in omim/3party/osrm/osrm-backend/build
# - Data path with classificators etc. should be present in omim/data

# Cross-borders routing index is not created, since we don't assume
# the source file to be one of the pre-defined countries.

if [ $# -lt 1 ]; then
	echo ''
	echo "Usage: $0 \<file.o5m\>"
	echo "To build routing: $0 \<file.o5m\> \<profile.lua\>"
	echo ''
	exit 0
fi
SOURCE_FILE="$1"
BASE_NAME="${SOURCE_FILE%%.*}"
MY_PATH=$(cd $(dirname $0); pwd)

if [ $# -gt 1 ]; then
	MODE=routing
else
	MODE=mwm
fi

# find generator_tool. Supply your own priority dir if needed
PRIORITY_PATH="$MY_PATH/../../build_omim-zv-desktop-Debug"
IT_PATHS_ARRAY=()
for i in $PRIORITY_PATH $MY_PATH/../.. $MY_PATH/../../../*omim*elease* $MY_PATH/../../../*omim*ebug; do
	if [ -d "$i/out" ]; then
		IT_PATHS_ARRAY+=("$i/out/release/generator_tool" "$i/out/debug/generator_tool")
	fi
done

for i in ${IT_PATHS_ARRAY[@]}; do
	if [ -x "$i" ]; then
		GENERATOR_TOOL="$i"
		echo TOOL: $GENERATOR_TOOL
		break
	fi
done

if [[ ! -n $GENERATOR_TOOL ]]; then
	echo "No generator_tool found in ${IT_PATHS_ARRAY[*]}"
	exit 1
fi

if [[ "`uname`" == 'Darwin' ]]; then
	INTDIR=$(mktemp -d -t mwmgen)
else
	INTDIR=$(mktemp -d)
fi

if [ "$MODE" == "mwm" ]; then

	INTDIR_FLAG="--intermediate_data_path=$INTDIR/ --osm_file_type=o5m --osm_file_name=$SOURCE_FILE --node_storage=map"
	$GENERATOR_TOOL $INTDIR_FLAG --preprocess=true

	DATA_PATH="$MY_PATH/../../data/"
	$GENERATOR_TOOL --data_path=$MY_PATH --user_resource_path=$DATA_PATH $INTDIR_FLAG --generate_features=true --generate_geometry=true --generate_index=true --generate_search_index=true --output=$BASE_NAME

elif [ "$MODE" == "routing" ]; then

	OSRM_PATH="$MY_PATH/../../3party/osrm/osrm-backend"
	BIN_PATH="$OSRM_PATH/build"
	if [ ! -x "$BIN_PATH/osrm-extract" ]; then
		echo "Please compile OSRM binaries to $BIN_PATH"
		exit 1
	fi
	if [ ! -r "$BASE_NAME.mwm" ]; then
		echo "Please build mwm file beforehand"
		exit 1
	fi

	EXTRACT_CFG="$OSRM_PATH/../extractor.ini"
	PREPARE_CFG="$OSRM_PATH/../contractor.ini"
	if [ -r "$2" ]; then
		PROFILE="$2"
	else
		echo "$2 is not a profile, using standard car.lua"
		PROFILE="$OSRM_PATH/profiles/car.lua"
	fi

	PBF="$INTDIR/$BASENAME.pbf"
	OSRM="$INTDIR/$BASENAME.osrm"
	export STXXLCFG=~/.stxxl
	# just a guess
	OSMCONVERT=~/osmctools/osmconvert
	if [ ! -x $OSMCONVERT ]; then
		OSMCONVERT="$INTDIR/osmconvert"
		wget -O - http://m.m.i24.cc/osmconvert.c | cc -x c - -lz -O3 -o $OSMCONVERT
	fi
	$OSMCONVERT $SOURCE_FILE -o=$PBF
	"$BIN_PATH/osrm-extract" --config "$EXTRACT_CFG" --profile "$PROFILE" "$PBF"
	rm "$PBF"
	"$BIN_PATH/osrm-prepare" --config "$PREPARE_CFG" --profile "$PROFILE" "$OSRM"
	"$BIN_PATH/osrm-mapsme" -i "$OSRM"
	# create fake poly file
	POLY="$MY_PATH/borders/$BASE_NAME.poly"
	if [ ! -r "$POLY" ]; then
		POLY_DIR="$(dirname "$POLY")"
		mkdir -p "$POLY_DIR"
		cat > "$POLY" <<'EOPOLY'
fake
1
	-180.0	-90.0
	-180.0	90.0
	180.0	90.0
	180.0	-90.0
	-180.0	-90.0
END
END
EOPOLY
		if [ -r "$MY_PATH/polygons.lst" ]; then
			mv "$MY_PATH/polygons.lst" "$POLY_DIR"
		fi
		echo "$BASE_NAME" > "$MY_PATH/polygons.lst"
	fi
	$GENERATOR_TOOL --osrm_file_name=$OSRM --data_path=$MY_PATH --output=$BASE_NAME
	if [ -n "$POLY_DIR" ]; then
		# remove fake poly
		rm "$POLY"
		rm "$MY_PATH/polygons.lst"
		if [ -r "$POLY_DIR/polygons.lst" ]; then
			mv "$POLY_DIR/polygons.lst" "$MY_PATH"
		fi
		if [ -z "$(ls -A "$POLY_DIR")" ]; then
			rm -r "$POLY_DIR"
		fi
	fi

fi

rm -r $INTDIR

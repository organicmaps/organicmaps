#!/bin/bash

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"

types_to_keep="amenity-|shop-|craft-|man_made-|power-plant|tourism"

editor_types=$(grep -oP 'id="\K[^"]*' $OMIM_PATH/data/editor.config | sort)
drule_types=$(awk -F'"' '/cont \{/ {getline; if ($2) print $2}' $OMIM_PATH/data/drules_proto.txt | grep -E "^($types_to_keep)" | sort)

echo "Missing Editor Types:"
echo "Only matching types: $types_to_keep" 
comm -13 <(echo "$editor_types") <(echo "$drule_types")

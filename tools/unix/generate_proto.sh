#!/bin/bash
set -e -u -x

OMIM_PATH="${OMIM_PATH:-$(dirname "$0")/../..}"

rm "$OMIM_PATH/indexer"/drules_struct.pb.* || true
rm "$OMIM_PATH/tools/python/stylesheet"/drules_struct_pb2.* || true
rm "$OMIM_PATH/tools/kothic/src"/drules_struct_pb2.* || true

PROTO="$OMIM_PATH/indexer/drules_struct.proto"
protoc --proto_path="$OMIM_PATH/indexer" --cpp_out="$OMIM_PATH/indexer" "$PROTO"
protoc --proto_path="$OMIM_PATH/indexer" --python_out="$OMIM_PATH/tools/python/stylesheet" "$PROTO"
protoc --proto_path="$OMIM_PATH/indexer" --python_out="$OMIM_PATH/tools/kothic/src" "$PROTO"

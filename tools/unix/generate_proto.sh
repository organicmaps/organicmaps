#!/bin/bash
set -e -u -x

MY_PATH=`pwd`

cd ../..

rm indexer/drules_struct.pb.* || true
rm tools/python/stylesheet/drules_struct_pb2.* || true
rm tools/kothic/drules_struct_pb2.* || true

protoc --proto_path=indexer --cpp_out=indexer indexer/drules_struct.proto
if [ $? -ne 0  ]; then
  echo "Error"
  exit 1 # error
fi

protoc --proto_path=indexer --python_out=./tools/python/stylesheet indexer/drules_struct.proto
if [ $? -ne 0  ]; then
  echo "Error"
  exit 1 # error
fi

protoc --proto_path=indexer --python_out=./tools/kothic indexer/drules_struct.proto
if [ $? -ne 0  ]; then
  echo "Error"
  exit 1 # error
fi

cd $MY_PATH

echo "Done"
exit 0 # ok

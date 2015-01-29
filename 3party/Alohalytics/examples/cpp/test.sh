#!/bin/bash
SERVER=${ALOHA_SERVER:-http://localhost/upload}
make clean all
./build/example --server_url=$SERVER --event=Event1
./build/example --server_url=$SERVER --values=Value1
./build/example --server_url=$SERVER --event=Event2 --values=Value2
./build/example --server_url=$SERVER --event=Event3 --values=Foo=foo,Bar=bar

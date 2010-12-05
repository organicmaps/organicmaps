#!/bin/bash

mv $1 ~/tmp/file.bak
perl -pe 's/\r\n|\n|\r/\n/g' ~/tmp/file.bak > $1
 
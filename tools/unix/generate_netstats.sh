#!/bin/bash

RX_BYTES=`ifconfig eth0 | grep -o 'RX bytes:[0-9]*' | grep -o '[0-9]*'`
TX_BYTES=`ifconfig eth0 | grep -o 'TX bytes:[0-9]*' | grep -o '[0-9]*'`
ACTIVE_DOWNLOADS=`netstat -n | grep 'tcp.*:80 .*ESTABLISHED' | wc -l`

echo $ACTIVE_DOWNLOADS $RX_BYTES $TX_BYTES $(($RX_BYTES + $TX_BYTES))

#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/translate.awk {en=en+nl+uk+zh-TW+cs+fr+it+ja+ko+pt+es+pl+de+ru} "$1"

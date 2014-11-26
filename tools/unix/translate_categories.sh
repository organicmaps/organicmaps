#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/translate.awk {en=en+ru+uk+de+fr+it+es+ko+ja+cs+nl+zh-TW+pl+pt+hu+th+zh-CN+ar+da+tr+sv} "$1"

#!/bin/bash
set -e -u -x
MY_PATH=`dirname $(stat -f %N $PWD"/"$0)`
SAVED_PATH="${HOME}/Library/Application Support/Google/Chrome/Default/FileSystem/chrome-extension_jemlklgaibiijojffihnhieihhagocma_0/Persistent/chrome-oWHMA7fJwDx8JDjs"
SAVED_FILE="${SAVED_PATH}/${2}"

rm "$SAVED_FILE" || true

for i in $(cat $1) ; do
    if ! osascript "$MY_PATH/download.applescript" "$i" "${SAVED_FILE}"
    then
	echo "applescript failed";
	sleep 10s
	osascript "$MY_PATH/download.applescript" "$i" "${SAVED_FILE}"
    fi 

    if [ ! -f "${SAVED_FILE}" ]
    then
	sleep 5s
    fi

    if [ ! -f "${SAVED_FILE}" ]
    then
	echo "file not found"
	exit 1
    fi

    mv "${SAVED_FILE}" $3/${i##*/}".html"
done
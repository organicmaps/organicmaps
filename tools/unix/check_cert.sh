#!/bin/bash

MONTHS_BEFORE_EXPIRATION_TO_BREAK="3"
DAYS_WARNING_INTERVAL="14"
PRIVATE_H=$1
MODE=$2

if [[ "$MODE" == "" ]]; then
	exit 0
fi
if [[ "$PRIVATE_H" == "" ]]; then
	PRIVATE_H=$(dirname $0)/../../private.h
fi

while read line; do
	eval $line
done < <(
	sed -e ':a' -e 'N' -e '$!ba' -e 's/\\\n//g' $PRIVATE_H |\
	grep "^#define USER_BINDING_PKCS12" |\
	sed -e 's/#define //' -e 's/ /=/' -e 's/$/;/'
)
if [[ "$USER_BINDING_PKCS12" == "" ]]; then
	echo "No certificate found in private.h, skipping check"
	exit 0
fi

read mon day time year tz < <(
	echo $USER_BINDING_PKCS12 |\
	base64 --decode |\
	openssl pkcs12 -passin pass:$USER_BINDING_PKCS12_PASSWORD -clcerts -nodes |\
	openssl x509 -noout -enddate |\
	cut -d '=' -f 2-
)

if [[ $(uname) == "Darwin" ]]; then
	warning_timestamp=`LANG=C LC_ALL=C date -j -v "+${MONTHS_BEFORE_EXPIRATION_TO_BREAK}m" -v "+${DAYS_WARNING_INTERVAL}d" +%s`
	threshold_timestamp=`LANG=C LC_ALL=C date -j -v "+${MONTHS_BEFORE_EXPIRATION_TO_BREAK}m" +%s`
	cert_end_timestamp=`LANG=C LC_ALL=C date -j -f "%Y %b %d %H:%M:%S %Z" "$year $mon $day $time $tz" +%s`
else
	warning_timestamp=`LANG=C LC_ALL=C date --date "+${MONTHS_BEFORE_EXPIRATION_TO_BREAK}month ${DAYS_WARNING_INTERVAL}days" +%s`
	threshold_timestamp=`LANG=C LC_ALL=C date --date "+$MONTHS_BEFORE_EXPIRATION_TO_BREAK months" +%s`
	cert_end_timestamp=`LANG=C LC_ALL=C date --date "$mon $day $year $time $tz" +%s`
fi
if [[ "$MODE" == "check" ]]; then
	if [[ "$threshold_timestamp" -gt "$cert_end_timestamp" ]]; then
		echo "Our client certificate end date of $mon $day $time $year $tz is within $MONTHS_BEFORE_EXPIRATION_TO_BREAK months from now."
		echo "Update this certificate immediately!"
		echo "Error"
		exit 1
	elif [[ "$warning_timestamp" -gt "$cert_end_timestamp" ]]; then
		echo "Our client certificate end date of $mon $day $time $year $tz is within ${MONTHS_BEFORE_EXPIRATION_TO_BREAK} month ${DAYS_WARNING_INTERVAL} days from now."
		echo "Update this certificate!"
		echo "Warning"
		exit 1
	fi
fi
if [[ "$threshold_timestamp" -gt "$cert_end_timestamp" ]]; then
	echo "Our client certificate end date of $mon $day $time $year $tz is within $MONTHS_BEFORE_EXPIRATION_TO_BREAK months from now."
	echo "Update this certificate immediately!"
	echo "Aborting build"
	exit 1
else
	echo "Our client certificate end date of $mon $day $time $year $tz is later than $MONTHS_BEFORE_EXPIRATION_TO_BREAK months from now."
	echo "Certificate check passed, continuing the build"
	exit 0
fi

#!/bin/bash

APK=MapsWithMeTest/bin/MapsWithMeTest-debug.apk
TEST_APK=tests/bin/MapsWithMeTests-debug.apk
TEST_PROJ=tests
SPOON_JAR=tests/tools/spoon-runner-1.0.5-jar-with-dependencies.jar 

if [ ! -f $APK ]; then
    echo "File $APK not found, please run 'ant debug' from $TEST_PROJ "
elif [ ! -f $TEST_APK ]; then
    echo "File $TEST_APK not found, please run 'ant debug' from $TEST_PROJ "    
else
    # normal execution
    java -jar $SPOON_JAR  --apk $APK --test-apk $TEST_APK
    echo "Done testing"
    open spoon-output/index.html
fi

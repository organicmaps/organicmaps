#!/bin/sh
# put this to /Library/StartupItems/pugixml-autotest/pugixml-autotest, then create
# file StartupParameters.plist in the same folder with the following contents:
# <plist><dict><key>Provides</key><array><string>pugixml-autotest</string></array></dict></plist>
# don't forget to chmod +x pugixml-autotest and to replace /Users/USERNAME with actual path

if [ "$1" = "start" ]
then
	PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin
	cd /Users/USERNAME/pugixml
	perl tests/autotest-remote-host.pl "shutdown -h now" &
fi

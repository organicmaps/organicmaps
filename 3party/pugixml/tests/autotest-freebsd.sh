#!/bin/sh
# put this to /etc/rc.d/pugixml-autotest
# don't forget to chmod +x pugixml-autotest and to replace /home/USERNAME with actual path

if [ "$1" = "start" -o "$1" = "faststart" ]
then
	PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin
	cd /home/USERNAME/pugixml
	perl tests/autotest-remote-host.pl "shutdown -p now" &
fi

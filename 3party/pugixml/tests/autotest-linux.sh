#!/bin/sh
# chkconfig: 2345 20 80
# description: pugixml autotest script
# put this to /etc/init.d/pugixml-autotest.sh, then launch
# Debian/Ubuntu: sudo update-rc.d pugixml-autotest.sh defaults 80
# Fedora/RedHat: sudo chkconfig --add pugixml-autotest.sh
# don't forget to chmod +x pugixml-autotest.sh and to replace /home/USERNAME with actual path

if [ "$1" = "start" ]
then
	PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin
	cd /home/USERNAME/pugixml
	perl tests/autotest-remote-host.pl "shutdown -P now" &
fi

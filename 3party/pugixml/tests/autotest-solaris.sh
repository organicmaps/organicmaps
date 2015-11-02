#!/bin/sh
# put this to /etc/init.d/pugixml-autotest.sh, then launch
# ln -s /etc/init.d/pugixml-autotest.sh /etc/rc3.d/S80pugixml-autotest
# don't forget to chmod +x pugixml-autotest.sh and to replace /export/home/USERNAME with actual path

if [ "$1" = "start" ]
then
	cd /export/home/USERNAME/pugixml
	perl tests/autotest-remote-host.pl "shutdown -g 0 -i 5 -y" &
fi

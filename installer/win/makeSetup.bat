@echo on

set PATH="C:\Program Files\Windows Installer XML v3.5\bin";%PATH%

candle MapsWithMe.wxs

if NOT ERRORLEVEL 0 echo "candle returned error %ERRORLEVEL%"

light MapsWithMe.wixobj

if NOT ERRORLEVEL 0 echo "light returned error %ERRORLEVEL%"

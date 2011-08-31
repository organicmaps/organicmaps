@echo on

del MapsWithMe.wxs > nul
del MapsWithMe.wixobj > nul
del MapsWithMe.wixpdb > nul
del MapsWithMe.msi > nul

perl generator.pl > MapsWithMe.wxs

if NOT ERRORLEVEL 0 echo "Generator returned error %ERRORLEVEL%"

set PATH="C:\Program Files (x86)\Windows Installer XML v3.5\bin";%PATH%

candle -ext WiXUtilExtension MapsWithMe.wxs

if NOT ERRORLEVEL 0 echo "candle returned error %ERRORLEVEL%"

light -ext WiXUtilExtension MapsWithMe.wixobj

if NOT ERRORLEVEL 0 echo "light returned error %ERRORLEVEL%"

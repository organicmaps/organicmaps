SET configuration=Debug
IF NOT EXIST deps\gyp\ git clone --depth 1 https://chromium.googlesource.com/external/gyp.git deps/gyp
IF EXIST %configuration% rd /s /q %configuration%
del variant.sln
del tests.vcxproj
C:\Python27\python.exe deps/gyp/gyp_main.py variant.gyp --depth=. -f msvs -G msvs_version=2013
msbuild variant.sln /nologo /p:Configuration=%configuration%;Platform=Win32
.\"%configuration%"\tests.exe
@echo on
IF [%1]==[] (
  echo Please specify mingw or msvc2008 build should be used
  exit /B 1
)
set PKG_TYPE=%1
set PKG_DIR=MapsWithMe
set BASE_PATH=C:\Qt\omim-build-%PKG_TYPE%-release
set MINGW_BIN_PATH=C:\Qt\SDK\mingw\bin
set QT_BIN_PATH=C:\Qt\SDK\Desktop\Qt\4.7.2\%PKG_TYPE%\bin
set RAR_CMD="C:\Program Files\WinRar\WinRar.exe"
set ZIP_NAME=MapsWithMe-%PKG_TYPE%.zip
set PKG_RES_DIR=%PKG_DIR%\resources

del /F /S /Q %PKG_DIR%
del /Q %ZIP_NAME%

mkdir %PKG_DIR%
copy %BASE_PATH%\out\release\MapsWithMe.exe %PKG_DIR%\
mkdir %PKG_DIR%\data
mkdir %PKG_RES_DIR%
copy %BASE_PATH%\data\*.txt %PKG_RES_DIR%
copy %BASE_PATH%\data\drawing_rules.bin %PKG_RES_DIR%
copy %BASE_PATH%\data\*.skn %PKG_RES_DIR%
copy %BASE_PATH%\data\*.png %PKG_RES_DIR%
copy %BASE_PATH%\data\*.ttf %PKG_RES_DIR%

copy %BASE_PATH%\data\eula.html %PKG_RES_DIR%
copy %BASE_PATH%\data\welcome.html %PKG_RES_DIR%

copy %BASE_PATH%\data\maps.update %PKG_RES_DIR%
copy %BASE_PATH%\data\World.mwm %PKG_RES_DIR%

copy %BASE_PATH%\data\dictionary.slf %PKG_RES_DIR%


IF %PKG_TYPE%==mingw (
  copy %MINGW_BIN_PATH%\pthreadGC2.dll %PKG_DIR%\
  copy %QT_BIN_PATH%\libgcc_s_dw2-1.dll %PKG_DIR%\
  copy %QT_BIN_PATH%\mingwm10.dll %PKG_DIR%\
) ELSE (
  copy C:\Windows\winsxs\x86_microsoft.vc90.crt_1fc8b3b9a1e18e3b_9.0.30729.4926_none_508ed732bcbc0e5a\msvcp90.dll %PKG_DIR%\
  copy C:\Windows\winsxs\x86_microsoft.vc90.crt_1fc8b3b9a1e18e3b_9.0.30729.4926_none_508ed732bcbc0e5a\msvcr90.dll %PKG_DIR%\
)

copy %QT_BIN_PATH%\QtCore4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtGui4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtNetwork4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtOpenGl4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtWebKit4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\phonon4.dll %PKG_DIR%\

%RAR_CMD% a -m5 -r %ZIP_NAME% %PKG_DIR%\*

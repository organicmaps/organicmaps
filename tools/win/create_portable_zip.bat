set PKG_TYPE=mingw
set PKG_DIR=MapsWithMe
set BASE_PATH=C:\Qt\omim-build-%PKG_TYPE%-release
set MINGW_BIN_PATH=C:\Qt\qtcreator-2.0.1\mingw\bin
set QT_BIN_PATH=C:\Qt\4.7.0-%PKG_TYPE%\bin
set RAR_CMD="C:\Program Files\WinRar\WinRar.exe"
set ZIP_NAME=MapsWithMe-%PKG_TYPE%.zip
set PKG_RES_DIR=%PKG_DIR%\resources

del /F /S /Q %PKG_DIR%
del %ZIP_NAME%

mkdir %PKG_DIR%
copy %BASE_PATH%\out\release\MapsWithMe.exe %PKG_DIR%\
mkdir %PKG_DIR%\data
mkdir %PKG_RES_DIR%
copy %BASE_PATH%\data\classificator.txt %PKG_RES_DIR%
copy %BASE_PATH%\data\visibility.txt %PKG_RES_DIR%
copy %BASE_PATH%\data\drawing_rules.bin %PKG_RES_DIR%
copy %BASE_PATH%\data\basic.skn %PKG_RES_DIR%
copy %BASE_PATH%\data\symbols_24.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_8.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_10.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_12.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_14.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_16.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_20.png %PKG_RES_DIR%
copy %BASE_PATH%\data\dejavusans_24.png %PKG_RES_DIR%

IF %PKG_TYPE%==mingw (
  copy %BASE_PATH%\..\omim\tools\redist\pthreadGC2.dll %PKG_DIR%\
  copy %MINGW_BIN_PATH%\libgcc_s_dw2-1.dll %PKG_DIR%\
  copy %MINGW_BIN_PATH%\mingwm10.dll %PKG_DIR%\
) ELSE (
  echo "aa"
)

copy %QT_BIN_PATH%\QtCore4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtGui4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtNetwork4.dll %PKG_DIR%\
copy %QT_BIN_PATH%\QtOpenGl4.dll %PKG_DIR%\

%RAR_CMD% a -m5 -r %ZIP_NAME% %PKG_DIR%\*

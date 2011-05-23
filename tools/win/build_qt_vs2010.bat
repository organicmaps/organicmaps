@echo on
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /x86 /xp

set PATH=C:\Qt\SDK\Desktop\Qt\4.7.3\msvc2010\bin;%PATH%

C:\Qt\SDK\QtSources\4.7.3\configure.exe -I C:\Qt\openssl\inc32 -L C:\Qt\openssl\out32 -debug-and-release -opensource -shared -ltcg -fast -no-accessibility -no-sql-sqlite -no-qt3support -platform win32-msvc2010 -no-dsp -no-vcproj -no-incredibuild-xge -no-dbus -openssl-linked OPENSSL_LIBS="-lssleay32 -llibeay32" -no-dbus -no-phonon -no-phonon-backend -no-multimedia -no-audio-backend -webkit -no-script -no-scripttools -no-declarative -no-declarative-debug -mp -saveconfig vs2010

nmake
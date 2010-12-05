SUMMARY
=======
This is a port of expat for AmigaOS 4.0 which includes the
SDK, some XML tools and the libraries.

Both static and shared library versions are supported.

The static library version is limited to clib2 although it should
be possible to use newlib with the appopriate compile options.

The shared library version is based on the work of Fredrik Wikstrom
and is currently limited to PPC only.


HISTORY
=======
4.2  - updated to correspond to Expat 2.0.1 release
     - bumped copyright banners and versions
     - simplified amigaconfig.h
     - updated include/libraries/expat.h file
     - modified launch.c to use contructor/deconstructor
     - removed need for amiga_main() from expat utilities

4.1  - fixed memory freeing bug in shared library version
     - now allocates shared memory

4.0  - updated for corresponding Expat 2.0 release
     - some minor CVS related changes

3.1  - removed obsolete sfd file
     - added library description xml file
     - refactored Makefile
     - removed extraneous VARARGS68K keywords
     - reworked default memory handling functions in shared lib
     - updated amigaconfig.h

3.0  - initial release
     - based on expat 1.95.8


BUILDING
========
To build expat.library, xmlwf tool, examples and run the test suite,
simply type 'make all' in the amiga subdirectory.

The test suite will compile and run for both the static and shared
library versions.


INSTALLATION
============
To install both static and shared versions of expat into the
AmigaOS SDK type 'make install' in the amiga subdirectory.


CONFIGURATION
=============
You may want to edit the lib/amigaconfig.h file to remove
DTD and/or XML namespace support if they are not required by your
specific application for a smaller and faster implementation.


TO DO
=====
- wide character support (UTF-16)
- provide 68k backwards compatibility

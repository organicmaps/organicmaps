cxa_demangle.cpp was copied here to avoid compilation bug on Android, when
compiler can't detect abi::__cxa_demangle() function from cxxabi.h include file.

The patch was applied to include/details.util.hpp by Alex Zolotarev (me@alex.bio)

Another simple patch changes includes in all headers to avoid -I compiler parameter.
Also removed Windows-style line endings from files in external folder.
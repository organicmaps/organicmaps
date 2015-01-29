cxa_demangle.cpp was copied here to avoid compilation bug on Android, when
compiler can't detect abi::__cxa_demangle() function from cxxabi.h include file.

The patch was applied to include/details.util.hpp by Alex Zolotarev

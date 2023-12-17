#pragma once

#include "std/target_os.hpp"

// Surprisingly, clang defines __GNUC__
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif  // defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)

#include "indexer/drules_struct.pb.h"

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif  // defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)

#pragma once

// Surprisingly, clang defines __GNUC__
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif  // defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)

// #if defined(__clang__)
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wdeprecated-declarations"
// #endif  // defined(__clang__)

#include <boost/container_hash/hash.hpp>

// #if defined(__clang__)
// #pragma clang diagnostic pop
// #endif  // defined(__clang__)

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif  // defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)

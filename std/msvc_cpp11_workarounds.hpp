// Workarounds to fix C++11 bugs or non-implemented features of MS Visual C++.
// These workarounds are to be done only for older versions of MSVC (and probably
// to be removed after new compiler is widely used).

#pragma once

// alignof
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define ALIGNOF __alignof
#else
  #define ALIGNOF alignof
#endif

// constexpr
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define CONSTEXPR_VALUE const
#else
  #define CONSTEXPR_VALUE constexpr
#endif

// noexcept
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define NOEXCEPT_MODIFIER
#else
  #define NOEXCEPT_MODIFIER noexcept
#endif

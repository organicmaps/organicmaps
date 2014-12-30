#pragma once

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define CONSTEXPR_VALUE const
#else
  #define CONSTEXPR_VALUE constexpr
#endif

#pragma once

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define NOEXCEPT_MODIFIER
#else
  #define NOEXCEPT_MODIFIER noexcept
#endif

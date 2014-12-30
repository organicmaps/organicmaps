#pragma once

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
  #define ALIGNOF __alignof
#else
  #define ALIGNOF alignof
#endif

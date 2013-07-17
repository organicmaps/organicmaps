#pragma once

#include "defines.hpp"

namespace graphics
{
  static const int debugDepth = maxDepth;
  static const int benchmarkDepth = maxDepth;

  /// @todo 100 is a temporary solution fo iOS.
  /// Need to review logic of gui elements, glyphs and symbols caching
  /// (display_list_cache.dpp). Depth is hardcoded there.

  static const int balloonContentInc = 100;
  static const int balloonBaseDepth = maxDepth - balloonContentInc;

  static const int compassDepth = balloonBaseDepth - 10;
  static const int rulerDepth = compassDepth;
  static const int locationDepth = rulerDepth - 10;
  static const int poiAndBookmarkDepth = locationDepth - 10;
  static const int countryStatusDepth = poiAndBookmarkDepth - 10;
}

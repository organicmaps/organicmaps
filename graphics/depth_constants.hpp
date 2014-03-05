#pragma once

#include "defines.hpp"

namespace graphics
{
  static const int debugDepth = maxDepth;
  static const int benchmarkDepth = maxDepth;

  static const int compassDepth = maxDepth;
  static const int rulerDepth = maxDepth;
  static const int countryStatusDepth = maxDepth - 10;

  /// @todo 100 is a temporary solution fo iOS.
  /// Need to review logic of gui elements, glyphs and symbols caching
  /// (display_list_cache.dpp). Depth is hardcoded there.
  static const int balloonContentInc = 100;
  static const int balloonBaseDepth = countryStatusDepth - (balloonContentInc + 10);

  static const int locationDepth = balloonBaseDepth - 10;
  static const int activePinDepth = locationDepth - 10;
  static const int poiAndBookmarkDepth = activePinDepth - 10;
  static const int tracksDepth = poiAndBookmarkDepth - 10;
}

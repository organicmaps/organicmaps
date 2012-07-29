#pragma once

namespace yg
{
  enum DataFormat
  {
    Data8Bpp,
    Data4Bpp,
    Data565Bpp
  };

  /// getting size of the pixel in specified DataFormat.
  unsigned formatSize(DataFormat const & fmt);
  /// getting string representation of the specified DataFormat.
  char const * formatName(DataFormat const & fmt);
}

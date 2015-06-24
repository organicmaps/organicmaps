#pragma once

#include "std/cstdint.hpp"
#include "std/vector.hpp"

struct FrameImage
{
  // image data.
  // TopLeft-to-RightBottom order
  // Format - png
  vector<uint8_t> m_data;
  uint32_t m_width = 0;       // pixel width of image
  uint32_t m_height = 0;      // pixel height of image
  uint32_t m_stride = 0;      // row stride in bytes
};

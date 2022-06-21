#pragma once

#include "geometry/rect2d.hpp"

namespace dp
{

class RectPacker
{
public:
  explicit RectPacker(m2::PointU const & size) : m_size(size) {}

  bool Pack(uint32_t width, uint32_t height, m2::RectU & rect);
  bool CanBePacked(uint32_t glyphsCount, uint32_t width, uint32_t height) const;
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;
  bool IsFull() const { return m_isFull; }
  m2::PointU const & GetSize() const { return m_size; }

private:
  m2::PointU m_size;
  m2::PointU m_cursor{0, 0};
  uint32_t m_yStep = 0;
  bool m_isFull = false;
};

} // namespace dp

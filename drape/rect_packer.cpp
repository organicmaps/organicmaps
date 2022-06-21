#include "rect_packer.hpp"

namespace dp
{

bool RectPacker::Pack(uint32_t width, uint32_t height, m2::RectU & rect)
{
  ASSERT_LESS(width, m_size.x, ());
  ASSERT_LESS(height, m_size.y, ());

  if (m_cursor.x + width > m_size.x)
  {
    m_cursor.x = 0;
    m_cursor.y += m_yStep;
    m_yStep = 0;
  }

  if (m_cursor.y + height > m_size.y)
  {
    m_isFull = true;
    return false;
  }

  rect = m2::RectU(m_cursor.x, m_cursor.y,
                   m_cursor.x + width, m_cursor.y + height);

  m_cursor.x += width;
  m_yStep = std::max(height, m_yStep);
  return true;
}

bool RectPacker::CanBePacked(uint32_t glyphsCount, uint32_t width, uint32_t height) const
{
  uint32_t x = m_cursor.x;
  uint32_t y = m_cursor.y;
  uint32_t step = m_yStep;
  for (uint32_t i = 0; i < glyphsCount; i++)
  {
    if (x + width > m_size.x)
    {
      x = 0;
      y += step;
    }

    if (y + height > m_size.y)
      return false;

    x += width;
    step = std::max(height, step);
  }
  return true;
}

m2::RectF RectPacker::MapTextureCoords(const m2::RectU & pixelRect) const
{
  auto const width = static_cast<float>(m_size.x);
  auto const height = static_cast<float>(m_size.y);

  // Half-pixel offset to eliminate artifacts on fetching from texture.
  float offset = 0.0f;
  if (pixelRect.SizeX() != 0 && pixelRect.SizeY() != 0)
    offset = 0.5f;

  return {(pixelRect.minX() + offset) / width,
          (pixelRect.minY() + offset) / height,
          (pixelRect.maxX() - offset) / width,
          (pixelRect.maxY() - offset) / height};
}

} // namespace dp

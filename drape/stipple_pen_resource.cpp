#include "stipple_pen_resource.hpp"

#include "../std/numeric.hpp"

namespace dp
{

uint32_t const MAX_STIPPLE_PEN_LENGTH = 254;
uint32_t const COLUMN_WIDTH = MAX_STIPPLE_PEN_LENGTH + 1;

StipplePenPacker::StipplePenPacker(m2::PointU const & canvasSize)
  : m_canvasSize(canvasSize)
  , m_currentColumn(0)
{
  // canvasSize.x - 1 = we reserve 1 pixel border on left
  // to reduce problems with bilinear filtration on GPU
  // MAX_STIPPLE_PEN_LENGTH + 1 = we reserve 1 pixel empty space after pen resource
  // pen in last column reserve 1 pixel on right size of canvas
  uint32_t columnCount = floor((canvasSize.x - 1) / MAX_STIPPLE_PEN_LENGTH);
  m_columns.resize(columnCount, 0);
}

m2::RectU StipplePenPacker::PackResource(uint32_t width)
{
  ASSERT(m_currentColumn < m_columns.size(), ());
  uint32_t countInColumn = m_columns[m_currentColumn];
  // on one pattern we reserve 2 pixels. 1 pixel for pattern
  // and 1 pixel for empty space beetween patterns
  // also we reserve 1 pixel border on top of canvas
  uint32_t yOffset = countInColumn * 2 + 1;
  // ASSERT that ne pattern can be packed in current column
  ASSERT(yOffset + 2 < m_canvasSize.y, ());
  ++m_columns[m_currentColumn];
  // 1 + m_currentColumn = reserve 1 pixel border on left side
  uint32_t xOffset = 1 + m_currentColumn * COLUMN_WIDTH;
  // we check if new pattern can be mapped in this column
  // yOffset + 4 = 2 pixels on current pattern and 2 for new pattern
  if (yOffset + 4 > m_canvasSize.y)
    m_currentColumn++;

  // maxY = yOffset + 1 because real height of stipple pattern is 1
  return m2::RectU(xOffset, yOffset, xOffset + width, yOffset + 1);
}

StipplePenResource::StipplePenResource(StipplePenKey const & key)
  : m_key(key)
{
  uint32_t fullPattern = accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  ASSERT(fullPattern < MAX_STIPPLE_PEN_LENGTH, ());
  uint32_t count = floor(MAX_STIPPLE_PEN_LENGTH / fullPattern);
  m_pixelLength = count * fullPattern;
}

uint32_t StipplePenResource::GetSize() const
{
  return m_pixelLength;
}

uint32_t StipplePenResource::GetBufferSize() const
{
  return m_pixelLength;
}

TextureFormat StipplePenResource::GetExpectedFormat() const
{
  return ALPHA;
}

void StipplePenResource::Rasterize(void * buffer)
{
  uint8_t * pixels = static_cast<uint8_t *>(buffer);
  uint16_t offset = 0;
  for (size_t i = 0; i < m_key.m_pattern.size(); ++i)
  {
    uint8_t value = (i & 0x1) == 0 ? 255 : 0;
    uint8_t length = m_key.m_pattern[i];
    memset(pixels + offset, value, length * sizeof(uint8_t));
    offset += length;
  }

  uint8_t period = offset;

  while (offset < m_pixelLength)
  {
    memcpy(pixels + offset, pixels, period);
    offset += period;
  }
}

}


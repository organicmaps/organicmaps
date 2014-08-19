#include "stipple_pen_resource.hpp"

#include "texture.hpp"

#include "../base/shared_buffer_manager.hpp"

#include "../std/numeric.hpp"
#include "../std/sstream.hpp"

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
  // we devide on MAX_STIPPLE_PEN_LENGTH because this length considers 1 pixel border on right side
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

m2::RectF StipplePenPacker::MapTextureCoords(m2::RectU const & pixelRect) const
{
  return m2::RectF((pixelRect.minX() + 0.5f) / m_canvasSize.x,
                   (pixelRect.minY() + 0.5f) / m_canvasSize.y,
                   (pixelRect.maxX() - 0.5f) / m_canvasSize.x,
                   (pixelRect.maxY() - 0.5f) / m_canvasSize.y);
}

StipplePenHandle::StipplePenHandle(buffer_vector<uint8_t, 8> const & pattern)
  : m_keyValue(0)
{
  Init(pattern);
}

StipplePenHandle::StipplePenHandle(StipplePenKey const & info)
  : m_keyValue(0)
{
  Init(info.m_pattern);
}

void StipplePenHandle::Init(buffer_vector<uint8_t, 8> const & pattern)
{
  // encoding scheme
  // 63 - 61 bits = size of pattern in range [1 : 8]
  // 60 - 53 bits = first value of pattern in range [1 : 128]
  // 52 - 45 bits = second value of pattern
  // ....
  // 0 - 5 bits = reserved

  uint32_t patternSize = pattern.size();
  ASSERT(patternSize > 1, ());
  ASSERT(patternSize < 9, ());

  m_keyValue = patternSize - 1; // we code value 1 as 000 and value 8 as 111
  for (size_t i = 0; i < patternSize; ++i)
  {
    m_keyValue <<=7;
    ASSERT(pattern[i] > 0, ()); // we have 7 bytes for value. value = 1 encode like 0000000
    ASSERT(pattern[i] < 129, ()); // value = 128 encode like 1111111
    uint32_t value = pattern[i] - 1;
    m_keyValue += value;
  }

  m_keyValue <<= ((8 - patternSize) * 7 + 5);
}

StipplePenRasterizator::StipplePenRasterizator(StipplePenKey const & key)
  : m_key(key)
{
  uint32_t fullPattern = accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  ASSERT(fullPattern < MAX_STIPPLE_PEN_LENGTH, ());
  uint32_t count = floor(MAX_STIPPLE_PEN_LENGTH / fullPattern);
  m_pixelLength = count * fullPattern;
}

uint32_t StipplePenRasterizator::GetSize() const
{
  return m_pixelLength;
}

uint32_t StipplePenRasterizator::GetBufferSize() const
{
  return m_pixelLength;
}

void StipplePenRasterizator::Rasterize(void * buffer)
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

StipplePenResourceInfo const * StipplePenIndex::MapResource(StipplePenKey const & key)
{
  StipplePenHandle handle(key);
  TResourceMapping::const_iterator it = m_resourceMapping.find(handle);
  if (it != m_resourceMapping.end())
    return it->second.GetRaw();

  StipplePenRasterizator resource(key);
  m2::RectU pixelRect = m_packer.PackResource(resource.GetSize());
  m_pendingNodes.push_back(make_pair(pixelRect, resource));

  typedef pair<TResourceMapping::iterator, bool> TInsertionNode;
  MasterPointer<StipplePenResourceInfo> info(new StipplePenResourceInfo(m_packer.MapTextureCoords(pixelRect),
                                                             resource.GetSize()));
  TInsertionNode result = m_resourceMapping.insert(TResourceMapping::value_type(handle, info));
  ASSERT(result.second, ());
  return result.first->second.GetRaw();
}

void StipplePenIndex::UploadResources(RefPointer<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::ALPHA, ());
  if (m_pendingNodes.empty())
    return;

  buffer_vector<uint32_t, 5> ranges;
  ranges.push_back(0);

  uint32_t xOffset = m_pendingNodes[0].first.minX();
  for (size_t i = 1; i < m_pendingNodes.size(); ++i)
  {
    m2::RectU & node = m_pendingNodes[i].first;
#ifdef DEBUG
    ASSERT(xOffset <= node.minX(), ());
    if (xOffset == node.minX())
    {
      m2::RectU const & prevNode = m_pendingNodes[i - 1].first;
      ASSERT(prevNode.minY() < node.minY(), ());
    }
#endif
    if (node.minX() > xOffset)
      ranges.push_back(i);
    xOffset = node.minX();
  }

  ranges.push_back(m_pendingNodes.size());
  SharedBufferManager & mng = SharedBufferManager::instance();

  for (size_t i = 1; i < ranges.size(); ++i)
  {
    uint32_t rangeStart = ranges[i - 1];
    uint32_t rangeEnd = ranges[i];
    // rangeEnd - rangeStart give us count of patterns in this package
    // 2 * range - count of lines for patterns
    uint32_t lineCount = 2 * (rangeEnd - rangeStart);
    // MAX_STIPPLE_PEN_LENGTH * lineCount - byte count on all patterns
    uint32_t bufferSize = MAX_STIPPLE_PEN_LENGTH * lineCount;
    uint32_t reserveBufferSize = my::NextPowOf2(bufferSize);
    SharedBufferManager::shared_buffer_ptr_t ptr = mng.reserveSharedBuffer(reserveBufferSize);
    uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
    memset(rawBuffer, 0, reserveBufferSize);

    m2::RectU const & startNode = m_pendingNodes[rangeStart].first;
    uint32_t minX = startNode.minX();
    uint32_t minY = startNode.minY();
#ifdef DEBUG
    m2::RectU const & endNode = m_pendingNodes[rangeEnd - 1].first;
    ASSERT(endNode.maxY() + 1 == (minY + lineCount), ());
#endif

    for (size_t r = rangeStart; r < rangeEnd; ++r)
    {
      m_pendingNodes[r].second.Rasterize(rawBuffer);
      rawBuffer += 2 * MAX_STIPPLE_PEN_LENGTH;
    }

    rawBuffer = SharedBufferManager::GetRawPointer(ptr);
    texture->UploadData(minX, minY, MAX_STIPPLE_PEN_LENGTH, lineCount,
                        dp::ALPHA, MakeStackRefPointer<void>(rawBuffer));

    mng.freeSharedBuffer(reserveBufferSize, ptr);
  }

  m_pendingNodes.clear();
}

string DebugPrint(StipplePenHandle const & key)
{
  ostringstream out;
  out << "0x" << hex << key.m_keyValue;
  return out.str();
}

}


#include "drape/stipple_pen_resource.hpp"

#include "drape/texture.hpp"

#include "base/shared_buffer_manager.hpp"

#include "std/numeric.hpp"
#include "std/sstream.hpp"
#include "std/cstring.hpp"

namespace dp
{

uint32_t const MAX_STIPPLE_PEN_LENGTH = 254;
uint32_t const COLUMN_WIDTH = MAX_STIPPLE_PEN_LENGTH + 2;

StipplePenPacker::StipplePenPacker(m2::PointU const & canvasSize)
  : m_canvasSize(canvasSize)
  , m_currentColumn(0)
{
  uint32_t columnCount = floor(canvasSize.x / static_cast<float>(COLUMN_WIDTH));
  m_columns.resize(columnCount, 0);
}

m2::RectU StipplePenPacker::PackResource(uint32_t width)
{
  ASSERT(m_currentColumn < m_columns.size(), ());
  uint32_t countInColumn = m_columns[m_currentColumn];
  // 2 pixels height on pattern
  uint32_t yOffset = countInColumn * 2;
  // ASSERT that ne pattern can be packed in current column
  ASSERT(yOffset + 1 <= m_canvasSize.y, ());
  ++m_columns[m_currentColumn];
  // 1 + m_currentColumn = reserve 1 pixel border on left side
  uint32_t xOffset = m_currentColumn * COLUMN_WIDTH;
  // we check if new pattern can be mapped in this column
  // yOffset + 4 = 2 pixels on current pattern and 2 for new pattern
  if (yOffset + 4 > m_canvasSize.y)
    m_currentColumn++;

  return m2::RectU(xOffset, yOffset, xOffset + width + 2, yOffset + 2);
}

m2::RectF StipplePenPacker::MapTextureCoords(m2::RectU const & pixelRect) const
{
  return m2::RectF((pixelRect.minX() + 1.0f) / m_canvasSize.x,
                   (pixelRect.minY() + 1.0f) / m_canvasSize.y,
                   (pixelRect.maxX() - 1.0f) / m_canvasSize.x,
                   (pixelRect.maxY() - 1.0f) / m_canvasSize.y);
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
  // 63 - 61 bits = size of pattern in range [0 : 8]
  // 60 - 53 bits = first value of pattern in range [1 : 128]
  // 52 - 45 bits = second value of pattern
  // ....
  // 0 - 5 bits = reserved

  uint32_t patternSize = pattern.size();
  ASSERT(patternSize >= 1 && patternSize < 9, (patternSize));

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
  m_patternLength = accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  ASSERT(m_patternLength < MAX_STIPPLE_PEN_LENGTH, ());
  uint32_t count = floor(MAX_STIPPLE_PEN_LENGTH / m_patternLength);
  m_pixelLength = count * m_patternLength;
}

uint32_t StipplePenRasterizator::GetSize() const
{
  return m_pixelLength;
}

uint32_t StipplePenRasterizator::GetPatternSize() const
{
  return m_patternLength;
}

uint32_t StipplePenRasterizator::GetBufferSize() const
{
  return m_pixelLength;
}

void StipplePenRasterizator::Rasterize(void * buffer)
{
  ASSERT(!m_key.m_pattern.empty(), ());
  uint8_t * pixels = static_cast<uint8_t *>(buffer);
  uint16_t offset = 1;
  buffer_vector<uint8_t, 8> pattern = m_key.m_pattern;
  for (size_t i = 0; i < pattern.size(); ++i)
  {
    uint8_t value = (i & 0x1) == 0 ? 255 : 0;
    uint8_t length = m_key.m_pattern[i];
    memset(pixels + offset, value, length * sizeof(uint8_t));
    offset += length;
  }

  uint8_t period = offset - 1;

  while (offset < m_pixelLength + 1)
  {
    memcpy(pixels + offset, pixels + 1, period);
    offset += period;
  }

  pixels[0] = pixels[1];
  pixels[offset] = pixels[offset - 1];

  memcpy(pixels + COLUMN_WIDTH, pixels, COLUMN_WIDTH);
}

RefPointer<Texture::ResourceInfo> StipplePenIndex::MapResource(StipplePenKey const & key, bool & newResource)
{
  newResource = false;
  StipplePenHandle handle(key);
  TResourceMapping::iterator it = m_resourceMapping.find(handle);
  if (it != m_resourceMapping.end())
    return MakeStackRefPointer<Texture::ResourceInfo>(&it->second);

  newResource = true;

  StipplePenRasterizator resource(key);
  m2::RectU pixelRect = m_packer.PackResource(resource.GetSize());
  {
    threads::MutexGuard g(m_lock);
    m_pendingNodes.push_back(make_pair(pixelRect, resource));
  }

  auto res = m_resourceMapping.emplace(handle, StipplePenResourceInfo(m_packer.MapTextureCoords(pixelRect),
                                                                      resource.GetSize(),
                                                                      resource.GetPatternSize()));
  ASSERT(res.second, ());
  return MakeStackRefPointer<Texture::ResourceInfo>(&res.first->second);
}

void StipplePenIndex::UploadResources(RefPointer<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::ALPHA, ());
  if (m_pendingNodes.empty())
    return;

  TPendingNodes pendingNodes;
  {
    threads::MutexGuard g(m_lock);
    m_pendingNodes.swap(pendingNodes);
  }

  buffer_vector<uint32_t, 5> ranges;
  ranges.push_back(0);

  uint32_t xOffset = pendingNodes[0].first.minX();
  for (size_t i = 1; i < pendingNodes.size(); ++i)
  {
    m2::RectU & node = pendingNodes[i].first;
#ifdef DEBUG
    ASSERT(xOffset <= node.minX(), ());
    if (xOffset == node.minX())
    {
      m2::RectU const & prevNode = pendingNodes[i - 1].first;
      ASSERT(prevNode.minY() < node.minY(), ());
    }
#endif
    if (node.minX() > xOffset)
      ranges.push_back(i);
    xOffset = node.minX();
  }

  ranges.push_back(pendingNodes.size());
  SharedBufferManager & mng = SharedBufferManager::instance();

  for (size_t i = 1; i < ranges.size(); ++i)
  {
    uint32_t rangeStart = ranges[i - 1];
    uint32_t rangeEnd = ranges[i];
    // rangeEnd - rangeStart give us count of patterns in this package
    // 2 * range - count of lines for patterns
    uint32_t lineCount = 2 * (rangeEnd - rangeStart);
    // MAX_STIPPLE_PEN_LENGTH * lineCount - byte count on all patterns
    uint32_t bufferSize = COLUMN_WIDTH * lineCount;
    uint32_t reserveBufferSize = my::NextPowOf2(bufferSize);
    SharedBufferManager::shared_buffer_ptr_t ptr = mng.reserveSharedBuffer(reserveBufferSize);
    uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
    memset(rawBuffer, 0, reserveBufferSize);

    m2::RectU const & startNode = pendingNodes[rangeStart].first;
    uint32_t minX = startNode.minX();
    uint32_t minY = startNode.minY();
#ifdef DEBUG
    m2::RectU const & endNode = pendingNodes[rangeEnd - 1].first;
    ASSERT(endNode.maxY() == (minY + lineCount), ());
#endif

    for (size_t r = rangeStart; r < rangeEnd; ++r)
    {
      pendingNodes[r].second.Rasterize(rawBuffer);
      rawBuffer += 2 * COLUMN_WIDTH;
    }

    rawBuffer = SharedBufferManager::GetRawPointer(ptr);
    texture->UploadData(minX, minY, COLUMN_WIDTH, lineCount,
                        dp::ALPHA, MakeStackRefPointer<void>(rawBuffer));

    mng.freeSharedBuffer(reserveBufferSize, ptr);
  }
}

glConst StipplePenIndex::GetMinFilter() const
{
  return gl_const::GLNearest;
}

glConst StipplePenIndex::GetMagFilter() const
{
  return gl_const::GLNearest;
}

string DebugPrint(StipplePenHandle const & key)
{
  ostringstream out;
  out << "0x" << hex << key.m_keyValue;
  return out.str();
}

}


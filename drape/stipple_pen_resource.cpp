#include "drape/stipple_pen_resource.hpp"

#include "drape/texture.hpp"

#include "base/shared_buffer_manager.hpp"

#include <cstring>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace dp
{
uint32_t const kMaxStipplePenLength = 512;
uint32_t const kStippleHeight = 1;

StipplePenPacker::StipplePenPacker(m2::PointU const & canvasSize)
  : m_canvasSize(canvasSize)
  , m_currentRow(0)
{
  ASSERT_GREATER_OR_EQUAL(canvasSize.x, kMaxStipplePenLength, ());
}

m2::RectU StipplePenPacker::PackResource(uint32_t width)
{
  ASSERT_LESS(m_currentRow, m_canvasSize.y, ());
  ASSERT_LESS_OR_EQUAL(width, m_canvasSize.x, ());
  uint32_t yOffset = m_currentRow;
  m_currentRow += kStippleHeight;
  return m2::RectU(0, yOffset, width, yOffset + kStippleHeight);
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
  // 63 - 61 bits = size of pattern in range [0 : 8]
  // 60 - 53 bits = first value of pattern in range [1 : 128]
  // 52 - 45 bits = second value of pattern
  // ....
  // 0 - 5 bits = reserved

  ASSERT(pattern.size() >= 1 && pattern.size() < 9, (pattern.size()));
  uint32_t const patternSize = static_cast<uint32_t>(pattern.size());

  m_keyValue = patternSize - 1; // we code value 1 as 000 and value 8 as 111
  for (size_t i = 0; i < patternSize; ++i)
  {
    m_keyValue <<=7;
    ASSERT_GREATER(pattern[i], 0, ()); // we have 7 bytes for value. value = 1 encode like 0000000
    ASSERT_LESS(pattern[i], 129, ()); // value = 128 encode like 1111111
    uint32_t value = pattern[i] - 1;
    m_keyValue += value;
  }

  m_keyValue <<= ((8 - patternSize) * 7 + 5);
}

StipplePenRasterizator::StipplePenRasterizator(StipplePenKey const & key)
  : m_key(key)
{
  m_patternLength = std::accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  uint32_t const availableSize = kMaxStipplePenLength - 2; // the first and the last pixel reserved
  ASSERT_LESS(m_patternLength, availableSize, ());
  uint32_t const count = floor(availableSize / m_patternLength);
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

  ASSERT_LESS(offset, kMaxStipplePenLength, ());

  pixels[0] = pixels[1];
  pixels[offset] = pixels[offset - 1];
}

ref_ptr<Texture::ResourceInfo> StipplePenIndex::ReserveResource(bool predefined, StipplePenKey const & key,
                                                                bool & newResource)
{
  std::lock_guard<std::mutex> g(m_mappingLock);

  newResource = false;
  StipplePenHandle handle(key);
  TResourceMapping & resourceMapping = predefined ? m_predefinedResourceMapping : m_resourceMapping;
  TResourceMapping::iterator it = resourceMapping.find(handle);
  if (it != resourceMapping.end())
    return make_ref(&it->second);

  newResource = true;

  StipplePenRasterizator resource(key);
  m2::RectU pixelRect = m_packer.PackResource(resource.GetSize());
  {
    std::lock_guard<std::mutex> g(m_lock);
    m_pendingNodes.push_back(std::make_pair(pixelRect, resource));
  }

  auto res = resourceMapping.emplace(handle, StipplePenResourceInfo(m_packer.MapTextureCoords(pixelRect),
                                                                    resource.GetSize(),
                                                                    resource.GetPatternSize()));
  ASSERT(res.second, ());
  return make_ref(&res.first->second);
}

ref_ptr<Texture::ResourceInfo> StipplePenIndex::MapResource(StipplePenKey const & key, bool & newResource)
{
  StipplePenHandle handle(key);
  TResourceMapping::iterator it = m_predefinedResourceMapping.find(handle);
  if (it != m_predefinedResourceMapping.end())
  {
    newResource = false;
    return make_ref(&it->second);
  }

  return ReserveResource(false /* predefined */, key, newResource);
}

void StipplePenIndex::UploadResources(ref_ptr<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::TextureFormat::Alpha, ());
  TPendingNodes pendingNodes;
  {
    std::lock_guard<std::mutex> g(m_lock);
    if (m_pendingNodes.empty())
      return;
    m_pendingNodes.swap(pendingNodes);
  }

  SharedBufferManager & mng = SharedBufferManager::instance();
  uint32_t const bytesPerNode = kMaxStipplePenLength * kStippleHeight;
  uint32_t reserveBufferSize = my::NextPowOf2(static_cast<uint32_t>(pendingNodes.size()) * bytesPerNode);
  SharedBufferManager::shared_buffer_ptr_t ptr = mng.reserveSharedBuffer(reserveBufferSize);
  uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
  memset(rawBuffer, 0, reserveBufferSize);
  for (size_t i = 0; i < pendingNodes.size(); ++i)
    pendingNodes[i].second.Rasterize(rawBuffer + i * bytesPerNode);

  texture->UploadData(0, pendingNodes.front().first.minY(), kMaxStipplePenLength,
                      static_cast<uint32_t>(pendingNodes.size()) * kStippleHeight, make_ref(rawBuffer));

  mng.freeSharedBuffer(reserveBufferSize, ptr);
}

void StipplePenTexture::ReservePattern(buffer_vector<uint8_t, 8> const & pattern)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, StipplePenKey(pattern), newResource);
}

std::string DebugPrint(StipplePenHandle const & key)
{
  std::ostringstream out;
  out << "0x" << std::hex << key.m_keyValue;
  return out.str();
}
}  // namespace dp

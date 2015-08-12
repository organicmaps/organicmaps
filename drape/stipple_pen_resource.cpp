#include "drape/stipple_pen_resource.hpp"

#include "drape/texture.hpp"

#include "base/shared_buffer_manager.hpp"

#include "std/numeric.hpp"
#include "std/sstream.hpp"
#include "std/cstring.hpp"

namespace dp
{

uint32_t const MAX_STIPPLE_PEN_LENGTH = 512;
uint32_t const STIPPLE_HEIGHT = 1;

StipplePenPacker::StipplePenPacker(m2::PointU const & canvasSize)
  : m_canvasSize(canvasSize)
  , m_currentRow(0)
{
  ASSERT_LESS_OR_EQUAL(canvasSize.x, MAX_STIPPLE_PEN_LENGTH, ());
}

m2::RectU StipplePenPacker::PackResource(uint32_t width)
{
  ASSERT_LESS(m_currentRow, m_canvasSize.y, ());
  ASSERT_LESS_OR_EQUAL(width, m_canvasSize.x, ());
  uint32_t yOffset = m_currentRow;
  m_currentRow += STIPPLE_HEIGHT;
  return m2::RectU(0, yOffset, width, yOffset + STIPPLE_HEIGHT);
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

  uint32_t patternSize = pattern.size();
  ASSERT(patternSize >= 1 && patternSize < 9, (patternSize));

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
  m_patternLength = accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  ASSERT_LESS(m_patternLength, MAX_STIPPLE_PEN_LENGTH, ());
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

  memcpy(pixels + MAX_STIPPLE_PEN_LENGTH, pixels, MAX_STIPPLE_PEN_LENGTH);
}

ref_ptr<Texture::ResourceInfo> StipplePenIndex::ReserveResource(bool predefined, StipplePenKey const & key, bool & newResource)
{
  lock_guard<mutex> g(m_mappingLock);

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
    lock_guard<mutex> g(m_lock);
    m_pendingNodes.push_back(make_pair(pixelRect, resource));
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
  ASSERT(texture->GetFormat() == dp::ALPHA, ());
  if (m_pendingNodes.empty())
    return;

  TPendingNodes pendingNodes;
  {
    lock_guard<mutex> g(m_lock);
    m_pendingNodes.swap(pendingNodes);
  }

  SharedBufferManager & mng = SharedBufferManager::instance();
  uint32_t const bytesPerNode = MAX_STIPPLE_PEN_LENGTH * STIPPLE_HEIGHT;
  uint32_t reserveBufferSize = my::NextPowOf2(pendingNodes.size() * bytesPerNode);
  SharedBufferManager::shared_buffer_ptr_t ptr = mng.reserveSharedBuffer(reserveBufferSize);
  uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
  memset(rawBuffer, 0, reserveBufferSize);
  for (size_t i = 0; i < pendingNodes.size(); ++i)
    pendingNodes[i].second.Rasterize(rawBuffer + i * bytesPerNode);

  texture->UploadData(0, pendingNodes.front().first.minY(),
                      MAX_STIPPLE_PEN_LENGTH, pendingNodes.size() * STIPPLE_HEIGHT,
                      dp::ALPHA, make_ref(rawBuffer));

  mng.freeSharedBuffer(reserveBufferSize, ptr);
}

glConst StipplePenIndex::GetMinFilter() const
{
  return gl_const::GLNearest;
}

glConst StipplePenIndex::GetMagFilter() const
{
  return gl_const::GLNearest;
}

void StipplePenTexture::ReservePattern(buffer_vector<uint8_t, 8> const & pattern)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, StipplePenKey(pattern), newResource);
}

string DebugPrint(StipplePenHandle const & key)
{
  ostringstream out;
  out << "0x" << hex << key.m_keyValue;
  return out.str();
}

}


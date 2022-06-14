#include "drape/stipple_pen_resource.hpp"

#include "drape/texture.hpp"

#include "base/shared_buffer_manager.hpp"

#include <cstring>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace dp
{
uint32_t const kMaxStipplePenLength = 512;  /// @todo Should be equal with kStippleTextureWidth?
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

StipplePenRasterizator::StipplePenRasterizator(StipplePenKey const & key)
  : m_key(key)
{
  uint32_t const patternLength = std::accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
  uint32_t const availableSize = kMaxStipplePenLength - 2; // the first and the last pixel reserved
  ASSERT(patternLength > 0 && patternLength < availableSize, (patternLength, availableSize));
  uint32_t const count = floor(availableSize / patternLength);
  m_pixelLength = count * patternLength;
}

uint32_t StipplePenRasterizator::GetSize() const
{
  return m_pixelLength;
}

void StipplePenRasterizator::Rasterize(uint8_t * pixels)
{
  ASSERT(!m_key.m_pattern.empty(), ());
  uint16_t offset = 1;
  for (size_t i = 0; i < m_key.m_pattern.size(); ++i)
  {
    uint8_t const value = (i & 0x1) == 0 ? 255 : 0;
    uint8_t const length = m_key.m_pattern[i];
    memset(pixels + offset, value, length);
    offset += length;
  }

  // clone pattern
  uint32_t const patternLength = offset - 1;
  while (offset < m_pixelLength + 1)
  {
    memcpy(pixels + offset, pixels + 1, patternLength);
    offset += patternLength;
  }

  ASSERT_LESS(offset, kMaxStipplePenLength, ());

  pixels[0] = pixels[1];
  pixels[offset] = pixels[offset - 1];
}

ref_ptr<Texture::ResourceInfo> StipplePenIndex::ReserveResource(bool predefined, StipplePenKey const & key,
                                                                bool & newResource)
{
  TResourceMapping & resourceMapping = predefined ? m_predefinedResourceMapping : m_resourceMapping;
  auto it = resourceMapping.find(key);
  if (it != resourceMapping.end())
  {
    newResource = false;
    return make_ref(&it->second);
  }
  newResource = true;

  StipplePenRasterizator resource(key);
  m2::RectU const pixelRect = m_packer.PackResource(resource.GetSize());
  {
    std::lock_guard<std::mutex> g(m_lock);
    m_pendingNodes.emplace_back(pixelRect, resource);
  }

  auto res = resourceMapping.emplace(key, StipplePenResourceInfo(m_packer.MapTextureCoords(pixelRect),
                                                                 resource.GetSize()));
  ASSERT(res.second, ());
  return make_ref(&res.first->second);
}

ref_ptr<Texture::ResourceInfo> StipplePenIndex::MapResource(StipplePenKey const & key, bool & newResource)
{
  auto it = m_predefinedResourceMapping.find(key);
  if (it != m_predefinedResourceMapping.end())
  {
    newResource = false;
    return make_ref(&it->second);
  }

  std::lock_guard<std::mutex> g(m_mappingLock);
  return ReserveResource(false /* predefined */, key, newResource);
}

void StipplePenIndex::UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::TextureFormat::Alpha, ());
  TPendingNodes pendingNodes;
  {
    std::lock_guard<std::mutex> g(m_lock);
    if (m_pendingNodes.empty())
      return;
    m_pendingNodes.swap(pendingNodes);
  }

  uint32_t const nodesCount = static_cast<uint32_t>(pendingNodes.size());
  uint32_t const bytesPerNode = kMaxStipplePenLength * kStippleHeight;
  uint32_t const reserveBufferSize = base::NextPowOf2(nodesCount * bytesPerNode);

  SharedBufferManager & mng = SharedBufferManager::instance();
  SharedBufferManager::shared_buffer_ptr_t ptr = mng.reserveSharedBuffer(reserveBufferSize);
  uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
  memset(rawBuffer, 0, reserveBufferSize);

  for (uint32_t i = 0; i < nodesCount; ++i)
    pendingNodes[i].second.Rasterize(rawBuffer + i * bytesPerNode);

  texture->UploadData(context, 0, pendingNodes.front().first.minY(), kMaxStipplePenLength,
                      nodesCount * kStippleHeight, make_ref(rawBuffer));

  mng.freeSharedBuffer(reserveBufferSize, ptr);
}

void StipplePenTexture::ReservePattern(PenPatternT const & pattern)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, StipplePenKey(pattern), newResource);
}
}  // namespace dp

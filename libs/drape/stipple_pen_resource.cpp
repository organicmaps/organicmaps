#include "drape/stipple_pen_resource.hpp"

#include "drape/texture.hpp"

#include "base/logging.hpp"
#include "base/shared_buffer_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace dp
{
StipplePenPacker::StipplePenPacker(m2::PointU const & canvasSize) : m_canvasSize(canvasSize), m_currentRow(0)
{
  ASSERT_GREATER_OR_EQUAL(canvasSize.x, kMaxStipplePenLength, ());
}

m2::RectU StipplePenPacker::PackResource(m2::PointU const & size)
{
  ASSERT_LESS(m_currentRow, m_canvasSize.y, ());
  ASSERT_LESS_OR_EQUAL(size.x, m_canvasSize.x, ());
  uint32_t const yOffset = m_currentRow;
  m_currentRow += size.y;
  return m2::RectU(0, yOffset, size.x, m_currentRow);
}

m2::RectF StipplePenPacker::MapTextureCoords(m2::RectU const & pixelRect) const
{
  return {(pixelRect.minX() + 0.5f) / m_canvasSize.x, (pixelRect.minY() + 0.5f) / m_canvasSize.y,
          (pixelRect.maxX() - 0.5f) / m_canvasSize.x, (pixelRect.maxY() - 0.5f) / m_canvasSize.y};
}

StipplePenRasterizator::StipplePenRasterizator(StipplePenKey const & key) : m_key(key)
{
  if (IsTrianglePattern(m_key.m_pattern))
  {
    m_patternLength = 2 * m_key.m_pattern[0] + m_key.m_pattern[1];
    m_height = m_key.m_pattern[2] + m_key.m_pattern[3];
  }
  else
  {
    m_patternLength = std::accumulate(m_key.m_pattern.begin(), m_key.m_pattern.end(), 0);
    m_height = kStipplePenDashHeight;
  }

  uint32_t const availableSize = kMaxStipplePenLength - 2;  // the first and the last pixel reserved
  ASSERT(m_patternLength > 0 && m_patternLength < availableSize, (m_patternLength, availableSize));
  uint32_t const count = floor(availableSize / m_patternLength);
  m_pixelLength = count * m_patternLength;
}

void StipplePenRasterizator::Rasterize(uint8_t * buffer) const
{
  if (IsTrianglePattern(m_key.m_pattern))
    RasterizeTriangle(buffer);
  else
    RasterizeDash(buffer);
}

void StipplePenRasterizator::RasterizeDash(uint8_t * pixels) const
{
  ASSERT(!m_key.m_pattern.empty() && m_key.m_pattern.size() <= 2, (m_key.m_pattern.size()));

  uint32_t const dashLen = m_key.m_pattern[0];
  float const radius = std::min(kStipplePenDashRoundRadius, static_cast<float>(dashLen) / 2.0f);
  float const halfHeight = static_cast<float>(m_height) / 2.0f;

  // Rasterize the first period of each row, then clone it across the mask.
  // A dash is a capsule: full height in the middle, tapering to a rounded tip
  // at both ends (an ellipse with X radius 'radius' and Y radius 'halfHeight').
  for (uint32_t y = 0; y < m_height; ++y)
  {
    uint8_t * row = pixels + y * kMaxStipplePenLength;
    float const dy = (static_cast<float>(y) + 0.5f) - halfHeight;

    for (uint32_t x = 0; x < m_patternLength; ++x)
    {
      uint8_t value = 0;
      if (x < dashLen)
      {
        bool on = true;
        if (x < radius || x >= dashLen - radius)
        {
          float const cx = (x < radius) ? radius : static_cast<float>(dashLen) - radius;
          float const dx = (static_cast<float>(x) + 0.5f) - cx;
          float const nx = dx / radius;
          float const ny = dy / halfHeight;
          on = nx * nx + ny * ny <= 1.0f;
        }
        if (on)
          value = 255;
      }
      row[x + 1] = value;
    }

    // Clone the period across the row (with a wrap-around pixel for seamless tiling).
    uint32_t offset = m_patternLength + 1;
    while (offset < m_pixelLength + 1)
    {
      memcpy(row + offset, row + 1, m_patternLength);
      offset += m_patternLength;
    }
    ASSERT_EQUAL(offset, m_pixelLength + 1, ());

    row[0] = row[1];
    row[m_pixelLength + 1] = row[m_pixelLength];
  }
}

void StipplePenRasterizator::ClonePattern(uint8_t * pixels) const
{
  uint32_t offset = m_patternLength + 1;
  while (offset < m_pixelLength + 1)
  {
    memcpy(pixels + offset, pixels + 1, m_patternLength);
    offset += m_patternLength;
  }

  ASSERT_EQUAL(offset, m_pixelLength + 1, ());

  pixels[0] = pixels[1];
  pixels[offset] = pixels[offset - 1];
}

void StipplePenRasterizator::RasterizeTriangle(uint8_t * pixels) const
{
  // 4 values: dash (===), triangle base (tb), triangle height, base height.
  // Triangle should go on the right.
  // ===\tb /===  - base height
  //     \/     | - triangle height

  uint8_t baseH = m_key.m_pattern[3];
  ASSERT(baseH > 0, ());

  while (baseH > 0)
  {
    memset(pixels, 255, m_pixelLength + 2);

    pixels += kMaxStipplePenLength;
    --baseH;
  }

  uint8_t trgH = m_key.m_pattern[2];
  ASSERT(trgH > 0, ());
  double const tan = m_key.m_pattern[1] / double(trgH);
  ASSERT(tan > 0, ());

  while (trgH > 0)
  {
    uint8_t const base = std::lround(trgH * tan);
    uint32_t const left = (m_patternLength - base) / 2;
    memset(pixels + 1, 0, left);
    memset(pixels + left + 1, 255, base);
    memset(pixels + left + 1 + base, 0, m_patternLength - left - base);

    ClonePattern(pixels);

    pixels += kMaxStipplePenLength;
    --trgH;
  }
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

  auto res =
      resourceMapping.emplace(key, StipplePenResourceInfo(m_packer.MapTextureCoords(pixelRect), resource.GetSize()));
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
  ASSERT(texture->GetFormat() == dp::TextureFormat::Red, ());
  TPendingNodes pendingNodes;
  {
    std::lock_guard<std::mutex> g(m_lock);
    if (m_pendingNodes.empty())
      return;
    m_pendingNodes.swap(pendingNodes);
  }

  // Assume that all patterns are initialized when creating texture (ReserveResource) and uploaded once.
  // Should provide additional logic like in ColorPalette::UploadResources, if we want multiple uploads.
  // TODO: https://github.com/organicmaps/organicmaps/issues/4539
  //  if (m_uploadCalled)
  //    LOG(LERROR, ("Multiple stipple pen texture uploads are not supported"));
  m_uploadCalled = true;

  size_t height = 0;
  for (auto const & n : pendingNodes)
    height += n.second.GetSize().y;

  SharedBufferManager & mng = SharedBufferManager::Instance();
  // Rounds up the requested size to the nearest power of 2.
  auto ptr = mng.ReserveSharedBuffer(height * kMaxStipplePenLength);
  uint8_t * rawBuffer = SharedBufferManager::GetRawPointer(ptr);
  memset(rawBuffer, 0, ptr->size());

  uint8_t * pixels = rawBuffer;
  for (auto const & n : pendingNodes)
  {
    n.second.Rasterize(pixels);
    pixels += (kMaxStipplePenLength * n.second.GetSize().y);
  }

  texture->UploadData(context, 0, pendingNodes.front().first.minY(), kMaxStipplePenLength, height, make_ref(rawBuffer));
  // ptr's deleter returns the pooled buffer on scope exit.
}

void StipplePenTexture::ReservePattern(PenPatternT const & pattern)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, StipplePenKey(pattern), newResource);
}
}  // namespace dp

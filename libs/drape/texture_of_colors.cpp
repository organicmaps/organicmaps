#include "drape/texture_of_colors.hpp"

#include "base/logging.hpp"
#include "base/shared_buffer_manager.hpp"

#include <cstring>  // memset

namespace dp
{

namespace
{
int const kResourceSize = 2;
int const kBytesPerPixel = 4;
}  // namespace

ColorPalette::ColorPalette(m2::PointU const & canvasSize) : m_textureSize(canvasSize), m_cursor(m2::PointU::Zero()) {}

ref_ptr<Texture::ResourceInfo> ColorPalette::ReserveResource(bool predefined, ColorKey const & key, bool & newResource)
{
  TPalette & palette = predefined ? m_predefinedPalette : m_palette;
  auto itm = palette.find(key.m_color);
  newResource = (itm == palette.end());
  if (newResource)
  {
    PendingColor pendingColor;
    pendingColor.m_color = key.m_color;
    pendingColor.m_rect = m2::RectU(m_cursor.x, m_cursor.y, m_cursor.x + kResourceSize, m_cursor.y + kResourceSize);
    {
      std::lock_guard<std::mutex> g(m_lock);
      m_pendingNodes.push_back(pendingColor);
    }

    m_cursor.x += kResourceSize;
    if (m_cursor.x >= m_textureSize.x)
    {
      m_cursor.y += kResourceSize;
      m_cursor.x = 0;

      ASSERT(m_cursor.y < m_textureSize.y, ());
    }

    float const sizeX = static_cast<float>(m_textureSize.x);
    float const sizeY = static_cast<float>(m_textureSize.y);
    m2::PointF const resCenter = m2::RectF(pendingColor.m_rect).Center();
    float const x = resCenter.x / sizeX;
    float const y = resCenter.y / sizeY;
    auto res = palette.emplace(key.m_color, ColorResourceInfo(m2::RectF(x, y, x, y)));
    ASSERT(res.second, ());
    itm = res.first;
  }

  return make_ref(&itm->second);
}

ref_ptr<Texture::ResourceInfo> ColorPalette::MapResource(ColorKey const & key, bool & newResource)
{
  auto itm = m_predefinedPalette.find(key.m_color);
  if (itm != m_predefinedPalette.end())
  {
    newResource = false;
    return make_ref(&itm->second);
  }

  std::lock_guard<std::mutex> lock(m_mappingLock);
  return ReserveResource(false /* predefined */, key, newResource);
}

void ColorPalette::UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::TextureFormat::RGBA8, ());
  bool const hasPartialTextureUpdate = context->HasPartialTextureUpdates();

  std::vector<PendingColor> pendingNodes;
  {
    std::lock_guard<std::mutex> g(m_lock);
    if (m_pendingNodes.empty())
      return;

    if (hasPartialTextureUpdate)
    {
      pendingNodes.swap(m_pendingNodes);
    }
    else
    {
      // Store all colors in m_nodes, because we should update *whole* texture, if partial update is not available.
      m_nodes.insert(m_nodes.end(), m_pendingNodes.begin(), m_pendingNodes.end());
      m_pendingNodes.clear();
      pendingNodes = m_nodes;
    }
  }

  if (!hasPartialTextureUpdate)
  {
    PendingColor lastPendingColor = pendingNodes.back();
    lastPendingColor.m_color = Color::Transparent();
    while (lastPendingColor.m_rect.maxX() < m_textureSize.x)
    {
      lastPendingColor.m_rect.Offset(kResourceSize, 0);
      pendingNodes.push_back(lastPendingColor);
    }
  }

  buffer_vector<size_t, 4> ranges;
  ranges.push_back(0);

  if (hasPartialTextureUpdate)
  {
    // Split into ranges by row wraps (when minX decreases).
    uint32_t minX = pendingNodes[0].m_rect.minX();
    for (size_t i = 1; i < pendingNodes.size(); ++i)
    {
      m2::RectU const & currentRect = pendingNodes[i].m_rect;
      if (minX > currentRect.minX())
      {
        ranges.push_back(i);
        minX = currentRect.minX();
      }
    }
  }
  // For !hasPartialTextureUpdate: single range covering all accumulated m_nodes.

  ranges.push_back(pendingNodes.size());
  for (size_t i = 1; i < ranges.size(); ++i)
  {
    size_t startRange = ranges[i - 1];
    size_t endRange = ranges[i];

    m2::RectU const & startRect = pendingNodes[startRange].m_rect;
    m2::RectU const & endRect = pendingNodes[endRange - 1].m_rect;

    m2::RectU uploadRect;
    if (startRect.minY() == endRect.minY() && startRect.maxY() == endRect.maxY())
    {
      uploadRect = m2::RectU(startRect.minX(), startRect.minY(), endRect.maxX(), endRect.maxY());
    }
    else
    {
      ASSERT(startRect.minX() == 0, ());
      uploadRect = m2::RectU(0, startRect.minY(), m_textureSize.x, endRect.maxY());
    }

    size_t const pixelStride = uploadRect.SizeX();
    size_t const byteStride = pixelStride * kBytesPerPixel;
    size_t const byteCount = byteStride * uploadRect.SizeY();
    // Scales up the buffer size to the nearest power of 2.
    auto buffer = SharedBufferManager::Instance().ReserveSharedBuffer(byteCount);
    uint8_t * basePtr = SharedBufferManager::GetRawPointer(buffer);
    // Zero the buffer so gaps (partially filled rows, padding) are transparent.
    memset(basePtr, 0, byteCount);

    uint32_t const originX = uploadRect.minX();
    uint32_t const originY = uploadRect.minY();
    for (size_t j = startRange; j < endRange; ++j)
    {
      PendingColor const & c = pendingNodes[j];

      // Compute absolute position of this color block within the upload buffer.
      uint32_t const localX = c.m_rect.minX() - originX;
      uint32_t const localY = c.m_rect.minY() - originY;
      size_t const blockOffset = localY * byteStride + localX * kBytesPerPixel;

      uint8_t const red = c.m_color.GetRed();
      uint8_t const green = c.m_color.GetGreen();
      uint8_t const blue = c.m_color.GetBlue();
      uint8_t const alpha = c.m_color.GetAlpha();

      for (size_t row = 0; row < kResourceSize; row++)
      {
        for (size_t column = 0; column < kResourceSize; column++)
        {
          size_t const idx = blockOffset + row * byteStride + column * kBytesPerPixel;
          ASSERT_LESS(idx + 3, byteCount, ());
          basePtr[idx] = red;
          basePtr[idx + 1] = green;
          basePtr[idx + 2] = blue;
          basePtr[idx + 3] = alpha;
        }
      }
    }

    texture->UploadData(context, originX, originY, uploadRect.SizeX(), uploadRect.SizeY(), make_ref(basePtr));

    SharedBufferManager::Instance().FreeSharedBuffer(std::move(buffer));
  }
}

bool ColorPalette::ReserveStrip(RainbowColors const & colors, m2::PointF & firstCenter, m2::PointF & lastCenter)
{
  ASSERT(!colors.empty(), ());

  // Synchronize with MapResource which also modifies m_cursor and m_pendingNodes.
  std::lock_guard lock(m_mappingLock);

  // Return cached strip if the same color combination was already allocated.
  auto const it = m_stripCache.find(colors);
  if (it != m_stripCache.end())
  {
    firstCenter = it->second.m_firstCenter;
    lastCenter = it->second.m_lastCenter;
    return true;
  }

  int const n = static_cast<int>(colors.size());

  // Ensure the strip fits in the current row; advance to next row if needed.
  if (m_cursor.x + n * kResourceSize > m_textureSize.x)
  {
    m_cursor.y += kResourceSize;
    m_cursor.x = 0;
  }

  if (m_cursor.y + kResourceSize > m_textureSize.y)
  {
    LOG(LERROR, ("Color atlas full, cannot allocate rainbow strip of", n, "colors"));
    return false;
  }

  float const sizeX = static_cast<float>(m_textureSize.x);
  float const sizeY = static_cast<float>(m_textureSize.y);

  {
    // Lock once for the entire strip to keep colors contiguous in m_pendingNodes.
    std::lock_guard g(m_lock);
    for (int i = 0; i < n; ++i)
    {
      PendingColor pendingColor;
      pendingColor.m_color = colors[i];
      pendingColor.m_rect = m2::RectU(m_cursor.x, m_cursor.y, m_cursor.x + kResourceSize, m_cursor.y + kResourceSize);

      m2::PointF const center = m2::RectF(pendingColor.m_rect).Center();
      m2::PointF const uv(center.x / sizeX, center.y / sizeY);
      if (i == 0)
        firstCenter = uv;
      if (i == n - 1)
        lastCenter = uv;

      m_pendingNodes.push_back(pendingColor);

      m_cursor.x += kResourceSize;
    }
  }

  // Wrap to next row if we've reached the edge (same logic as ReserveResource).
  if (m_cursor.x >= m_textureSize.x)
  {
    m_cursor.y += kResourceSize;
    m_cursor.x = 0;
  }

  m_stripCache[colors] = {firstCenter, lastCenter};
  return true;
}

void ColorTexture::ReserveColor(dp::Color const & color)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, ColorKey(color), newResource);
}

bool ColorTexture::ReserveStrip(RainbowColors const & colors, m2::PointF & firstCenter, m2::PointF & lastCenter)
{
  return m_palette.ReserveStrip(colors, firstCenter, lastCenter);
}

int ColorTexture::GetColorSizeInPixels()
{
  return kResourceSize;
}

}  // namespace dp

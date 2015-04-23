#include "drape/texture_of_colors.hpp"

#include "base/shared_buffer_manager.hpp"
#include "base/stl_add.hpp"

#include "std/cstring.hpp"


namespace  dp
{

namespace
{
  int const RESOURCE_SIZE = 2;
  int const BYTES_PER_PIXEL = 4;
}

ColorPalette::ColorPalette(m2::PointU const & canvasSize)
   : m_textureSize(canvasSize)
   , m_cursor(m2::PointU::Zero())
{}

ref_ptr<Texture::ResourceInfo> ColorPalette::MapResource(ColorKey const & key, bool & newResource)
{
  TPalette::iterator itm = m_palette.find(key.m_color);
  newResource = itm == m_palette.end();
  if (newResource)
  {
    PendingColor pendingColor;
    pendingColor.m_color = key.m_color;
    pendingColor.m_rect = m2::RectU(m_cursor.x, m_cursor.y,
                                    m_cursor.x + RESOURCE_SIZE, m_cursor.y + RESOURCE_SIZE);
    {
      threads::MutexGuard g(m_lock);
      m_pendingNodes.push_back(pendingColor);
    }

    m_cursor.x += RESOURCE_SIZE;
    if (m_cursor.x >= m_textureSize.x)
    {
      m_cursor.y += RESOURCE_SIZE;
      m_cursor.x = 0;

      ASSERT(m_cursor.y < m_textureSize.y, ());
    }

    float const sizeX = static_cast<float>(m_textureSize.x);
    float const sizeY = static_cast<float>(m_textureSize.y);
    m2::PointF const resCenter = m2::RectF(pendingColor.m_rect).Center();
    float const x = resCenter.x / sizeX;
    float const y = resCenter.y / sizeY;
    auto res = m_palette.emplace(key.m_color, ColorResourceInfo(m2::RectF(x, y, x, y)));
    ASSERT(res.second, ());
    itm = res.first;
  }
  return make_ref(&itm->second);
}

void ColorPalette::UploadResources(ref_ptr<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::RGBA8, ());
  if (m_pendingNodes.empty())
    return;

  buffer_vector<PendingColor, 16> pendingNodes;
  {
    threads::MutexGuard g(m_lock);
    m_pendingNodes.swap(pendingNodes);
  }

  buffer_vector<size_t, 3> ranges;
  ranges.push_back(0);

  uint32_t minX = pendingNodes[0].m_rect.minX();
  for (size_t i = 0; i < pendingNodes.size(); ++i)
  {
    m2::RectU const & currentRect = pendingNodes[i].m_rect;
    if (minX > currentRect.minX())
    {
      ranges.push_back(i);
      minX = currentRect.minX();
    }
  }

  ranges.push_back(pendingNodes.size());

  for (size_t i = 1; i < ranges.size(); ++i)
  {
    size_t startRange = ranges[i - 1];
    size_t endRange = ranges[i];

    m2::RectU const & startRect = pendingNodes[startRange].m_rect;
    m2::RectU const & endRect = pendingNodes[endRange - 1].m_rect;

    m2::RectU uploadRect;
    if (startRect.minY() == endRect.minY() &&
        startRect.maxY() == endRect.maxY())
    {
      uploadRect = m2::RectU(startRect.minX(), startRect.minY(), endRect.maxX(), endRect.maxY());
    }
    else
    {
      ASSERT(startRect.minX() == 0, ());
      uploadRect = m2::RectU(0, startRect.minY(), m_textureSize.x, endRect.maxY());
    }

    size_t pixelStride = uploadRect.SizeX();
    size_t byteCount = BYTES_PER_PIXEL * uploadRect.SizeX() * uploadRect.SizeY();
    size_t bufferSize = my::NextPowOf2(byteCount);

    SharedBufferManager::shared_buffer_ptr_t buffer = SharedBufferManager::instance().reserveSharedBuffer(bufferSize);
    uint8_t * pointer = SharedBufferManager::GetRawPointer(buffer);
    if (m_isDebug)
      memset(pointer, 0, bufferSize);
    uint32_t currentY = startRect.minY();
    for (size_t j = startRange; j < endRange; ++j)
    {
      ASSERT(pointer < SharedBufferManager::GetRawPointer(buffer) + byteCount, ());
      PendingColor const & c = pendingNodes[j];
      if (c.m_rect.minY() > currentY)
      {
        pointer += BYTES_PER_PIXEL * pixelStride;
        currentY = c.m_rect.minY();
      }

      uint32_t byteStride = pixelStride * BYTES_PER_PIXEL;
      uint8_t red = c.m_color.GetRed();
      uint8_t green = c.m_color.GetGreen();
      uint8_t blue = c.m_color.GetBlue();
      uint8_t alfa = c.m_color.GetAlfa();

      pointer[0] = pointer[4] = red;
      pointer[1] = pointer[5] = green;
      pointer[2] = pointer[6] = blue;
      pointer[3] = pointer[7] = alfa;
      pointer[byteStride + 0] = pointer[byteStride + 4] = red;
      pointer[byteStride + 1] = pointer[byteStride + 5] = green;
      pointer[byteStride + 2] = pointer[byteStride + 6] = blue;
      pointer[byteStride + 3] = pointer[byteStride + 7] = alfa;

      pointer += 2 * BYTES_PER_PIXEL;
      ASSERT(pointer <= SharedBufferManager::GetRawPointer(buffer) + byteCount, ());
    }

    pointer = SharedBufferManager::GetRawPointer(buffer);
    texture->UploadData(uploadRect.minX(), uploadRect.minY(), uploadRect.SizeX(), uploadRect.SizeY(),
                        dp::RGBA8, make_ref(pointer));
  }
}

glConst ColorPalette::GetMinFilter() const
{
  return gl_const::GLNearest;
}

glConst ColorPalette::GetMagFilter() const
{
  return gl_const::GLNearest;
}

void ColorPalette::MoveCursor()
{
  m_cursor.x += RESOURCE_SIZE;
  if (m_cursor.x >= m_textureSize.x)
  {
    m_cursor.y += RESOURCE_SIZE;
    m_cursor.x = 0;
  }

  ASSERT(m_cursor.y + RESOURCE_SIZE <= m_textureSize.y, ());
}

}

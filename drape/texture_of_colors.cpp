#include "drape/texture_of_colors.hpp"

#include "base/shared_buffer_manager.hpp"
#include "base/stl_add.hpp"

#include "std/cstring.hpp"


namespace  dp
{

namespace
{
  int const RESOURCE_SIZE = 1;
  int const BYTES_PER_PIXEL = 4;
}

ColorPalette::ColorPalette(m2::PointU const & canvasSize)
   : m_textureSize(canvasSize)
   , m_cursor(m2::PointU::Zero())
{}

ref_ptr<Texture::ResourceInfo> ColorPalette::ReserveResource(bool predefined, ColorKey const & key, bool & newResource)
{
  lock_guard<mutex> lock(m_mappingLock);

  TPalette & palette = predefined ? m_predefinedPalette : m_palette;
  TPalette::iterator itm = palette.find(key.m_color);
  newResource = (itm == palette.end());
  if (newResource)
  {
    PendingColor pendingColor;
    pendingColor.m_color = key.m_color;
    pendingColor.m_rect = m2::RectU(m_cursor.x, m_cursor.y,
                                    m_cursor.x + RESOURCE_SIZE, m_cursor.y + RESOURCE_SIZE);
    {
      lock_guard<mutex> g(m_lock);
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
    auto res = palette.emplace(key.m_color, ColorResourceInfo(m2::RectF(x, y, x, y)));
    ASSERT(res.second, ());
    itm = res.first;
  }

  return make_ref(&itm->second);
}

ref_ptr<Texture::ResourceInfo> ColorPalette::MapResource(ColorKey const & key, bool & newResource)
{
  TPalette::iterator itm = m_predefinedPalette.find(key.m_color);
  if (itm != m_predefinedPalette.end())
  {
    newResource = false;
    return make_ref(&itm->second);
  }
  return ReserveResource(false /* predefined */, key, newResource);
}

void ColorPalette::UploadResources(ref_ptr<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::RGBA8, ());
  if (m_pendingNodes.empty())
    return;

  buffer_vector<PendingColor, 16> pendingNodes;
  {
    lock_guard<mutex> g(m_lock);
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

    size_t byteCount = BYTES_PER_PIXEL * uploadRect.SizeX() * uploadRect.SizeY();
    size_t bufferSize = my::NextPowOf2(byteCount);

    SharedBufferManager::shared_buffer_ptr_t buffer = SharedBufferManager::instance().reserveSharedBuffer(bufferSize);
    uint8_t * pointer = SharedBufferManager::GetRawPointer(buffer);
    if (m_isDebug)
      memset(pointer, 0, bufferSize);

    for (size_t j = startRange; j < endRange; ++j)
    {
      ASSERT(pointer < SharedBufferManager::GetRawPointer(buffer) + byteCount, ());
      PendingColor const & c = pendingNodes[j];
      pointer[0] = c.m_color.GetRed();
      pointer[1] = c.m_color.GetGreen();
      pointer[2] = c.m_color.GetBlue();
      pointer[3] = c.m_color.GetAlfa();

      pointer += BYTES_PER_PIXEL;
      ASSERT(pointer <= SharedBufferManager::GetRawPointer(buffer) + byteCount, ());
    }

    pointer = SharedBufferManager::GetRawPointer(buffer);
    texture->UploadData(uploadRect.minX(), uploadRect.minY(),
                        uploadRect.SizeX(), uploadRect.SizeY(), make_ref(pointer));
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

void ColorTexture::ReserveColor(dp::Color const & color)
{
  bool newResource = false;
  m_indexer->ReserveResource(true /* predefined */, ColorKey(color), newResource);
}

}

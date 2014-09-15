#include "texture_of_colors.hpp"

namespace  dp
{

ColorPalette::ColorPalette(m2::PointU const & canvasSize)
   : m_textureSize(canvasSize),
  m_curY(0),
  m_curX(0)
{
  m_info = new ColorResourceInfo(m2::RectF(0, 0, 1, 1));
}

ColorResourceInfo const * ColorPalette::MapResource(const ColorKey &key)
{
  TPalette::iterator itm = m_palette.find(key.GetColor());
  if (itm == m_palette.end())
  {
    m_pendingNodes.push_back(key.GetColor());
    TInserted example = m_palette.insert(make_pair(key.GetColor(), m_palette.size()));
    itm = example.first;
  }
  float const sizeX = static_cast<float>(m_textureSize.x);
  float x = (itm->second % m_textureSize.x) / sizeX;
  float y = (itm->second / m_textureSize.x) / sizeX;
  float deltaX = 1.0f / sizeX;
  float deltaY = 1.0f / sizeX;
  *m_info = ColorResourceInfo(m2::RectF(x, y, x + deltaX, y + deltaY));
  return m_info;
}

void ColorPalette::UploadResources(RefPointer<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::RGBA8, ());
  if (m_pendingNodes.empty())
    return;
  uint32_t nodeCnt = m_pendingNodes.size();
  uint32_t offset1 = m_textureSize.x - m_curX;
  if (offset1 >= nodeCnt)
  {
    texture->UploadData(m_curX, m_curY, nodeCnt, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[0]));
    Move(nodeCnt);
  }
  else
  {
    texture->UploadData(m_curX, m_curY, offset1, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[0]));
    m_curY += 1;
    m_curX = 0;
    uint32_t ySteps = (nodeCnt - offset1) / m_textureSize.x;
    uint32_t xSteps = (nodeCnt - offset1) % m_textureSize.x;
    texture->UploadData(0, m_curY, m_textureSize.x, ySteps, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[offset1]));
    m_curY += ySteps;
    if (!xSteps)
      return;
    texture->UploadData(m_curX, m_curY, xSteps, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[offset1 + ySteps * m_textureSize.x]));
    m_curX += xSteps;
  }

  m_pendingNodes.clear();
}

void ColorPalette::AddData(void const * data, uint32_t size)
{
  ASSERT(size % 4 == 0, ());
  uint32_t const cnt = size / 4;
  uint32_t const * idata = (uint32_t *)data;
  for (int i = 0; i < cnt; i++)
  {
    TInserted example = m_palette.insert(make_pair(idata[i], m_palette.size()));
    if (example.second)
      m_pendingNodes.push_back(idata[i]);
  }
}

void ColorPalette::Move(uint32_t step)
{
  uint32_t initial = min(step, m_textureSize.x - m_curX);
  m_curX += initial;
  if (m_curX == m_textureSize.x)
  {
    m_curX = 0;
    m_curY += 1;
  }
  step -= initial;
  if (step <= 0)
    return;
  m_curY += step / m_textureSize.x;
  m_curX += step % m_textureSize.x;
}

}

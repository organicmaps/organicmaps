#include "texture_of_colors.hpp"

namespace  dp
{

ColorPalette::ColorPalette(m2::PointU const & canvasSize)
   : m_textureSize(canvasSize),
  m_curY(0),
  m_curX(0)
{}

ColorPalette::~ColorPalette()
{
  TPalette::iterator it = m_palette.begin();
  for (; it != m_palette.end(); ++it)
    it->second.Destroy();
}

ColorResourceInfo const * ColorPalette::MapResource(const ColorKey &key)
{
  uint32_t color = key.GetColor();
  TPalette::iterator itm = m_palette.find(color);
  if (itm == m_palette.end())
  {
    m_pendingNodes.push_back(color);
    int const size = m_palette.size();
    float const sizeX = static_cast<float>(m_textureSize.x);
    float const sizeY = static_cast<float>(m_textureSize.y);
    float const x = (size % m_textureSize.x + 0.5f) / sizeX;
    float const y = (size / m_textureSize.x + 0.5f) / sizeY;
    TResourcePtr m_ptr(new ColorResourceInfo(m2::RectF(x, y, x, y)));
    itm = m_palette.insert(make_pair(color, m_ptr)).first;
  }
  return itm->second.GetRaw();
}

void ColorPalette::UploadResources(RefPointer<Texture> texture)
{
  ASSERT(texture->GetFormat() == dp::RGBA8, ());
  if (m_pendingNodes.empty())
    return;
  uint32_t const nodeCnt = m_pendingNodes.size();
  uint32_t const offset = m_textureSize.x - m_curX;
  if (offset >= nodeCnt)
  {
    texture->UploadData(m_curX, m_curY, nodeCnt, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[0]));
    m_curX += nodeCnt;
    if (offset == nodeCnt)
    {
      m_curX = 0;
      m_curY++;
    }
  }
  else
  {
    texture->UploadData(m_curX, m_curY, offset, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[0]));
    m_curY++;
    m_curX = 0;
    uint32_t const remnant = nodeCnt - offset;
    uint32_t const ySteps = remnant / m_textureSize.x;
    uint32_t const xSteps = remnant % m_textureSize.x;
    if (ySteps)
      texture->UploadData(0, m_curY, m_textureSize.x, ySteps, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[offset]));
    m_curY += ySteps;
    if (!xSteps)
      return;
    texture->UploadData(m_curX, m_curY, xSteps, 1, dp::RGBA8, MakeStackRefPointer<void>(&m_pendingNodes[offset + ySteps * m_textureSize.x]));
    m_curX += xSteps;
  }

  m_pendingNodes.clear();
}

}

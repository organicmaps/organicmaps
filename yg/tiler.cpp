#include "../base/SRC_FIRST.hpp"
#include "tiler.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"
#include "../base/logging.hpp"

namespace yg
{
  uint64_t Tiler::RectInfo::toUInt64Cell() const
  {
    /// pack y in 0-16 bits
    /// pack x in 17-33 bits
    /// pack scale in 34-40 bits
    return ((m_y & 0x1FF)
         | ((m_x & 0x1FF) << 17)
         | ((uint64_t)m_tileScale & 0x1F) << 34)
         | ((uint64_t)m_drawScale & 0x1F) << 40;
  }

  void Tiler::RectInfo::fromUInt64Cell(uint64_t v, m2::RectD const & globRect, m2::PointD const & centerPt)
  {
    m_y = v & 0x1FF;
    m_x = (v >> 17) & 0x1FF;
    m_tileScale = (v >> 34) & 0x1F;
    m_drawScale = (v >> 40) & 0x1F;
    init(globRect, centerPt);
  }

  Tiler::RectInfo::RectInfo()
    : m_drawScale(0), m_tileScale(0), m_x(0), m_y(0), m_distance(0), m_coverage(0)
  {}

  Tiler::RectInfo::RectInfo(int drawScale, int tileScale, int x, int y, m2::RectD const & globRect, m2::PointD const & centerPt)
    : m_drawScale(drawScale), m_tileScale(tileScale), m_x(x), m_y(y)
  {
    init(globRect, centerPt);
  }

  void Tiler::RectInfo::init(m2::RectD const & globRect, m2::PointD const & centerPt)
  {
    int k = 1 << m_tileScale;

    double rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / k;
    double rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / k;

    m_rect.setMinX(m_x * rectSizeX);
    m_rect.setMaxX((m_x + 1) * rectSizeX);
    m_rect.setMinY(m_y * rectSizeY);
    m_rect.setMaxY((m_y + 1) * rectSizeY);

    m_distance = m_rect.Center().Length(centerPt);
    m2::RectD r = globRect;
    if (r.Intersect(m_rect))
      m_coverage = r.SizeX() * r.SizeY() / (m_rect.SizeX() * m_rect.SizeY());
    else
      m_coverage = 0;
  }


  bool operator<(Tiler::RectInfo const & l, Tiler::RectInfo const & r)
  {
    if (l.m_coverage != r.m_coverage)
      return l.m_coverage > r.m_coverage;

    return l.m_distance < r.m_distance;
  }

  bool operator>(Tiler::RectInfo const & l, Tiler::RectInfo const & r)
  {
    if (l.m_coverage != r.m_coverage)
      return l.m_coverage < r.m_coverage;

    return l.m_distance > r.m_distance;
  }

  void Tiler::seed(ScreenBase const & screen, m2::PointD const & centerPt, int tileSize, int scaleEtalonSize)
  {
    if (screen != m_screen)
      ++m_seqNum;

    m2::RectD glbRect;
    m2::PointD pxCenter = screen.PixelRect().Center();
    screen.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                          pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                glbRect);

    m_drawScale = scales::GetScaleLevel(glbRect);

    m_screen = screen;

    m2::RectD const screenRect = m_screen.GlobalRect();

    /// slightly smaller than original to produce "antialiasing" effect using bilinear filtration.
    tileSize /= 1.05;

    screen.PtoG(m2::RectD(pxCenter - m2::PointD(tileSize / 2, tileSize / 2),
                          pxCenter + m2::PointD(tileSize / 2, tileSize / 2)),
                glbRect);

    double glbRectSize = min(glbRect.SizeX(), glbRect.SizeY());

    m_tileScale = ceil(log((MercatorBounds::maxX - MercatorBounds::minX) / glbRectSize) / log(2));

    double rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / (1 << m_tileScale);
    double rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / (1 << m_tileScale);

    int minTileX = floor(screenRect.minX() / rectSizeX);
    int maxTileX = ceil(screenRect.maxX() / rectSizeX);
    int minTileY = floor(screenRect.minY() / rectSizeY);
    int maxTileY = ceil(screenRect.maxY() / rectSizeY);

    unsigned rectCount = 0;

    /// clearing previous coverage

    while (!m_coverage.empty())
      m_coverage.pop();

    for (int tileY = minTileY; tileY < maxTileY; ++tileY)
      for (int tileX = minTileX; tileX < maxTileX; ++tileX)
      {
        m_coverage.push(RectInfo(m_drawScale, m_tileScale, tileX, tileY, screenRect, centerPt));
        ++rectCount;
      }
  }

  Tiler::Tiler() : m_seqNum(0)
  {}

  bool Tiler::hasTile()
  {
    return !m_coverage.empty();
  }

  Tiler::RectInfo const Tiler::nextTile()
  {
    RectInfo r = m_coverage.top();
    m_coverage.pop();
    return r;
  }

  size_t Tiler::seqNum() const
  {
    return m_seqNum;
  }
}

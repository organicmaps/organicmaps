#include "../base/SRC_FIRST.hpp"
#include "tiler.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"
#include "../base/logging.hpp"

uint64_t Tiler::RectInfo::toUInt64Cell() const
{
  /// pack y in 0-23 bits
  /// pack x in 24-47 bits
  /// pack tileScale in 48-55 bits
  /// pack drawScale in 56-63 bits

  ASSERT(m_y <= 0xFFFFFF, ());
  ASSERT(m_x <= 0xFFFFFF, ());
  ASSERT(m_tileScale <= 0xFF, ());
  ASSERT(m_drawScale <= 0xFF, ());

  return (m_y & 0xFFFFFF)
      | ((m_x & 0xFFFFFF) << 24)
      | (((uint64_t)m_tileScale & 0xFF) << 48)
      | (((uint64_t)m_drawScale & 0xFF) << 56);
}

void Tiler::RectInfo::fromUInt64Cell(uint64_t v)
{
  m_y = v & 0xFFFFFF;
  m_x = (v >> 24) & 0xFFFFFF;
  m_tileScale = (v >> 48) & 0xFF;
  m_drawScale = (v >> 56) & 0xFF;
}

Tiler::RectInfo::RectInfo()
  : m_drawScale(0), m_tileScale(0), m_x(0), m_y(0)
{}

Tiler::RectInfo::RectInfo(int drawScale, int tileScale, int x, int y)
  : m_drawScale(drawScale), m_tileScale(tileScale), m_x(x), m_y(y)
{
  int k = 1 << m_tileScale;

  double rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / k;
  double rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / k;

  m_rect.setMinX(m_x * rectSizeX);
  m_rect.setMaxX((m_x + 1) * rectSizeX);
  m_rect.setMinY(m_y * rectSizeY);
  m_rect.setMaxY((m_y + 1) * rectSizeY);
}

LessByDistance::LessByDistance(m2::PointD const & pt)
  : m_pt(pt)
{
}

bool LessByDistance::operator()(Tiler::RectInfo const & l, Tiler::RectInfo const & r)
{
  return l.m_rect.Center().Length(m_pt) < r.m_rect.Center().Length(m_pt);
}

bool operator<(Tiler::RectInfo const & l, Tiler::RectInfo const & r)
{
  return l.toUInt64Cell() < r.toUInt64Cell();
}

int Tiler::drawScale(ScreenBase const & s) const
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  m2::RectD glbRect;
  m2::PointD pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(m_scaleEtalonSize / 2, m_scaleEtalonSize / 2),
                      pxCenter + m2::PointD(m_scaleEtalonSize / 2, m_scaleEtalonSize / 2)),
            glbRect);

  return scales::GetScaleLevel(glbRect);
}

int Tiler::tileScale(ScreenBase const & s) const
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  /// slightly smaller than original to produce "antialiasing" effect using bilinear filtration.
  size_t tileSize = static_cast<size_t>(m_tileSize / 1.05);

  m2::RectD glbRect;
  m2::PointD pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(tileSize / 2, tileSize / 2),
                      pxCenter + m2::PointD(tileSize / 2, tileSize / 2)),
            glbRect);

  double glbRectSize = min(glbRect.SizeX(), glbRect.SizeY());

  int res = static_cast<int>(ceil(log((MercatorBounds::maxX - MercatorBounds::minX) / glbRectSize) / log(2.0)));

  return res;
}

void Tiler::seed(ScreenBase const & screen, m2::PointD const & centerPt)
{
  if (screen != m_screen)
    ++m_sequenceID;

  m_screen = screen;

  m_drawScale = drawScale(screen);
  m_tileScale = tileScale(screen);

  double rectSizeX = (MercatorBounds::maxX - MercatorBounds::minX) / (1 << m_tileScale);
  double rectSizeY = (MercatorBounds::maxY - MercatorBounds::minY) / (1 << m_tileScale);

  m2::AARectD const globalRect = m_screen.GlobalRect();
  m2::RectD const clipRect = m_screen.ClipRect();

  int minTileX = static_cast<int>(floor(clipRect.minX() / rectSizeX));
  int maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSizeX));
  int minTileY = static_cast<int>(floor(clipRect.minY() / rectSizeY));
  int maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSizeY));

  /// clearing previous coverage
  m_coverage.clear();

  /// generating new coverage

  for (int tileY = minTileY; tileY < maxTileY; ++tileY)
    for (int tileX = minTileX; tileX < maxTileX; ++tileX)
    {
      m2::RectD tileRect(tileX * rectSizeX,
                         tileY * rectSizeY,
                         (tileX + 1) * rectSizeX,
                         (tileY + 1) * rectSizeY);

      if (globalRect.IsIntersect(m2::AARectD(tileRect)))
        m_coverage.push_back(RectInfo(m_drawScale, m_tileScale, tileX, tileY));
    }

  /// sorting coverage elements
  sort(m_coverage.begin(), m_coverage.end(), LessByDistance(centerPt));
}

Tiler::Tiler(size_t tileSize, size_t scaleEtalonSize)
  : m_sequenceID(0), m_tileSize(tileSize), m_scaleEtalonSize(scaleEtalonSize)
{}

void Tiler::visibleTiles(vector<RectInfo> & tiles)
{
  tiles = m_coverage;
}

size_t Tiler::sequenceID() const
{
  return m_sequenceID;
}

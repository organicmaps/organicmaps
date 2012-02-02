#include "../base/SRC_FIRST.hpp"
#include "tiler.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"
#include "../base/logging.hpp"
#include "../platform/platform.hpp"

Tiler::RectInfo::RectInfo()
  : m_drawScale(0), m_tileScale(0), m_x(0), m_y(0)
{}

Tiler::RectInfo::RectInfo(int drawScale, int tileScale, int x, int y)
  : m_drawScale(drawScale), m_tileScale(tileScale), m_x(x), m_y(y)
{
  initRect();
}

void Tiler::RectInfo::initRect()
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
  if (l.m_y != r.m_y)
    return l.m_y < r.m_y;
  if (l.m_x != r.m_x)
    return l.m_x < r.m_x;
  if (l.m_drawScale != r.m_drawScale)
    return l.m_drawScale < r.m_drawScale;
  if (l.m_tileScale != r.m_tileScale)
    return l.m_tileScale < r.m_tileScale;
  return false;
}

bool operator==(Tiler::RectInfo const & l, Tiler::RectInfo const & r)
{
  return (l.m_y == r.m_y)
      && (l.m_x == r.m_x)
      && (l.m_drawScale == r.m_drawScale)
      && (l.m_tileScale == r.m_tileScale);
}


int Tiler::drawScale(ScreenBase const & s) const
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  size_t tileSize = min(static_cast<size_t>(m_tileSize / 1.05), (size_t)512);

  m2::RectD glbRect;
  m2::PointD pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(tileSize / 2, tileSize / 2),
                      pxCenter + m2::PointD(tileSize / 2, tileSize / 2)),
            glbRect);

  double glbRectSize = min(glbRect.SizeX(), glbRect.SizeY());

  int res = static_cast<int>(ceil(log((MercatorBounds::maxX - MercatorBounds::minX) / glbRectSize) / log(2.0)));

  return res > scales::GetUpperScale() ? scales::GetUpperScale() : res;
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

  /// calculating coverage on the global rect, which corresponds to the
  /// pixel rect, which is square with size of nearest power-of-two

  m2::RectD pxRect = m_screen.PixelRect();

  int tileSize = GetPlatform().TileSize();

  int pxWidthInTiles = (pxRect.SizeX() + tileSize - 1) / tileSize;
  int pxHeightInTiles = (pxRect.SizeY() + tileSize - 1) / tileSize;

  pxWidthInTiles += 1;
  pxHeightInTiles += 1;

  double glbHalfSizeX = m_screen.PtoG(pxRect.Center() - m2::PointD(pxWidthInTiles * tileSize / 2, 0)).Length(m_screen.PtoG(pxRect.Center()));
  double glbHalfSizeY = m_screen.PtoG(pxRect.Center() - m2::PointD(0, pxHeightInTiles * tileSize / 2)).Length(m_screen.PtoG(pxRect.Center()));

  m2::AnyRectD globalRect(m_screen.PtoG(m_screen.PixelRect().Center()),
                          m_screen.GlobalRect().angle(),
                          m2::RectD(-glbHalfSizeX, -glbHalfSizeY, glbHalfSizeX, glbHalfSizeY));

  m2::RectD const clipRect = globalRect.GetGlobalRect();

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

      if (globalRect.IsIntersect(m2::AnyRectD(tileRect)))
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

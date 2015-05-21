#include "scales_processor.hpp"

#include "geometry/screenbase.hpp"

#include "indexer/mercator.hpp"
#include "indexer/scales.hpp"


/// Note! Default tile size value should be equal with
/// the default value in RenderPolicy::TileSize().
ScalesProcessor::ScalesProcessor()
  : m_tileSize(256), m_visualScale(1.0)
{
}

ScalesProcessor::ScalesProcessor(int tileSize)
  : m_tileSize(tileSize), m_visualScale(1.0)
{
}

void ScalesProcessor::SetParams(double visualScale, int tileSize)
{
  m_tileSize = tileSize;
  m_visualScale = visualScale;
}

m2::RectD const & ScalesProcessor::GetWorldRect()
{
  static m2::RectD const worldRect = MercatorBounds::FullRect();
  return worldRect;
}

int ScalesProcessor::GetTileScaleBase(ScreenBase const & s) const
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  // slightly smaller than original to produce "antialiasing" effect using bilinear filtration.
  int const halfSize = static_cast<int>(m_tileSize / 1.05 / 2.0);

  m2::RectD glbRect;
  m2::PointD const pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(halfSize, halfSize),
                      pxCenter + m2::PointD(halfSize, halfSize)),
            glbRect);

  return GetTileScaleBase(glbRect);
}

int ScalesProcessor::GetTileScaleBase(m2::RectD const & r) const
{
  double const sz = max(r.SizeX(), r.SizeY());
  return max(1, my::rounds(log((MercatorBounds::maxX - MercatorBounds::minX) / sz) / log(2.0)));
}

int ScalesProcessor::GetTileScaleIncrement() const
{
  return log(m_tileSize / 256.0 / m_visualScale) / log(2.0);
}

int ScalesProcessor::CalculateTileSize(int screenWidth, int screenHeight)
{
  int const maxSz = max(screenWidth, screenHeight);

  // we're calculating the tileSize based on (maxSz > 1024 ? rounded : ceiled)
  // to the nearest power of two value of the maxSz

  int const ceiledSz = 1 << static_cast<int>(ceil(log(double(maxSz + 1)) / log(2.0)));
  int res = 0;

  if (maxSz < 1024)
    res = ceiledSz;
  else
  {
    int const flooredSz = ceiledSz / 2;
    // rounding to the nearest power of two.
    if (ceiledSz - maxSz < maxSz - flooredSz)
      res = ceiledSz;
    else
      res = flooredSz;
  }

  return min(max(res / 2, 256), 1024);
}

m2::RectD ScalesProcessor::GetRectForDrawScale(int drawScale, m2::PointD const & center) const
{
  // +1 - we will calculate half length for each side
  double const factor = 1 << (max(1, drawScale - GetTileScaleIncrement()) + 1);

  double const len = (MercatorBounds::maxX - MercatorBounds::minX) / factor;

  return m2::RectD(MercatorBounds::ClampX(center.x - len),
                   MercatorBounds::ClampY(center.y - len),
                   MercatorBounds::ClampX(center.x + len),
                   MercatorBounds::ClampY(center.y + len));
}

double ScalesProcessor::GetClipRectInflation() const
{
  /// @todo Why 24? Probably better to get some part of m_tileSize?
  return 24 * m_visualScale;
}

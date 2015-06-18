#include "drape_frontend/visual_params.hpp"

#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/assert.hpp"

#include "indexer/mercator.hpp"

#include "std/limits.hpp"
#include "std/algorithm.hpp"

namespace df
{

uint32_t const YotaDevice = 0x1;

namespace
{

static VisualParams g_VizParams;

typedef pair<string, double> visual_scale_t;

struct VisualScaleFinder
{
  VisualScaleFinder(double vs)
    : m_vs(vs)
  {
  }

  bool operator()(visual_scale_t const & node)
  {
    return my::AlmostEqualULPs(node.second, m_vs);
  }

  double m_vs;
};

} // namespace

#ifdef DEBUG
static bool g_isInited = false;
#define RISE_INITED g_isInited = true
#define ASSERT_INITED ASSERT(g_isInited, ())
#else
#define RISE_INITED
#define ASSERT_INITED
#endif


void VisualParams::Init(double vs, uint32_t tileSize, vector<uint32_t> const & additionalOptions)
{
  g_VizParams.m_tileSize = tileSize;
  g_VizParams.m_visualScale = vs;
  if (find(additionalOptions.begin(), additionalOptions.end(), YotaDevice) != additionalOptions.end())
    g_VizParams.m_isYotaDevice = true;
  RISE_INITED;
}

VisualParams & VisualParams::Instance()
{
  ASSERT_INITED;
  return g_VizParams;
}

string const & VisualParams::GetResourcePostfix() const
{
  static visual_scale_t postfixes[] =
  {
    make_pair("ldpi", 0.75),
    make_pair("mdpi", 1.0),
    make_pair("hdpi", 1.5),
    make_pair("xhdpi", 2.0),
    make_pair("xxhdpi", 3.0),
    make_pair("6plus", 2.4),
  };

  static string specifixPostfixes[] =
  {
    "yota"
  };

  if (m_isYotaDevice)
    return specifixPostfixes[0];

  visual_scale_t * finded = find_if(postfixes, postfixes + ARRAY_SIZE(postfixes), VisualScaleFinder(m_visualScale));
  ASSERT(finded < postfixes + ARRAY_SIZE(postfixes), ());
  return finded->first;
}

double VisualParams::GetVisualScale() const
{
  return m_visualScale;
}

uint32_t VisualParams::GetTileSize() const
{
  return m_tileSize;
}

uint32_t VisualParams::GetTouchRectRadius() const
{
  return 20 * GetVisualScale();
}

double VisualParams::GetDragThreshold() const
{
  return 10.0 * GetVisualScale();
}

VisualParams::VisualParams()
  : m_tileSize(0)
  , m_visualScale(0.0)
{
}

m2::RectD const & GetWorldRect()
{
  static m2::RectD const worldRect = MercatorBounds::FullRect();
  return worldRect;
}

int GetTileScaleBase(ScreenBase const & s)
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  // slightly smaller than original to produce "antialiasing" effect using bilinear filtration.
  int const halfSize = static_cast<int>(VisualParams::Instance().GetTileSize() / 1.05 / 2.0);

  m2::RectD glbRect;
  m2::PointD const pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(halfSize, halfSize),
                      pxCenter + m2::PointD(halfSize, halfSize)),
            glbRect);

  return GetTileScaleBase(glbRect);
}

int GetTileScaleBase(m2::RectD const & r)
{
  double const sz = max(r.SizeX(), r.SizeY());
  return max(1, my::rounds(log((MercatorBounds::maxX - MercatorBounds::minX) / sz) / log(2.0)));
}

int GetTileScaleIncrement()
{
  VisualParams const & p = VisualParams::Instance();
  return log(p.GetTileSize() / 256.0 / p.GetVisualScale()) / log(2.0);
}

m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center)
{
  // +1 - we will calculate half length for each side
  double const factor = 1 << (max(1, drawScale - GetTileScaleIncrement()) + 1);

  double const len = (MercatorBounds::maxX - MercatorBounds::minX) / factor;

  return m2::RectD(MercatorBounds::ClampX(center.x - len),
                   MercatorBounds::ClampY(center.y - len),
                   MercatorBounds::ClampX(center.x + len),
                   MercatorBounds::ClampY(center.y + len));
}

m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center)
{
  return GetRectForDrawScale(my::rounds(drawScale), center);
}

int CalculateTileSize(int screenWidth, int screenHeight)
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

#ifndef OMIM_OS_DESKTOP
  return my::clamp(res / 2, 256, 1024);
#else
  return my::clamp(res / 2, 512, 1024);
#endif
}

int GetDrawTileScale(int baseScale)
{
  return max(1, baseScale + GetTileScaleIncrement());
}

int GetDrawTileScale(ScreenBase const & s)
{
  return GetDrawTileScale(GetTileScaleBase(s));
}

int GetDrawTileScale(m2::RectD const & r)
{
  return GetDrawTileScale(GetTileScaleBase(r));
}

} // namespace df

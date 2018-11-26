#include "drape_frontend/gui/ruler_helper.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"
#include "geometry/screenbase.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <set>

namespace gui
{
namespace
{
int constexpr kMinPixelWidth = 60;
int constexpr kMinMetersWidth = 10;
int constexpr kMaxMetersWidth = 1000000;

int constexpr kMinUnitValue = -1;
int constexpr kMaxUnitValue = std::numeric_limits<int>::max() - 1;
int constexpr kInvalidUnitValue = kMaxUnitValue + 1;

int constexpr kVisibleRulerBottomScale = 5;

struct UnitValue
{
  char const * m_s;
  int m_i;
};

UnitValue g_arrFeets[] = {
  { "10 ft", 10 },
  { "20 ft", 20 },
  { "50 ft", 50 },
  { "100 ft", 100 },
  { "200 ft", 200 },
  { "0.1 mi", 528 },
  { "0.2 mi", 528 * 2 },
  { "0.5 mi", 528 * 5 },
  { "1 mi", 5280 },
  { "2 mi", 2 * 5280 },
  { "5 mi", 5 * 5280 },
  { "10 mi", 10 * 5280 },
  { "20 mi", 20 * 5280 },
  { "50 mi", 50 * 5280 },
  { "100 mi", 100 * 5280 },
  { "200 mi", 200 * 5280 },
  { "500 mi", 500 * 5280 }
};

UnitValue g_arrYards[] = {
  { "50 yd", 50 },
  { "100 yd", 100 },
  { "200 yd", 200 },
  { "500 yd", 500 },
  { "0.5 mi", 1760 / 2 },
  { "1 mi", 1760 },
  { "2 mi", 2 * 1760 },
  { "5 mi", 5 * 1760 },
  { "10 mi", 10 * 1760 },
  { "20 mi", 20 * 1760 },
  { "50 mi", 50 * 1760 },
  { "100 mi", 100 * 1760 },
  { "200 mi", 200 * 1760 },
  { "500 mi", 500 * 1760 }
};

UnitValue g_arrMeters[] = {
  { "1 m", 1 },
  { "2 m", 2 },
  { "5 m", 5 },
  { "10 m", 10 },
  { "20 m", 20 },
  { "50 m", 50 },
  { "100 m", 100 },
  { "200 m", 200 },
  { "500 m", 500 },
  { "1 km", 1000 },
  { "2 km", 2000 },
  { "5 km", 5000 },
  { "10 km", 10000 },
  { "20 km", 20000 },
  { "50 km", 50000 },
  { "100 km", 100000 },
  { "200 km", 200000 },
  { "500 km", 500000 },
  { "1000 km", 1000000 }
};

double Identity(double val)
{
  return val;
}
}  // namespace

RulerHelper::RulerHelper()
  : m_pixelLength(0.0f)
  , m_rangeIndex(kInvalidUnitValue)
  , m_isTextDirty(false)
  , m_dirtyTextRequested(false)
{}

void RulerHelper::Update(ScreenBase const & screen)
{
  m2::PointD pivot = screen.PixelRect().Center();
  int const minPxWidth = base::rounds(kMinPixelWidth * df::VisualParams::Instance().GetVisualScale());
  m2::PointD pt1 = screen.PtoG(pivot);
  m2::PointD pt0 = screen.PtoG(pivot - m2::PointD(minPxWidth, 0));

  double const distanceInMeters = MercatorBounds::DistanceOnEarth(pt0, pt1);

  // convert metres to units for calculating m_metresDiff.
  double metersDiff = CalcMetersDiff(distanceInMeters);

  bool const higherThanMax = metersDiff > kMaxMetersWidth;
  bool const lessThanMin = metersDiff < kMinMetersWidth;
  m_pixelLength = static_cast<float>(minPxWidth);

  if (higherThanMax)
    m_pixelLength = static_cast<float>(minPxWidth) * 3.0f / 2.0f;
  else if (!lessThanMin)
  {
    double const a = ang::AngleTo(pt1, pt0);
    pt0 = MercatorBounds::GetSmPoint(pt1, cos(a) * metersDiff, sin(a) * metersDiff);

    m_pixelLength = base::rounds(pivot.Length(screen.GtoP(pt0)));
  }

  int drawScale = df::GetDrawTileScale(screen);
  if (m_currentDrawScale < kVisibleRulerBottomScale &&
      drawScale >= kVisibleRulerBottomScale)
  {
    SetTextDirty();
  }

  m_currentDrawScale = drawScale;
}

bool RulerHelper::IsVisible(ScreenBase const & screen) const
{
  DrapeGui & gui = DrapeGui::Instance();
  return !gui.IsCopyrightActive() && df::GetDrawTileScale(screen) >= kVisibleRulerBottomScale;
}

void RulerHelper::Invalidate()
{
  SetTextDirty();
}

float RulerHelper::GetRulerHalfHeight() const
{
  float const kRulerHalfHeight = 1.0f;
  return kRulerHalfHeight * static_cast<float>(df::VisualParams::Instance().GetVisualScale());
}

float RulerHelper::GetRulerPixelLength() const
{
  return m_pixelLength;
}

float RulerHelper::GetMaxRulerPixelLength() const
{
  return static_cast<float>(kMinPixelWidth) * 3.0f / 2.0f;
}

int RulerHelper::GetVerticalTextOffset() const
{
  return static_cast<int>(-5 * df::VisualParams::Instance().GetVisualScale());
}

bool RulerHelper::IsTextDirty() const
{
  return m_isTextDirty;
}

string const & RulerHelper::GetRulerText() const
{
  m_dirtyTextRequested = true;
  return m_rulerText;
}

void RulerHelper::ResetTextDirtyFlag()
{
  if (m_dirtyTextRequested)
    m_isTextDirty = false;
}

void RulerHelper::GetTextInitInfo(string & alphabet, uint32_t & size) const
{
  std::set<char> symbols;
  size_t result = 0;
  auto const functor = [&result, &symbols](UnitValue const & v)
  {
    size_t stringSize = strlen(v.m_s);
    result = max(result, stringSize);
    for (size_t i = 0; i < stringSize; ++i)
      symbols.insert(v.m_s[i]);
  };

  std::for_each(std::begin(g_arrFeets), std::end(g_arrFeets), functor);
  std::for_each(std::begin(g_arrMeters), std::end(g_arrMeters), functor);
  std::for_each(std::begin(g_arrYards), std::end(g_arrYards), functor);

  std::for_each(begin(symbols), end(symbols), [&alphabet](char c)
  {
    alphabet.push_back(c);
  });
  alphabet.append("<>");

  size = static_cast<uint32_t>(result) + 2; // add 2 char for symbols "< " and "> ".
}

double RulerHelper::CalcMetersDiff(double value)
{
  UnitValue * arrU = g_arrMeters;
  int count = ARRAY_SIZE(g_arrMeters);

  auto conversionFn = &Identity;

  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);

  if (units == measurement_utils::Units::Imperial)
  {
    arrU = g_arrFeets;
    count = ARRAY_SIZE(g_arrFeets);
    conversionFn = &measurement_utils::MetersToFeet;
  }

  int prevUnitRange = m_rangeIndex;
  double result = 0.0;
  double v = conversionFn(value);
  if (arrU[0].m_i > v)
  {
    m_rangeIndex = kMinUnitValue;
    m_rulerText = string("< ") + arrU[0].m_s;
    result = kMinMetersWidth - 1.0;
  }
  else if (arrU[count-1].m_i <= v)
  {
    m_rangeIndex = kMaxUnitValue;
    m_rulerText = string("> ") + arrU[count-1].m_s;
    result = kMaxMetersWidth + 1.0;
  }
  else
  {
    for (int i = 0; i < count; ++i)
    {
      if (arrU[i].m_i > v)
      {
        m_rangeIndex = i;
        result = arrU[i].m_i / conversionFn(1.0);
        m_rulerText = arrU[i].m_s;
        break;
      }
    }
  }

  if (m_rangeIndex != prevUnitRange)
    SetTextDirty();

  return result;
}

void RulerHelper::SetTextDirty()
{
  m_dirtyTextRequested = false;
  m_isTextDirty = true;
}
}  // namespace gui

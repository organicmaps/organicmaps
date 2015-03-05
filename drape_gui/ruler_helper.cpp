#include "ruler_helper.hpp"
#include "drape_gui.hpp"

#include "../platform/settings.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/measurement_utils.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/macros.hpp"

#include "../std/algorithm.hpp"
#include "../std/numeric.hpp"
#include "../std/iterator.hpp"

namespace gui
{

namespace
{

static int const MinPixelWidth = 60;
static int const MinMetersWidth = 10;
static int const MaxMetersWidth = 1000000;

static int const MinUnitValue = -1;
static int const MaxUnitValue = numeric_limits<int>::max() - 1;
static int const InvalidUnitValue = MaxUnitValue + 1;

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

UnitValue g_arrMetres[] = {
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

double identity(double val)
{
  return val;
}

}

RulerHelper::RulerHelper()
  : m_pixelLength(0.0)
  , m_rangeIndex(InvalidUnitValue)
  , m_isTextDirty(false)
{
}

RulerHelper & RulerHelper::Instance()
{
  static RulerHelper s_helper;
  return s_helper;
}

void RulerHelper::Update(ScreenBase const & screen)
{
  m2::PointD pivot = screen.PixelRect().Center();
  int const minPxWidth = my::rounds(MinPixelWidth * DrapeGui::Instance().GetScaleFactor());
  m2::PointD pt1 = screen.PtoG(pivot);
  m2::PointD pt0 = screen.PtoG(pivot - m2::PointD(minPxWidth, 0));

  double const distanceInMetres = MercatorBounds::DistanceOnEarth(pt0, pt1);

  // convert metres to units for calculating m_metresDiff
  double metersDiff = CalcMetresDiff(distanceInMetres);

  bool const higherThanMax = metersDiff > MaxMetersWidth;
  bool const lessThanMin = metersDiff < MinMetersWidth;
  m_pixelLength = minPxWidth;

  if (higherThanMax)
    m_pixelLength = minPxWidth * 3 / 2;
  else if (!lessThanMin)
  {
    double const a = ang::AngleTo(pt1, pt0);
    pt0 = MercatorBounds::GetSmPoint(pt1, cos(a) * metersDiff, sin(a) * metersDiff);

    m_pixelLength = my::rounds(pivot.Length(screen.GtoP(pt0)));
  }
}

bool RulerHelper::IsVisible(const ScreenBase & screen) const
{
  return DrapeGui::Instance().GetGeneralization(screen) > 4;
}

float RulerHelper::GetRulerHalfHeight() const
{
  return 2 * DrapeGui::Instance().GetScaleFactor();
}

float RulerHelper::GetRulerPixelLength() const
{
  return m_pixelLength;
}

int RulerHelper::GetVerticalTextOffset() const
{
  return -5 * DrapeGui::Instance().GetScaleFactor();
}

bool RulerHelper::IsTextDirty() const
{
  return m_isTextDirty;
}

string const & RulerHelper::GetRulerText() const
{
  m_isTextDirty = false;
  return m_rulerText;
}

void RulerHelper::GetTextInitInfo(string & alphabet, size_t & size) const
{
  set<char> symbols;
  size_t result = 0;
  auto const functor = [&result, &symbols](UnitValue const & v)
  {
    size_t stringSize = strlen(v.m_s);
    result = max(result, stringSize);
    for (int i = 0; i < stringSize; ++i)
      symbols.insert(v.m_s[i]);
  };

  for_each(begin(g_arrFeets), end(g_arrFeets), functor);
  for_each(begin(g_arrMetres), end(g_arrMetres), functor);
  for_each(begin(g_arrYards), end(g_arrYards), functor);

  for_each(begin(symbols), end(symbols), [&alphabet](char c)
  {
    alphabet.push_back(c);
  });
  alphabet.append("<>");

  size = result + 2; // add 2 char for symbols "< " and "> "
}

double RulerHelper::CalcMetresDiff(double value)
{
  UnitValue * arrU = g_arrMetres;
  int count = ARRAY_SIZE(g_arrMetres);

  typedef double (*ConversionFn)(double);
  ConversionFn conversionFn = &identity;

  Settings::Units units = Settings::Metric;
  Settings::Get("Units", units);

  switch (units)
  {
  case Settings::Foot:
    arrU = g_arrFeets;
    count = ARRAY_SIZE(g_arrFeets);
    conversionFn = &MeasurementUtils::MetersToFeet;
    break;

  case Settings::Yard:
    arrU = g_arrYards;
    count = ARRAY_SIZE(g_arrYards);
    conversionFn = &MeasurementUtils::MetersToYards;
    break;
  default:
    break;
  }

  int prevUnitRange = m_rangeIndex;
  double result = 0.0;
  double v = conversionFn(value);
  if (arrU[0].m_i > v)
  {
    m_rangeIndex = MinUnitValue;
    m_rulerText = string("< ") + arrU[0].m_s;
    result = MinMetersWidth - 1.0;
  }
  else if (arrU[count-1].m_i <= v)
  {
    m_rangeIndex = MaxUnitValue;
    m_rulerText = string("> ") + arrU[count-1].m_s;
    result = MaxMetersWidth + 1.0;
  }
  else
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

  if (m_rangeIndex != prevUnitRange)
    m_isTextDirty = true;
  return result;
}

}

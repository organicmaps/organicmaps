#pragma once

#include "topography_generator/marching_squares/contours_builder.hpp"
#include "topography_generator/utils/values_provider.hpp"

namespace topography_generator
{
template <typename ValueType>
class Square
{
public:
  Square(ms::LatLon const & leftBottom,
         ms::LatLon const & rightTop,
         ValueType minValue, ValueType valueStep,
         ValuesProvider<ValueType> & valuesProvider,
         std::string const & debugId)
    : m_minValue(minValue)
    , m_valueStep(valueStep)
    , m_left(leftBottom.m_lon)
    , m_right(rightTop.m_lon)
    , m_bottom(leftBottom.m_lat)
    , m_top(rightTop.m_lat)
    , m_debugId(debugId)
  {
    static_assert(std::is_integral<ValueType>::value, "Only integral types are supported.");

    m_valueLB = GetValue(leftBottom, valuesProvider);
    m_valueLT = GetValue(ms::LatLon(m_top, m_left), valuesProvider);
    m_valueRT = GetValue(ms::LatLon(m_top, m_right), valuesProvider);
    m_valueRB = GetValue(ms::LatLon(m_bottom, m_right), valuesProvider);
  }

  void GenerateSegments(ContoursBuilder & builder)
  {
    if (!m_isValid)
      return;

    ValueType minVal = std::min(m_valueLB, std::min(m_valueLT, std::min(m_valueRT, m_valueRB)));
    ValueType maxVal = std::max(m_valueLB, std::max(m_valueLT, std::max(m_valueRT, m_valueRB)));

    ToLevelsRange(m_valueStep, minVal, maxVal);

    CHECK_GREATER_OR_EQUAL(minVal, m_minValue, (m_debugId));

    for (auto val = minVal; val < maxVal; val += m_valueStep)
      AddSegments(val, (val - m_minValue) / m_valueStep, builder);
  }

  static void ToLevelsRange(ValueType step, ValueType & minVal, ValueType & maxVal)
  {
    if (minVal > 0)
      minVal = step * ((minVal + step - 1) / step);
    else
      minVal = step * (minVal / step);

    if (maxVal > 0)
      maxVal = step * ((maxVal + step) / step);
    else
      maxVal = step * ((maxVal + 1) / step);
  }

private:
  enum class Rib
  {
    None,
    Left,
    Top,
    Right,
    Bottom,
    Unclear,
  };

  ValueType GetValue(ms::LatLon const & pos, ValuesProvider<ValueType> & valuesProvider)
  {
    // If a contour goes right through the corner of the square false segments can be generated.
    // Shift the value slightly from the corner.
    ValueType val = valuesProvider.GetValue(pos);
    if (val == valuesProvider.GetInvalidValue())
    {
      LOG(LWARNING, ("Invalid value at the position", pos, m_debugId));
      m_isValid = false;
      return val;
    }

    if (abs(val) % m_valueStep == 0)
      return val + 1;
    return val;
  }

  void AddSegments(ValueType val, uint16_t ind, ContoursBuilder & builder)
  {
    // Segment is a vector directed so that higher values is on the right.
    static const std::pair<Rib, Rib> intersectedRibs[] =
      {
        {Rib::None, Rib::None},       // 0000
        {Rib::Left, Rib::Bottom},     // 0001
        {Rib::Top, Rib::Left},        // 0010
        {Rib::Top, Rib::Bottom},      // 0011
        {Rib::Right, Rib::Top},       // 0100
        {Rib::Unclear, Rib::Unclear}, // 0101
        {Rib::Right, Rib::Left},      // 0110
        {Rib::Right, Rib::Bottom},    // 0111
        {Rib::Bottom, Rib::Right},    // 1000
        {Rib::Left, Rib::Right},      // 1001
        {Rib::Unclear, Rib::Unclear}, // 1010
        {Rib::Top, Rib::Right},       // 1011
        {Rib::Bottom, Rib::Top},      // 1100
        {Rib::Left, Rib::Top},        // 1101
        {Rib::Bottom, Rib::Left},     // 1110
        {Rib::None, Rib::None},       // 1111
      };

    uint8_t const pattern =  (m_valueLB > val ? 1u : 0u) | ((m_valueLT > val ? 1u : 0u) << 1u) |
      ((m_valueRT > val ? 1u : 0u) << 2u) | ((m_valueRB > val ? 1u : 0u) << 3u);

    auto const ribs = intersectedRibs[pattern];

    if (ribs.first == Rib::None)
      return;

    if (ribs.first != Rib::Unclear)
    {
      builder.AddSegment(ind, InterpolatePoint(ribs.first, val), InterpolatePoint(ribs.second, val));
    }
    else
    {
      auto const leftPos = InterpolatePoint(Rib::Left, val);
      auto const rightPos = InterpolatePoint(Rib::Right, val);
      auto const bottomPos = InterpolatePoint(Rib::Bottom, val);
      auto const topPos = InterpolatePoint(Rib::Top, val);

      ValueType const avVal = (m_valueLB + m_valueLT + m_valueRT + m_valueRB) / 4;
      if (avVal > val)
      {
        if (m_valueLB > val)
        {
          builder.AddSegment(ind, leftPos, topPos);
          builder.AddSegment(ind, rightPos, bottomPos);
        }
        else
        {
          builder.AddSegment(ind, bottomPos, leftPos);
          builder.AddSegment(ind, topPos, rightPos);
        }
      }
      else
      {
        if (m_valueLB > val)
        {
          builder.AddSegment(ind, leftPos, bottomPos);
          builder.AddSegment(ind, rightPos, topPos);
        }
        else
        {
          builder.AddSegment(ind, topPos, leftPos);
          builder.AddSegment(ind, bottomPos, rightPos);
        }
      }
    }
  }

  ms::LatLon InterpolatePoint(Square::Rib rib, ValueType val)
  {
    double val1;
    double val2;
    double lat;
    double lon;

    switch (rib)
    {
    case Rib::Left:
      val1 = static_cast<double>(m_valueLB);
      val2 = static_cast<double>(m_valueLT);
      lon = m_left;
      break;
    case Rib::Right:
      val1 = static_cast<double>(m_valueRB);
      val2 = static_cast<double>(m_valueRT);
      lon = m_right;
      break;
    case Rib::Top:
      val1 = static_cast<double>(m_valueLT);
      val2 = static_cast<double>(m_valueRT);
      lat = m_top;
      break;
    case Rib::Bottom:
      val1 = static_cast<double>(m_valueLB);
      val2 = static_cast<double>(m_valueRB);
      lat = m_bottom;
      break;
    default:
      UNREACHABLE();
    }

    CHECK_NOT_EQUAL(val, val2, (m_debugId));
    double const coeff = (val1 - val) / (val - val2);

    switch (rib)
    {
    case Rib::Left:
    case Rib::Right:
      lat = (m_bottom + m_top * coeff) / (1 + coeff);
      break;
    case Rib::Bottom:
    case Rib::Top:
      lon = (m_left + m_right * coeff) / (1 + coeff);
      break;
    default:
      UNREACHABLE();
    }

    return {lat, lon};
  }

  ValueType m_minValue;
  ValueType m_valueStep;

  double m_left;
  double m_right;
  double m_bottom;
  double m_top;

  ValueType m_valueLB;
  ValueType m_valueLT;
  ValueType m_valueRT;
  ValueType m_valueRB;

  bool m_isValid = true;
  std::string m_debugId;
};
}  // topography_generator

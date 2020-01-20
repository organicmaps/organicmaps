#pragma once

#include "topography_generator/marching_squares/contours_builder.hpp"
#include "topography_generator/utils/values_provider.hpp"

namespace topography_generator
{
template <typename ValueType>
class Square
{
public:
  Square(ms::LatLon const & leftBottom, double size,
         ValueType minValue, ValueType valueStep,
         ValuesProvider<ValueType> & valuesProvider)
    : m_minValue(minValue)
    , m_valueStep(valueStep)
    , m_left(leftBottom.m_lon)
    , m_right(leftBottom.m_lon + size)
    , m_bottom(leftBottom.m_lat)
    , m_top(leftBottom.m_lat + size)
  {
    static_assert(std::is_integral<ValueType>::value, "Only integral types are supported.");

    m_valueLB = GetValue(leftBottom, valuesProvider);
    m_valueLT = GetValue(ms::LatLon(m_top, m_left), valuesProvider);
    m_valueRT = GetValue(ms::LatLon(m_top, m_right), valuesProvider);
    m_valueRB = GetValue(ms::LatLon(m_bottom, m_right), valuesProvider);
  }

  void GenerateSegments(ContoursBuilder & builder)
  {
    ValueType minAlt = std::min(m_valueLB, std::min(m_valueLT, std::min(m_valueRT, m_valueRB)));
    ValueType maxAlt = std::max(m_valueLB, std::max(m_valueLT, std::max(m_valueRT, m_valueRB)));

    if (minAlt > 0)
      minAlt = m_valueStep * ((minAlt + m_valueStep - 1) / m_valueStep);
    else
      minAlt = m_valueStep * (minAlt / m_valueStep);
    if (maxAlt > 0)
      maxAlt = m_valueStep * ((maxAlt + m_valueStep) / m_valueStep);
    else
      maxAlt = m_valueStep * (maxAlt / m_valueStep);

    CHECK_GREATER_OR_EQUAL(minAlt, m_minValue, ());

    for (auto alt = minAlt; alt < maxAlt; alt += m_valueStep)
      AddSegments(alt, (alt - m_minValue) / m_valueStep, builder);
  }

//private:
  enum class Rib
  {
    None,
    Left,
    Top,
    Right,
    Bottom,
    Unclear,
  };

  ValueType GetValue(ms::LatLon const & pos, ValuesProvider<ValueType> & valuesProvider) const
  {
    ValueType val = valuesProvider.GetValue(pos);
    if (val != valuesProvider.GetInvalidValue() && (val % m_valueStep == 0))
      return val + 1;
    return val;
  }

  void AddSegments(ValueType val, uint16_t ind, ContoursBuilder & builder)
  {
    // Segment is a vector directed so that higher values is on the right.
    std::pair<Rib, Rib> intersectedRibs[] =
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

    auto ribs = intersectedRibs[pattern];

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

  ms::LatLon InterpolatePoint(Square::Rib rib, ValueType alt)
  {
    double alt1;
    double alt2;
    double lat;
    double lon;

    switch (rib)
    {
    case Rib::Left:
      alt1 = static_cast<double>(m_valueLB);
      alt2 = static_cast<double>(m_valueLT);
      lon = m_left;
      break;
    case Rib::Right:
      alt1 = static_cast<double>(m_valueRB);
      alt2 = static_cast<double>(m_valueRT);
      lon = m_right;
      break;
    case Rib::Top:
      alt1 = static_cast<double>(m_valueLT);
      alt2 = static_cast<double>(m_valueRT);
      lat = m_top;
      break;
    case Rib::Bottom:
      alt1 = static_cast<double>(m_valueLB);
      alt2 = static_cast<double>(m_valueRB);
      lat = m_bottom;
      break;
    default:
      UNREACHABLE();
    }

    CHECK_NOT_EQUAL(alt, alt2, ());
    double const coeff = (alt1 - alt) / (alt - alt2);

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
};
}  // topography_generator

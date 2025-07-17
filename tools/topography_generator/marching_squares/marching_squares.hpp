#pragma once

#include "topography_generator/marching_squares/contours_builder.hpp"
#include "topography_generator/marching_squares/square.hpp"
#include "topography_generator/utils/contours.hpp"

#include "base/logging.hpp"

namespace topography_generator
{
template <typename ValueType>
class MarchingSquares
{
public:
  MarchingSquares(ms::LatLon const & leftBottom, ms::LatLon const & rightTop,
                  double step, ValueType valueStep, ValuesProvider<ValueType> & valuesProvider,
                  std::string const & debugId)
    : m_leftBottom(leftBottom)
    , m_rightTop(rightTop)
    , m_step(step)
    , m_valueStep(valueStep)
    , m_valuesProvider(valuesProvider)
    , m_debugId(debugId)
  {
    CHECK_GREATER(m_rightTop.m_lon, m_leftBottom.m_lon, ());
    CHECK_GREATER(m_rightTop.m_lat, m_leftBottom.m_lat, ());

    m_stepsCountLon = static_cast<size_t>((m_rightTop.m_lon - m_leftBottom.m_lon) / step);
    m_stepsCountLat = static_cast<size_t>((m_rightTop.m_lat - m_leftBottom.m_lat) / step);

    CHECK_GREATER(m_stepsCountLon, 0, ());
    CHECK_GREATER(m_stepsCountLat, 0, ());
  }

  void GenerateContours(Contours<ValueType> & result)
  {
    ScanValuesInRect(result.m_minValue, result.m_maxValue, result.m_invalidValuesCount);
    result.m_valueStep = m_valueStep;

    auto const levelsCount = static_cast<size_t>(result.m_maxValue - result.m_minValue) / m_valueStep;
    if (levelsCount == 0)
    {
      LOG(LINFO, ("Contours can't be generated: min and max values are equal:", result.m_minValue));
      return;
    }

    ContoursBuilder contoursBuilder(levelsCount, m_debugId);

    for (size_t i = 0; i < m_stepsCountLat; ++i)
    {
      contoursBuilder.BeginLine();
      for (size_t j = 0; j < m_stepsCountLon; ++j)
      {
        auto const leftBottom = ms::LatLon(m_leftBottom.m_lat + m_step * i,
                                           m_leftBottom.m_lon + m_step * j);
        // Use std::min to prevent floating-point number precision error.
        auto const rightTop = ms::LatLon(std::min(leftBottom.m_lat + m_step, m_rightTop.m_lat),
                                         std::min(leftBottom.m_lon + m_step, m_rightTop.m_lon));

        Square<ValueType> square(leftBottom, rightTop, result.m_minValue, m_valueStep,
                                 m_valuesProvider, m_debugId);
        square.GenerateSegments(contoursBuilder);
      }
      auto const isLastLine = i == m_stepsCountLat - 1;
      contoursBuilder.EndLine(isLastLine);
    }

    contoursBuilder.GetContours(result.m_minValue, result.m_valueStep, result.m_contours);
  }

private:
  void ScanValuesInRect(ValueType & minValue, ValueType & maxValue, size_t & invalidValuesCount) const
  {
    minValue = maxValue = m_valuesProvider.GetValue(m_leftBottom);
    invalidValuesCount = 0;

    for (size_t i = 0; i <= m_stepsCountLat; ++i)
    {
      for (size_t j = 0; j <= m_stepsCountLon; ++j)
      {
        auto const pos = ms::LatLon(m_leftBottom.m_lat + m_step * i,
                                    m_leftBottom.m_lon + m_step * j);
        auto const value = m_valuesProvider.GetValue(pos);
        if (value == m_valuesProvider.GetInvalidValue())
        {
          ++invalidValuesCount;
          continue;
        }
        if (value < minValue)
          minValue = value;
        if (value > maxValue)
          maxValue = value;
      }
    }

    if (invalidValuesCount > 0)
      LOG(LWARNING, ("Tile", m_debugId, "contains", invalidValuesCount, "invalid values."));

    Square<ValueType>::ToLevelsRange(m_valueStep, minValue, maxValue);

    CHECK_GREATER_OR_EQUAL(maxValue, minValue, (m_debugId));
  }

  ms::LatLon const m_leftBottom;
  ms::LatLon const m_rightTop;
  double const m_step;
  ValueType const m_valueStep;
  ValuesProvider<ValueType> & m_valuesProvider;

  size_t m_stepsCountLon;
  size_t m_stepsCountLat;

  std::string m_debugId;
};
}  // namespace topography_generator

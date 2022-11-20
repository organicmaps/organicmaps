#pragma once

#include "generator/feature_helpers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <vector>
#include <unordered_map>

namespace topography_generator
{
using Contour = std::vector<m2::PointD>;

template <typename ValueType>
struct Contours
{
  enum class Version : uint8_t
  {
    V0 = 0,
    Latest = V0
  };

  std::unordered_map<ValueType, std::vector<Contour>> m_contours;
  ValueType m_minValue = 0;
  ValueType m_maxValue = 0;
  ValueType m_valueStep = 0;
  size_t m_invalidValuesCount = 0; // for debug purpose only.
};

template <typename ValueType>
void CropContours(m2::RectD & rect, std::vector<m2::RegionD> & regions, size_t maxLength,
                  size_t valueStepFactor, Contours<ValueType> & contours)
{
  static_assert(std::is_integral<ValueType>::value, "Only integral types are supported.");

  contours.m_minValue = std::numeric_limits<ValueType>::max();
  contours.m_maxValue = std::numeric_limits<ValueType>::min();

  auto it = contours.m_contours.begin();
  while (it != contours.m_contours.end())
  {
    std::vector<Contour> levelCroppedContours;

    if (it->first % static_cast<ValueType>(contours.m_valueStep * valueStepFactor) == 0)
    {
      for (auto const & contour : it->second)
      {
        Contour cropped;
        cropped.reserve(contour.size());
        for (auto const & pt : contour)
        {
          cropped.push_back(pt);
          auto const isInside = rect.IsPointInside(pt) && RegionsContain(regions, pt);
          if (!isInside || cropped.size() == maxLength)
          {
            if (cropped.size() > 1)
              levelCroppedContours.emplace_back(std::move(cropped));
            cropped.clear();
            if (isInside)
              cropped.push_back(pt);
          }
        }
        if (cropped.size() > 1)
          levelCroppedContours.emplace_back(std::move(cropped));
      }
    }
    it->second = std::move(levelCroppedContours);

    if (!it->second.empty())
    {
      contours.m_minValue = std::min(it->first, contours.m_minValue);
      contours.m_maxValue = std::max(it->first, contours.m_maxValue);
      ++it;
    }
    else
    {
      it = contours.m_contours.erase(it);
    }
  }
}

template <typename ValueType>
void SimplifyContours(int simplificationZoom, Contours<ValueType> & contours)
{
  for (auto & levelContours : contours.m_contours)
  {
    for (auto & contour : levelContours.second)
    {
      std::vector<m2::PointD> contourSimple;
      feature::SimplifyPoints(m2::SquaredDistanceFromSegmentToPoint(),
                              simplificationZoom, contour, contourSimple);
      contour = std::move(contourSimple);
    }
  }
}
}  // namespace topography_generator

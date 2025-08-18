#pragma once

#include "topography_generator/utils/contours.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include <algorithm>
#include <deque>
#include <list>
#include <unordered_map>
#include <vector>

namespace topography_generator
{
class ContoursBuilder
{
public:
  ContoursBuilder(size_t levelsCount, std::string const & debugId);

  void AddSegment(size_t levelInd, ms::LatLon const & beginPos, ms::LatLon const & endPos);
  void BeginLine();
  void EndLine(bool finalLine);

  template <typename ValueType>
  void GetContours(ValueType minValue, ValueType valueStep,
                   std::unordered_map<ValueType, std::vector<Contour>> & contours)
  {
    contours.clear();
    for (size_t i = 0; i < m_finalizedContours.size(); ++i)
    {
      auto const levelValue = minValue + i * valueStep;
      auto const & contoursList = m_finalizedContours[i];
      for (auto const & contour : contoursList)
      {
        Contour contourMerc;
        contourMerc.reserve(contour.size());
        std::transform(contour.begin(), contour.end(), std::back_inserter(contourMerc),
                       [](ms::LatLon const & pt) { return mercator::FromLatLon(pt); });

        contours[levelValue].emplace_back(std::move(contourMerc));
      }
    }
  }

private:
  using ContourRaw = std::deque<ms::LatLon>;
  using ContoursList = std::list<ContourRaw>;

  struct ActiveContour
  {
    explicit ActiveContour(ContourRaw && isoline) : m_countour(std::move(isoline)) {}

    ContourRaw m_countour;
    bool m_active = true;
  };
  using ActiveContoursList = std::list<ActiveContour>;
  using ActiveContourIter = ActiveContoursList::iterator;

  ActiveContourIter FindContourWithStartPoint(size_t levelInd, ms::LatLon const & pos);
  ActiveContourIter FindContourWithEndPoint(size_t levelInd, ms::LatLon const & pos);

  size_t const m_levelsCount;

  std::vector<ContoursList> m_finalizedContours;
  std::vector<ActiveContoursList> m_activeContours;

  std::string m_debugId;
};
}  // namespace topography_generator

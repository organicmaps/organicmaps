#pragma once

#include "search/hotels_filter.hpp"
#include "search/mode.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

namespace search
{
class Results;

struct SearchParams
{
  using OnStarted = function<void()>;
  using OnResults = function<void(Results const &)>;

  void SetPosition(ms::LatLon const & position) { m_position = position; }
  bool IsValidPosition() const { return static_cast<bool>(m_position); }
  m2::PointD GetPositionMercator() const;
  ms::LatLon GetPositionLatLon() const;

  bool IsEqualCommon(SearchParams const & rhs) const;

  void Clear() { m_query.clear(); }

  OnStarted m_onStarted;
  OnResults m_onResults;

  std::string m_query;
  std::string m_inputLocale;

  boost::optional<ms::LatLon> m_position;
  m2::RectD m_viewport;

  // A minimum distance between search results in meters, needed for
  // pre-ranking of viewport search results.
  double m_minDistanceOnMapBetweenResults = 0.0;

  Mode m_mode = Mode::Everywhere;
  bool m_forceSearch = false;
  bool m_suggestsEnabled = true;

  std::shared_ptr<hotels_filter::Rule> m_hotelsFilter;
  bool m_cianMode = false;
};

std::string DebugPrint(SearchParams const & params);
}  // namespace search

#pragma once

#include "search/hotels_filter.hpp"
#include "search/mode.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace search
{
class Results;

class SearchParams
{
public:
  using TOnStarted = function<void()>;
  using TOnResults = function<void(Results const &)>;

  void SetPosition(double lat, double lon);
  m2::PointD GetPositionMercator() const;
  ms::LatLon GetPositionLatLon() const;

  inline bool IsValidPosition() const { return m_validPos; }
  bool IsEqualCommon(SearchParams const & rhs) const;
  inline void Clear() { m_query.clear(); }

  TOnStarted m_onStarted;
  TOnResults m_onResults;

  string m_query;
  string m_inputLocale;

  // A minimum distance between search results in meters, needed for
  // pre-ranking of viewport search results.
  double m_minDistanceOnMapBetweenResults = 0.0;

  Mode m_mode = Mode::Everywhere;
  bool m_forceSearch = false;
  bool m_suggestsEnabled = true;

  shared_ptr<hotels_filter::Rule> m_hotelsFilter;

  friend string DebugPrint(SearchParams const & params);

private:
  double m_lat = 0.0;
  double m_lon = 0.0;

  bool m_validPos = false;
};
}  // namespace search

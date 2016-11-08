#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "indexer/index.hpp"

#include "std/set.hpp"

namespace df
{
class DrapeEngine;
}  // namespace df

class TrafficManager
{
public:
  struct MyPosition
  {
    m2::PointD m_position = m2::PointD(0.0, 0.0);
    bool m_knownPosition = false;

    MyPosition() = default;
    MyPosition(m2::PointD const & position)
      : m_position(position),
        m_knownPosition(true)
    {}
  };

  TrafficManager(Index const & index)
    : m_index(index)
  {}

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

private:
  void CalculateSegmentsGeometry(traffic::TrafficInfo const & trafficInfo,
                                 df::TrafficSegmentsGeometry & output) const;
  void CalculateSegmentsColoring(traffic::TrafficInfo const & trafficInfo,
                                 df::TrafficSegmentsColoring & output) const;

  Index const & m_index;
  ref_ptr<df::DrapeEngine> m_drapeEngine;
  m2::PointD m_myPosition;
  set<MwmSet::MwmId> m_mwmIds;
};

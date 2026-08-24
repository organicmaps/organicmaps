#pragma once

#include "traffic/traffic_info.hpp"

#include "geometry/rect2d.hpp"

#include "indexer/mwm_set.hpp"

#include <map>

namespace traffic
{
// An abstract source of live traffic data that is independent of the MWM
// format, e.g. an HTTP road-speed API. FetchColorings() is called on the
// traffic manager's worker thread only, so implementations need no
// synchronization unless they are shared with other threads.
class TrafficProvider
{
public:
  struct FetchResult
  {
    bool m_ok = false;
    bool m_hasFreshData = false;
  };

  virtual ~TrafficProvider() = default;
  virtual FetchResult FetchColorings(m2::RectD const & area,
                                     std::map<MwmSet::MwmId, TrafficInfo::Coloring> & colorings) = 0;
};
}  // namespace traffic

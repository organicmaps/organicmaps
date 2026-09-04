#pragma once

#include "map/traffic_provider.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace traffic
{
// Retrieves live road-speed data from an HTTP vector-flow-tile service
// (XYZ tile scheme, Mapbox Vector Tile protobuf payload, e.g. TomTom Traffic
// Flow) and converts it to Organic Maps colorings by snapping the reported
// road polylines to routable segments of the local maps.
class FlowTilesProvider final : public TrafficProvider
{
public:
  FlowTilesProvider(DataSource const & dataSource, std::string apiKey);

  FetchResult FetchColorings(m2::RectD const & area,
                             std::map<MwmSet::MwmId, TrafficInfo::Coloring> & colorings) override;

private:
  struct TileEntry
  {
    std::chrono::steady_clock::time_point m_fetchedAt;
    std::map<MwmSet::MwmId, TrafficInfo::Coloring> m_colorings;
  };

  DataSource const & m_dataSource;
  std::string m_apiKey;

  // Recently downloaded tiles keyed by TileKey(). Lets pans and zooms reuse
  // still-fresh tiles instead of refetching the whole viewport, and keeps
  // FetchColorings() results complete for the requested area so callers may
  // safely replace previously applied colorings with them.
  std::map<uint64_t, TileEntry> m_tileCache;
};

void AddToColoring(TrafficInfo::Coloring & coloring, TrafficInfo::RoadSegmentId const & id, SpeedGroup group);
void MergeColoring(TrafficInfo::Coloring & dst, TrafficInfo::Coloring const & src);

SpeedGroup SpeedGroupFromFlow(double relativeSpeed, bool closed);

std::unique_ptr<TrafficProvider> CreateFlowTilesProvider(DataSource const & dataSource, std::string apiKey);
}  // namespace traffic

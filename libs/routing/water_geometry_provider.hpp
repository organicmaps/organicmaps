#pragma once

#include "routing/checkpoints.hpp"
#include "routing/router_delegate.hpp"
#include "routing/water_geometry.hpp"

#include "indexer/data_source.hpp"
#include "indexer/water_geometry.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace routing
{
class WaterGeometryProvider
{
public:
  using CoverageFn = std::function<bool(std::string const &, m2::RectD const &)>;
  using MultiCoverageFn = std::function<bool(std::vector<std::string> const &, m2::RectD const &)>;
  explicit WaterGeometryProvider(DataSource const & dataSource, CoverageFn coverage = {},
                                 MultiCoverageFn multiCoverage = {})
    : m_dataSource(dataSource)
    , m_coverage(std::move(coverage))
    , m_multiCoverage(std::move(multiCoverage))
  {}
  water::Geometry GetGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate,
                              double extraMargin = 0.0, std::optional<m2::RectD> requestedBounds = {},
                              unsigned corridorRefinement = 0);
  water::Geometry ExpandGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate,
                                 water::Geometry const & previous, std::array<bool, 4> const & frontiers,
                                 bool adaptive);
  using RectCoverageFn = std::function<bool(m2::RectD const &)>;
  static void SetCoverage(water::Geometry & geometry, std::vector<m2::PointD> const & checkpoints,
                          RectCoverageFn covered, water::CancelledFn const & cancelled,
                          std::vector<m2::RectD> const & regions = {});
  // Also used by deterministic section/provider integration tests.
  static water::Geometry Extract(::water::Reader const & reader, m2::RectD const & bounds,
                                 water::CancelledFn const & cancelled);
  // Standard MWM display geometry has no source node identities or complete water coverage.
  static water::Geometry ExtractFeatures(DataSource const & dataSource, MwmSet::MwmId const & id,
                                         m2::RectD const & bounds, water::CancelledFn const & cancelled);

  static water::Geometry ExtractFeatures(DataSource const & dataSource, std::vector<MwmSet::MwmId> const & ids,
                                         m2::RectD const & bounds, water::CancelledFn const & cancelled);

  using TileLoader = std::function<water::Geometry(m2::RectD const &)>;
  // Candidate regions only: final routing never has to visit the coarse search nodes.
  static std::vector<m2::RectD> FindWaterCorridor(std::vector<m2::PointD> const & points, TileLoader const & loader,
                                                  water::CancelledFn const & cancelled, unsigned refinement = 0);

private:
  static water::Geometry ExtractFeatures(DataSource const & dataSource, std::vector<MwmSet::MwmId> const & ids,
                                         m2::RectD const & bounds, water::CancelledFn const & cancelled,
                                         std::vector<m2::RectD> const & regions);
  DataSource const & m_dataSource;
  CoverageFn m_coverage;
  MultiCoverageFn m_multiCoverage;
  std::map<MwmSet::MwmId, std::unique_ptr<::water::Reader>> m_readers;
};
}  // namespace routing

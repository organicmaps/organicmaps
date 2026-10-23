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
  explicit WaterGeometryProvider(DataSource const & dataSource, CoverageFn coverage = {})
    : m_dataSource(dataSource)
    , m_coverage(std::move(coverage))
  {}
  water::Geometry GetGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate);
  // Also used by deterministic section/provider integration tests.
  static water::Geometry Extract(::water::Reader const & reader, m2::RectD const & bounds,
                                 water::CancelledFn const & cancelled);
  // Standard MWM display geometry has no source node identities or complete water coverage.
  static water::Geometry ExtractFeatures(DataSource const & dataSource, MwmSet::MwmId const & id,
                                         m2::RectD const & bounds, water::CancelledFn const & cancelled);

private:
  DataSource const & m_dataSource;
  CoverageFn m_coverage;
  std::map<MwmSet::MwmId, std::unique_ptr<::water::Reader>> m_readers;
};
}  // namespace routing

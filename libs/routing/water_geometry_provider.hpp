#pragma once

#include "routing/checkpoints.hpp"
#include "routing/router_delegate.hpp"
#include "routing/water_geometry.hpp"

#include "indexer/data_source.hpp"
#include "indexer/water_geometry.hpp"

#include <map>
#include <memory>

namespace routing
{
class WaterGeometryProvider
{
public:
  explicit WaterGeometryProvider(DataSource const & dataSource) : m_dataSource(dataSource) {}
  water::Geometry GetGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate);
  // Also used by deterministic section/provider integration tests.
  static water::Geometry Extract(::water::Reader const & reader, m2::RectD const & bounds,
                                 water::CancelledFn const & cancelled);

private:
  DataSource const & m_dataSource;
  std::map<MwmSet::MwmId, std::unique_ptr<::water::Reader>> m_readers;
};
}  // namespace routing

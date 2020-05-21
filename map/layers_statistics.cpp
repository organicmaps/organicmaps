#include "map/layers_statistics.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace
{
std::string ToString(LayersStatistics::Status status)
{
  switch (status)
  {
  case LayersStatistics::Status::Success: return "success";
  case LayersStatistics::Status::Error: return "error";
  case LayersStatistics::Status::Unavailable: return "unavailable";
  }
  UNREACHABLE();
}

std::string ToString(LayersStatistics::LayerItemType itemType)
{
  switch (itemType)
  {
  case LayersStatistics::LayerItemType::Point: return "point";
  case LayersStatistics::LayerItemType::Cluster: return "cluster";
  }
  UNREACHABLE();
}
}  // namespace

LayersStatistics::LayersStatistics(std::string const & layerName)
  : m_layerName(layerName)
{
}

void LayersStatistics::LogActivate(Status status,
                                   std::set<int64_t> const & mwmVersions /* = {} */) const
{
  alohalytics::TStringMap params = {{"name", m_layerName}, {"status", ToString(status)}};

  if (!mwmVersions.empty())
    params.emplace("dataversion", strings::JoinAny(mwmVersions));

  alohalytics::LogEvent("Map_Layers_activate", params);
}

void LayersStatistics::LogItemSelected(LayerItemType itemType) const
{
  alohalytics::LogEvent("Map_Layers_item_selected",
                        {{"name", m_layerName}, {"type", ToString(itemType)}});
}

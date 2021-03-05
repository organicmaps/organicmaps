#include "map/layers_statistics.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

LayersStatistics::LayersStatistics(std::string const & layerName)
  : m_layerName(layerName)
{
}

void LayersStatistics::LogActivate(Status status,
                                   std::set<int64_t> const & mwmVersions /* = {} */,
                                   InitType initType /* = InitType::User */) const
{
}

void LayersStatistics::LogItemSelected(LayerItemType itemType) const
{
}

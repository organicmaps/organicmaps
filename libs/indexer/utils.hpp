#pragma once

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include <memory>
#include <vector>

namespace indexer
{
MwmSet::MwmHandle FindWorld(DataSource const & dataSource, std::vector<std::shared_ptr<MwmInfo>> const & infos);
MwmSet::MwmHandle FindWorld(DataSource const & dataSource);
}  // namespace indexer

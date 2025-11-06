#pragma once

#include "indexer/feature_meta.hpp"

#include <map>
#include <string>

namespace feature
{
void ReadRegionData(std::string regionID, RegionData & data);

struct AllRegionsData
{
  std::map<std::string, RegionData, std::less<>> m_cont;

  RegionData const * Get(std::string_view regionID) const;

  void Finish();
};

void ReadAllRegions(AllRegionsData & allData);
}  // namespace feature

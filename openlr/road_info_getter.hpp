#pragma once

#include "openlr/openlr_model.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <map>

class Classificator;
class DataSource;

namespace feature
{
class TypesHolder;
}

namespace openlr
{
class RoadInfoGetter final
{
public:
  struct RoadInfo
  {
    explicit RoadInfo(FeatureType & ft);

    ftypes::HighwayClass m_hwClass = ftypes::HighwayClass::Undefined;
    bool m_link = false;
    bool m_oneWay = false;
    bool m_isRoundabout = false;
  };

  explicit RoadInfoGetter(DataSource const & dataSource);

  RoadInfo Get(FeatureID const & fid);

private:
  DataSource const & m_dataSource;
  std::map<FeatureID, RoadInfo> m_cache;
};
}  // namespace openlr

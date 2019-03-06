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
    FunctionalRoadClass m_frc = FunctionalRoadClass::NotAValue;
    FormOfWay m_fow = FormOfWay::NotAValue;
    ftypes::HighwayClass m_hwClass = ftypes::HighwayClass::Undefined;
    bool m_link = false;
    bool m_oneWay = false;
  };

  explicit RoadInfoGetter(DataSource const & dataSource);

  RoadInfo Get(FeatureID const & fid);

 private:
  FormOfWay GetFormOfWay(feature::TypesHolder const & types) const;

  DataSource const & m_dataSource;
  std::map<FeatureID, RoadInfo> m_cache;
};
}  // namespace openlr

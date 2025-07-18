#pragma once

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "base/macros.hpp"

#include <map>
#include <memory>
#include <string>

class DataSource;
struct FeatureID;

// *NOTE* This class IS NOT thread-safe.
class CachingRankTableLoader
{
public:
  CachingRankTableLoader(DataSource const & dataSource, std::string const & sectionName);

  /// @return 0 if there is no rank for feature.
  uint8_t Get(FeatureID const & featureId) const;
  void OnMwmDeregistered(platform::LocalCountryFile const & localFile);

private:
  DataSource const & m_dataSource;
  std::string const m_sectionName;
  mutable std::map<MwmSet::MwmId, std::unique_ptr<search::RankTable>> m_deserializers;

  DISALLOW_COPY(CachingRankTableLoader);
};

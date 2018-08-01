#pragma once

#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "base/macros.hpp"

#include <map>
#include <memory>
#include <mutex>

class DataSource;
struct FeatureID;

// *NOTE* This class IS NOT thread-safe.
class CachingPopularityLoader
{
public:
  explicit CachingPopularityLoader(DataSource const & dataSource);
  uint8_t Get(FeatureID const & featureId) const;
  void OnMwmDeregistered(platform::LocalCountryFile const & localFile);

private:
  DataSource const & m_dataSource;
  mutable std::map<MwmSet::MwmId, std::unique_ptr<search::RankTable const> const> m_deserializers;

  DISALLOW_COPY(CachingPopularityLoader);
};

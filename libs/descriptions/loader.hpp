#pragma once

#include "descriptions/serdes.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include <map>
#include <mutex>
#include <string>

class DataSource;

namespace descriptions
{
// *NOTE* This class IS thread-safe.
class Loader
{
public:
  explicit Loader(DataSource const & dataSource) : m_dataSource(dataSource) {}

  std::string GetWikiDescription(FeatureID const & featureId, LangsBufferT const & langPriority);

  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);
  void Clear();

private:
  DataSource const & m_dataSource;
  std::map<MwmSet::MwmId, Deserializer> m_deserializers;
  std::mutex m_mutex;
};
}  // namespace descriptions

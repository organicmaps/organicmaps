#pragma once

#include "descriptions/serdes.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class DataSource;

namespace descriptions
{
// *NOTE* This class IS thread-safe.
class Loader
{
public:
  explicit Loader(DataSource const & dataSource) : m_dataSource(dataSource) {}

  bool GetDescription(FeatureID const & featureId, std::vector<int8_t> const & langPriority,
                      std::string & description);

private:
  struct Entry
  {
    std::mutex m_mutex;
    Deserializer m_deserializer;
  };

  using EntryPtr = std::shared_ptr<Entry>;

  DataSource const & m_dataSource;
  std::map<MwmSet::MwmId, EntryPtr> m_deserializers;
  std::mutex m_mutex;
};
}  // namespace descriptions

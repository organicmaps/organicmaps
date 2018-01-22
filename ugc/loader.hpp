#pragma once

#include "ugc/binary/serdes.hpp"
#include "ugc/types.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>
#include <mutex>

class Index;
struct FeatureID;

namespace ugc
{
// *NOTE* This class IS thread-safe.
class Loader
{
public:
  Loader(Index const & index);
  UGC GetUGC(FeatureID const & featureId);

private:
  struct Entry
  {
    std::mutex m_mutex;
    binary::UGCDeserializer m_deserializer;
  };

  using EntryPtr = std::shared_ptr<Entry>;

  Index const & m_index;
  std::map<MwmSet::MwmId, EntryPtr> m_deserializers;
  std::mutex m_mutex;
};
}  // namespace ugc

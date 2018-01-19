#pragma once

#include "ugc/binary/serdes.hpp"
#include "ugc/types.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
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
  Index const & m_index;
  std::map<MwmSet::MwmId, binary::UGCDeserializer> m_deserializers;
  std::mutex m_mutex;
};
}  // namespace ugc

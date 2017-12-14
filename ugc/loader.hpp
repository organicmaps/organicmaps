#pragma once

#include "ugc/binary/serdes.hpp"
#include "ugc/types.hpp"

#include "indexer/mwm_set.hpp"

class Index;
struct FeatureID;

namespace ugc
{
class Loader
{
public:
  Loader(Index const & index);
  UGC GetUGC(FeatureID const & featureId);

private:
  Index const & m_index;
  MwmSet::MwmId m_currentMwmId;
  binary::UGCDeserializer m_d;
};
}  // namespace ugc

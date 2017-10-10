#pragma once

#include "ugc/binary/serdes.hpp"
#include "ugc/types.hpp"

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
  binary::UGCDeserializer m_d;
};
}  // namespace ugc

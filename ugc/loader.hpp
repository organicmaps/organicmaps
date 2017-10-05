#pragma once

#include "ugc/binary/serdes.hpp"

class Index;
struct FeatureID;

namespace ugc
{
struct UGC;

class Loader
{
public:
  Loader(Index const & index);
  void GetUGC(FeatureID const & featureId, UGC & ugc);

private:
    Index const & m_index;
    binary::UGCDeserializer m_d;
};
}  // namespace ugc

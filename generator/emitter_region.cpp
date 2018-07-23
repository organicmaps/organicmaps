#include "generator/emitter_region.hpp"

#include "generator/feature_builder.hpp"

namespace generator
{
EmitterRegion::EmitterRegion(feature::GenerateInfo const & info) :
  m_regionGenerator(new RegionGenerator(info))
{
}

void EmitterRegion::GetNames(vector<string> & names) const
{
  if (m_regionGenerator)
    names = m_regionGenerator->Parent().Names();
  else
    names.clear();
}

void EmitterRegion::operator()(FeatureBuilder1 & fb)
{
  // This is a dirty hack. Without it, he will fall in the test of equality FeatureBuilder1 in
  // 407 : ASSERT ( fb == *this, ("Source feature: ", *this, "Deserialized feature: ", fb) );
  fb.SetRank(0);
  (*m_regionGenerator)(fb);
}
}  // namespace generator

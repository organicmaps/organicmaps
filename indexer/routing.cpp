#include "indexer/routing.hpp"

namespace routing
{
// static
uint32_t const Restriction::kInvalidFeatureId = numeric_limits<uint32_t>::max();

bool Restriction::IsValid() const
{
  return !m_featureIds.empty() && find(begin(m_featureIds), end(m_featureIds), kInvalidFeatureId)
      == end(m_featureIds);
}

bool Restriction::operator==(Restriction const & restriction) const
{
  return m_featureIds == restriction.m_featureIds && m_type == restriction.m_type;
}

bool Restriction::operator<(Restriction const & restriction) const
{
  if (m_type != restriction.m_type)
    return m_type < restriction.m_type;
  return m_featureIds < restriction.m_featureIds;
}
}  // namespace routing

namespace feature
{
// For the time being only one kind of restrictions is supported. It's line-point-line
// restrictions in osm ids term. Such restrictions correspond to two feature ids
// restrictions in feature id terms. Because of it supported number of links is two.
size_t const RestrictionSerializer::kSupportedLinkNumber = 2;
}

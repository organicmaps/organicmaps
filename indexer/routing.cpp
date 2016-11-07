#include "indexer/routing.hpp"

namespace routing
{
Restriction::FeatureId const Restriction::kInvalidFeatureId =
    numeric_limits<Restriction::FeatureId>::max();

Restriction::Restriction(Type type, size_t linkNumber) : m_type(type)
{
  m_links.resize(linkNumber, kInvalidFeatureId);
}

Restriction::Restriction(Type type, vector<FeatureId> const & links) : m_links(links), m_type(type)
{
}

bool Restriction::IsValid() const
{
  return !m_links.empty() && find(begin(m_links), end(m_links), kInvalidFeatureId) == end(m_links);
}

bool Restriction::operator==(Restriction const & restriction) const
{
  return m_links == restriction.m_links && m_type == restriction.m_type;
}

bool Restriction::operator<(Restriction const & restriction) const
{
  if (m_type != restriction.m_type)
    return m_type < restriction.m_type;

  return m_links < restriction.m_links;
}
}  // namespace routing

namespace feature
{
// For the time being only one kind of restrictions is supported. It's line-point-line
// restrictions in osm ids term. Such restrictions correspond to two feature ids
// restrictions in feature id terms. Because of it supported number of links is two.
size_t const RestrictionSerializer::kSupportedLinkNumber = 2;
}

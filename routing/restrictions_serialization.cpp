#include "routing/restrictions_serialization.hpp"

#include "std/sstream.hpp"

namespace
{
char const kNo[] = "No";
char const kOnly[] = "Only";
}  // namespace

namespace routing
{
// static
uint32_t const Restriction::kInvalidFeatureId = numeric_limits<uint32_t>::max();

bool Restriction::IsValid() const
{
  return !m_featureIds.empty() &&
         find(begin(m_featureIds), end(m_featureIds), kInvalidFeatureId) == end(m_featureIds);
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

string ToString(Restriction::Type const & type)
{
  switch (type)
  {
  case Restriction::Type::No: return kNo;
  case Restriction::Type::Only: return kOnly;
  }
  return "Unknown";
}

string DebugPrint(Restriction::Type const & type) { return ToString(type); }

string DebugPrint(Restriction const & restriction)
{
  ostringstream out;
  out << "m_featureIds:[" << ::DebugPrint(restriction.m_featureIds)
      << "] m_type:" << DebugPrint(restriction.m_type) << " ";
  return out.str();
}
}  // namespace routing

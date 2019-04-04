#include "routing/restrictions_serialization.hpp"

#include <sstream>

using namespace std;

namespace
{
char const kNo[] = "No";
char const kOnly[] = "Only";
}  // namespace

namespace routing
{
// static
uint32_t const Restriction::kInvalidFeatureId = numeric_limits<uint32_t>::max();

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

string DebugPrint(Restriction::Type const & type)
{
  switch (type)
  {
  case Restriction::Type::No: return kNo;
  case Restriction::Type::Only: return kOnly;
  }
  return "Unknown";
}

string DebugPrint(Restriction const & restriction)
{
  ostringstream out;
  out << "[" << DebugPrint(restriction.m_type) << "]: {";
  for (size_t i = 0; i < restriction.m_featureIds.size(); ++i)
  {
    out << restriction.m_featureIds[i];
    if (i + 1 != restriction.m_featureIds.size())
      out << ", ";
  }
  out << "}";
  return out.str();
}
}  // namespace routing

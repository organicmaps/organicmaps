#include "search/intersection_result.hpp"

#include <sstream>

namespace search
{
// static
uint32_t const IntersectionResult::kInvalidId;

void IntersectionResult::Set(Model::Type type, uint32_t id)
{
  switch (type)
  {
  case Model::TYPE_POI: m_poi = id; break;
  case Model::TYPE_BUILDING: m_building = id; break;
  case Model::TYPE_STREET: m_street = id; break;
  case Model::TYPE_CITY:
  case Model::TYPE_VILLAGE:
  case Model::TYPE_STATE:
  case Model::TYPE_COUNTRY:
  case Model::TYPE_UNCLASSIFIED:
  case Model::TYPE_COUNT: ASSERT(false, ("Unsupported type.")); break;
  }
}

uint32_t IntersectionResult::InnermostResult() const
{
  if (m_poi != kInvalidId)
    return m_poi;
  if (m_building != kInvalidId)
    return m_building;
  if (m_street != kInvalidId)
    return m_street;
  return kInvalidId;
}

void IntersectionResult::Clear()
{
  m_poi = kInvalidId;
  m_building = kInvalidId;
  m_street = kInvalidId;
}

std::string DebugPrint(IntersectionResult const & result)
{
  std::ostringstream os;
  os << "IntersectionResult [ ";
  if (result.m_poi != IntersectionResult::kInvalidId)
    os << "POI:" << result.m_poi << " ";
  if (result.m_building != IntersectionResult::kInvalidId)
    os << "BUILDING:" << result.m_building << " ";
  if (result.m_street != IntersectionResult::kInvalidId)
    os << "STREET:" << result.m_street << " ";
  os << "]";
  return os.str();
}
}  // namespace search

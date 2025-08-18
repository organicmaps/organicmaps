#include "search/intersection_result.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace search
{
// static
uint32_t const IntersectionResult::kInvalidId;

void IntersectionResult::Set(Model::Type type, uint32_t id)
{
  switch (type)
  {
  case Model::TYPE_SUBPOI: m_subpoi = id; break;
  case Model::TYPE_COMPLEX_POI: m_complexPoi = id; break;
  case Model::TYPE_BUILDING: m_building = id; break;
  case Model::TYPE_STREET: m_street = id; break;
  case Model::TYPE_SUBURB: m_suburb = id; break;

  /// @todo Store city (place) name for ranking? I suspect that it should work fine now, without it.
  case Model::TYPE_CITY: break;

  case Model::TYPE_VILLAGE:
  case Model::TYPE_STATE:
  case Model::TYPE_COUNTRY:
  case Model::TYPE_UNCLASSIFIED:
  case Model::TYPE_COUNT: ASSERT(false, ("Unsupported type.")); break;
  }
}

uint32_t IntersectionResult::InnermostResult() const
{
  if (m_subpoi != kInvalidId)
    return m_subpoi;
  if (m_complexPoi != kInvalidId)
    return m_complexPoi;
  if (m_building != kInvalidId)
    return m_building;
  if (m_street != kInvalidId)
    return m_street;
  if (m_suburb != kInvalidId)
    return m_suburb;
  return kInvalidId;
}

void IntersectionResult::Clear()
{
  m_subpoi = kInvalidId;
  m_complexPoi = kInvalidId;
  m_building = kInvalidId;
  m_street = kInvalidId;
  m_suburb = kInvalidId;
}

std::string DebugPrint(IntersectionResult const & result)
{
  std::ostringstream os;
  os << "IntersectionResult [ ";
  if (result.m_subpoi != IntersectionResult::kInvalidId)
    os << "SUBPOI:" << result.m_subpoi << " ";
  if (result.m_complexPoi != IntersectionResult::kInvalidId)
    os << "COMPLEX_POI:" << result.m_complexPoi << " ";
  if (result.m_building != IntersectionResult::kInvalidId)
    os << "BUILDING:" << result.m_building << " ";
  if (result.m_street != IntersectionResult::kInvalidId)
    os << "STREET:" << result.m_street << " ";
  if (result.m_suburb != IntersectionResult::kInvalidId)
    os << "SUBURB:" << result.m_suburb << " ";
  os << "]";
  return os.str();
}
}  // namespace search

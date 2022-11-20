#include "indexer/feature_decl.hpp"

#include "std/boost_container_hash.hpp"

#include <sstream>

namespace feature
{
std::string DebugPrint(GeomType type)
{
  switch (type)
  {
  case GeomType::Undefined: return "Undefined";
  case GeomType::Point: return "Point";
  case GeomType::Line: return "Line";
  case GeomType::Area: return "Area";
  }
  UNREACHABLE();
}
}  // namespace feature

std::string DebugPrint(FeatureID const & id)
{
  return "{ " + DebugPrint(id.m_mwmId) + ", " + std::to_string(id.m_index) + " }";
}

// static
char const * const FeatureID::kInvalidFileName = "INVALID";
// static
int64_t const FeatureID::kInvalidMwmVersion = -1;


std::string FeatureID::GetMwmName() const
{
  return IsValid() ? m_mwmId.GetInfo()->GetCountryName() : kInvalidFileName;
}

int64_t FeatureID::GetMwmVersion() const
{
  return IsValid() ? m_mwmId.GetInfo()->GetVersion() : kInvalidMwmVersion;
}

size_t std::hash<FeatureID>::operator()(FeatureID const & fID) const
{
  size_t seed = 0;
  boost::hash_combine(seed, fID.m_mwmId.GetInfo());
  boost::hash_combine(seed, fID.m_index);
  return seed;
}

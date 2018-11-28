#include "indexer/feature_decl.hpp"

#include <sstream>

using namespace std;

string DebugPrint(FeatureID const & id)
{
  ostringstream ss;
  ss << "{ " << DebugPrint(id.m_mwmId) << ", " << id.m_index << " }";
  return ss.str();
}

string DebugPrint(feature::EGeomType type)
{
  using feature::EGeomType;
  switch (type)
  {
  case EGeomType::GEOM_UNDEFINED: return "GEOM_UNDEFINED";
  case EGeomType::GEOM_POINT: return "GEOM_POINT";
  case EGeomType::GEOM_LINE: return "GEOM_LINE";
  case EGeomType::GEOM_AREA: return "GEOM_AREA";
  }
  UNREACHABLE();
}

// static
char const * const FeatureID::kInvalidFileName = "INVALID";
// static
int64_t const FeatureID::kInvalidMwmVersion = -1;


string FeatureID::GetMwmName() const
{
  return IsValid() ? m_mwmId.GetInfo()->GetCountryName() : kInvalidFileName;
}

int64_t FeatureID::GetMwmVersion() const
{
  return IsValid() ? m_mwmId.GetInfo()->GetVersion() : kInvalidMwmVersion;
}

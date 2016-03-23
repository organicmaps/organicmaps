#include "indexer/feature_decl.hpp"

#include "std/sstream.hpp"

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
}

pair<FeatureID::MwmName, FeatureID::MwmVersion> FeatureID::GetMwmNameAndVersion() const
{
  if (!IsValid())
    return {"INVALID", 0};

  auto const & mwmInfo = m_mwmId.GetInfo();
  return {mwmInfo->GetCountryName(), mwmInfo->GetVersion()};
}


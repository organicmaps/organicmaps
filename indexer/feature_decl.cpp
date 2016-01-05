#include "indexer/feature_decl.hpp"

#include "std/sstream.hpp"

namespace feature
{
string DebugPrint(feature::EGeomType type)
{
  switch (type)
  {
    case feature::GEOM_UNDEFINED: return "GEOM_UNDEFINED";
    case feature::GEOM_POINT: return "GEOM_POINT";
    case feature::GEOM_LINE: return "GEOM_LINE";
    case feature::GEOM_AREA: return "GEOM_AREA";
  }
}
}  // namespace feature

string DebugPrint(FeatureID const & id)
{
  ostringstream ss;
  ss << "{ " << DebugPrint(id.m_mwmId) << ", " << id.m_index << " }";
  return ss.str();
}

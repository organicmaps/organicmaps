#include "indexer/feature_decl.hpp"

#include <sstream>

using namespace std;

namespace feature
{
string DebugPrint(GeomType type)
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

string DebugPrint(FeatureID const & id)
{
  ostringstream ss;
  ss << "{ " << DebugPrint(id.m_mwmId) << ", " << id.m_index << " }";
  return ss.str();
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

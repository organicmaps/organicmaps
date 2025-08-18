#include "search/geocoder_locality.hpp"

#include <sstream>

namespace search
{
// static
Model::Type Region::ToModelType(Type type)
{
  switch (type)
  {
  case Region::TYPE_STATE: return Model::TYPE_STATE;
  case Region::TYPE_COUNTRY: return Model::TYPE_COUNTRY;
  case Region::TYPE_COUNT: return Model::TYPE_COUNT;
  }
  UNREACHABLE();
}

std::string DebugPrint(Locality const & locality)
{
  std::ostringstream os;
  os << "Locality [ ";
  os << "m_featureId=" << DebugPrint(locality.m_featureId) << ", ";
  os << "m_tokenRange=" << DebugPrint(locality.m_tokenRange) << ", ";
  os << " ]";
  return os.str();
}
}  // namespace search

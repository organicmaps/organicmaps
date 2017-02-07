#include "search/geocoder_locality.hpp"

#include <sstream>

namespace search
{
// static
SearchModel::SearchType Region::ToSearchType(Type type)
{
  switch (type)
  {
  case Region::TYPE_STATE: return SearchModel::SEARCH_TYPE_STATE;
  case Region::TYPE_COUNTRY: return SearchModel::SEARCH_TYPE_COUNTRY;
  case Region::TYPE_COUNT: return SearchModel::SEARCH_TYPE_COUNT;
  }
}

std::string DebugPrint(Locality const & locality)
{
  std::ostringstream os;
  os << "Locality [ ";
  os << "m_countryId=" << DebugPrint(locality.m_countryId) << ", ";
  os << "m_featureId=" << locality.m_featureId << ", ";
  os << "m_tokenRange=" << DebugPrint(locality.m_tokenRange) << ", ";
  os << "m_prob=" << locality.m_prob;
  os << " ]";
  return os.str();
}
}  // namespace search

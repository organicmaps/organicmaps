#include "search/v2/search_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

using namespace ftypes;

namespace search
{
namespace v2
{
// static
SearchModel const & SearchModel::Instance()
{
  static SearchModel model;
  return model;
}

SearchModel::SearchType SearchModel::GetSearchType(FeatureType const & feature) const
{
  static auto const & buildingChecker = IsBuildingChecker::Instance();
  static auto const & streetChecker = IsStreetChecker::Instance();
  static auto const & localityChecker = IsLocalityChecker::Instance();

  if (buildingChecker(feature))
    return SEARCH_TYPE_BUILDING;

  if (streetChecker(feature))
    return SEARCH_TYPE_STREET;

  if (localityChecker(feature))
  {
    Type type = localityChecker.GetType(feature);
    switch (type)
    {
    case COUNTRY:
    case STATE:
    case NONE:
      return SEARCH_TYPE_COUNT;
    case CITY:
    case TOWN:
    case VILLAGE:
      return SEARCH_TYPE_CITY;
    case LOCALITY_COUNT:
      return SEARCH_TYPE_COUNT;
    }
  }

  return SEARCH_TYPE_POI;
}

string DebugPrint(SearchModel::SearchType type)
{
  switch (type)
  {
  case SearchModel::SEARCH_TYPE_POI:
    return "POI";
  case SearchModel::SEARCH_TYPE_BUILDING:
    return "BUILDING";
  case SearchModel::SEARCH_TYPE_STREET:
    return "STREET";
  case SearchModel::SEARCH_TYPE_CITY:
    return "CITY";
  case SearchModel::SEARCH_TYPE_COUNT:
    return "COUNT";
  }
  ASSERT(false, ("Unknown search type:", static_cast<int>(type)));
  return string();
}
}  // namespace v2
}  // namespace search

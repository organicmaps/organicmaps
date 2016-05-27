#include "search/search_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/macros.hpp"

using namespace ftypes;

namespace search
{
TwoLevelPOIChecker::TwoLevelPOIChecker() : ftypes::BaseChecker(2 /* level */)
{
  Classificator const & c = classif();
  StringIL arr[] = {{"highway", "bus_stop"},
                    {"highway", "speed_camera"},
                    {"waterway", "waterfall"},
                    {"natural", "volcano"},
                    {"natural", "cave_entrance"},
                    {"natural", "beach"}};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(arr[i]));
}

namespace
{
/// Should be similar with ftypes::IsAddressObjectChecker object classes.
class OneLevelPOIChecker : public ftypes::BaseChecker
{
public:
  OneLevelPOIChecker() : ftypes::BaseChecker(1 /* level */)
  {
    Classificator const & c = classif();

    auto paths = {"amenity", "historic", "office", "railway", "shop", "sport", "tourism", "craft"};
    for (auto const & path : paths)
      m_types.push_back(c.GetTypeByPath({path}));
  }
};

class IsPoiChecker
{
public:
  IsPoiChecker() {}

  static IsPoiChecker const & Instance()
  {
    static const IsPoiChecker inst;
    return inst;
  }

  bool operator()(FeatureType const & ft) const { return m_oneLevel(ft) || m_twoLevel(ft); }

private:
  OneLevelPOIChecker const m_oneLevel;
  TwoLevelPOIChecker const m_twoLevel;
};

class CustomIsBuildingChecker
{
public:
  static CustomIsBuildingChecker const & Instance()
  {
    static const CustomIsBuildingChecker inst;
    return inst;
  }

  bool operator()(FeatureType const & ft) const
  {
    return !ft.GetHouseNumber().empty() || IsBuildingChecker::Instance()(ft);
  }

private:
  CustomIsBuildingChecker() {}
};
}  // namespace

// static
SearchModel const & SearchModel::Instance()
{
  static SearchModel model;
  return model;
}

SearchModel::SearchType SearchModel::GetSearchType(FeatureType const & feature) const
{
  static auto const & buildingChecker = CustomIsBuildingChecker::Instance();
  static auto const & streetChecker = IsStreetChecker::Instance();
  static auto const & localityChecker = IsLocalityChecker::Instance();
  static auto const & poiChecker = IsPoiChecker::Instance();

  if (buildingChecker(feature))
    return SEARCH_TYPE_BUILDING;

  if (streetChecker(feature))
    return SEARCH_TYPE_STREET;

  if (localityChecker(feature))
  {
    Type type = localityChecker.GetType(feature);
    switch (type)
    {
    case NONE: ASSERT(false, ("Unknown locality.")); return SEARCH_TYPE_UNCLASSIFIED;
    case STATE: return SEARCH_TYPE_STATE;
    case COUNTRY: return SEARCH_TYPE_COUNTRY;
    case CITY:
    case TOWN: return SEARCH_TYPE_CITY;
    case VILLAGE: return SEARCH_TYPE_VILLAGE;
    case LOCALITY_COUNT: return SEARCH_TYPE_UNCLASSIFIED;
    }
  }

  if (poiChecker(feature))
    return SEARCH_TYPE_POI;

  return SEARCH_TYPE_UNCLASSIFIED;
}

string DebugPrint(SearchModel::SearchType type)
{
  switch (type)
  {
  case SearchModel::SEARCH_TYPE_POI: return "POI";
  case SearchModel::SEARCH_TYPE_BUILDING: return "Building";
  case SearchModel::SEARCH_TYPE_STREET: return "Street";
  case SearchModel::SEARCH_TYPE_CITY: return "City";
  case SearchModel::SEARCH_TYPE_VILLAGE: return "Village";
  case SearchModel::SEARCH_TYPE_STATE: return "State";
  case SearchModel::SEARCH_TYPE_COUNTRY: return "Country";
  case SearchModel::SEARCH_TYPE_UNCLASSIFIED: return "Unclassified";
  case SearchModel::SEARCH_TYPE_COUNT: return "Count";
  }
  ASSERT(false, ("Unknown search type:", static_cast<int>(type)));
  return string();
}

}  // namespace search

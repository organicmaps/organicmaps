#include "search/model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <vector>

using namespace ftypes;
using namespace std;

namespace search
{
TwoLevelPOIChecker::TwoLevelPOIChecker() : ftypes::BaseChecker(2 /* level */)
{
  Classificator const & c = classif();
  base::StringIL arr[] = {{"aeroway", "terminal"},
                          {"aeroway", "gate"},
                          {"building", "train_station"},
                          {"emergency", "defibrillator"},
                          {"emergency", "fire_hydrant"},
                          {"highway", "bus_stop"},
                          {"highway", "ford"},
                          {"highway", "raceway"},
                          {"highway", "rest_area"},
                          {"highway", "speed_camera"},
                          {"natural", "beach"},
                          {"natural", "geyser"},
                          {"natural", "cave_entrance"},
                          {"natural", "spring"},
                          {"natural", "volcano"},
                          {"office", "estate_agent"},
                          {"office", "government"},
                          {"office", "insurance"},
                          {"office", "lawyer"},
                          {"office", "telecommunication"},
                          {"waterway", "waterfall"}};

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

  bool operator()(FeatureType & ft) const { return m_oneLevel(ft) || m_twoLevel(ft); }

private:
  OneLevelPOIChecker const m_oneLevel;
  TwoLevelPOIChecker const m_twoLevel;
};

class IsComplexPoiChecker : public ftypes::BaseChecker
{
  IsComplexPoiChecker() : ftypes::BaseChecker()
  {
    Classificator const & c = classif();
    vector<vector<string>> const paths = {{"aeroway", "aerodrome"},
                                          {"amenity", "hospital"},
                                          {"amenity", "university"},
                                          {"building", "train_station"},
                                          {"historic", "archaeological_site"},
                                          {"historic", "castle"},
                                          {"historic", "fort"},
                                          {"historic", "museum"},
                                          {"landuse", "cemetery"},
                                          {"landuse", "churchyard"},
                                          {"landuse", "commercial"},
                                          {"landuse", "forest"},
                                          {"landuse", "industrial"},
                                          {"landuse", "retail"},
                                          {"leisure", "garden"},
                                          {"leisure", "nature_reserve"},
                                          {"leisure", "park"},
                                          {"leisure", "stadium"},
                                          {"leisure", "water_park"},
                                          {"natural", "beach"},
                                          {"office", "company"},
                                          {"railway", "station"},
                                          {"shop", "mall"},
                                          {"tourism", "museum"},
                                          {"tourism", "gallery"}};

    for (auto const & path : paths)
      m_types.push_back(c.GetTypeByPath(path));
  }

public:
  static IsComplexPoiChecker const & Instance()
  {
    static IsComplexPoiChecker const inst;
    return inst;
  }
};

class CustomIsBuildingChecker
{
public:
  static CustomIsBuildingChecker const & Instance()
  {
    static const CustomIsBuildingChecker inst;
    return inst;
  }

  bool operator()(FeatureType & ft) const
  {
    return !ft.GetHouseNumber().empty() || IsBuildingChecker::Instance()(ft);
  }

private:
  CustomIsBuildingChecker() {}
};
}  // namespace

Model::Type Model::GetType(FeatureType & feature) const
{
  static auto const & buildingChecker = CustomIsBuildingChecker::Instance();
  static auto const & streetChecker = IsStreetOrSquareChecker::Instance();
  static auto const & suburbChecker = IsSuburbChecker::Instance();
  static auto const & localityChecker = IsLocalityChecker::Instance();
  static auto const & poiChecker = IsPoiChecker::Instance();
  static auto const & complexPoiChecker = IsComplexPoiChecker::Instance();

  // Check whether object is POI first to mark POIs with address tags as POI.
  if (complexPoiChecker(feature))
    return TYPE_COMPLEX_POI;
  if (poiChecker(feature))
    return TYPE_SUBPOI;

  if (buildingChecker(feature))
    return TYPE_BUILDING;

  if (streetChecker(feature))
    return TYPE_STREET;

  if (suburbChecker(feature))
    return TYPE_SUBURB;

  if (localityChecker(feature))
  {
    auto const type = localityChecker.GetType(feature);
    switch (type)
    {
    case LocalityType::None: ASSERT(false, ("Unknown locality.")); return TYPE_UNCLASSIFIED;
    case LocalityType::State: return TYPE_STATE;
    case LocalityType::Country: return TYPE_COUNTRY;
    case LocalityType::City:
    case LocalityType::Town: return TYPE_CITY;
    case LocalityType::Village: return TYPE_VILLAGE;
    case LocalityType::Count: return TYPE_UNCLASSIFIED;
    }
  }

  return TYPE_UNCLASSIFIED;
}

string DebugPrint(Model::Type type)
{
  switch (type)
  {
  case Model::TYPE_SUBPOI: return "SUBPOI";
  case Model::TYPE_COMPLEX_POI: return "COMPLEX_POI";
  case Model::TYPE_BUILDING: return "Building";
  case Model::TYPE_STREET: return "Street";
  case Model::TYPE_SUBURB: return "Suburb";
  case Model::TYPE_CITY: return "City";
  case Model::TYPE_VILLAGE: return "Village";
  case Model::TYPE_STATE: return "State";
  case Model::TYPE_COUNTRY: return "Country";
  case Model::TYPE_UNCLASSIFIED: return "Unclassified";
  case Model::TYPE_COUNT: return "Count";
  }
  ASSERT(false, ("Unknown search type:", static_cast<int>(type)));
  return {};
}
}  // namespace search

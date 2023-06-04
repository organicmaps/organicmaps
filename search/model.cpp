#include "search/model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/stl_helpers.hpp"

#include <vector>

namespace search
{
using namespace ftypes;
using namespace std;

TwoLevelPOIChecker::TwoLevelPOIChecker() : ftypes::BaseChecker(2 /* level */)
{
  Classificator const & c = classif();
  base::StringIL arr[] = {{"aeroway", "terminal"},
                          {"aeroway", "gate"},
                          {"building", "train_station"},
                          {"emergency", "defibrillator"},
                          {"emergency", "fire_hydrant"},
                          {"emergency", "phone"},
                          {"healthcare", "laboratory"},
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
                          {"waterway", "waterfall"}};

  for (auto const & path : arr)
    m_types.push_back(c.GetTypeByPath(path));
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
  DECLARE_CHECKER_INSTANCE(IsPoiChecker);

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
    base::StringIL const paths[] = {{"aeroway", "aerodrome"},
                                    {"amenity", "hospital"},
                                    {"amenity", "university"},
                                    {"building", "train_station"},
                                    {"historic", "archaeological_site"},
                                    {"historic", "castle"},
                                    {"historic", "fort"},
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
  DECLARE_CHECKER_INSTANCE(IsComplexPoiChecker);
};

class CustomIsBuildingChecker
{
public:
  DECLARE_CHECKER_INSTANCE(CustomIsBuildingChecker);

  bool operator()(FeatureType & ft) const
  {
    return !ft.GetHouseNumber().empty() || IsBuildingChecker::Instance()(ft);
  }
};
}  // namespace

Model::Type Model::GetType(FeatureType & feature) const
{
  // Check whether object is POI first to mark POIs with address tags as POI.
  if (IsComplexPoiChecker::Instance()(feature))
    return TYPE_COMPLEX_POI;
  if (IsPoiChecker::Instance()(feature))
    return TYPE_SUBPOI;

  if (CustomIsBuildingChecker::Instance()(feature))
    return TYPE_BUILDING;

  if (IsStreetOrSquareChecker::Instance()(feature))
    return TYPE_STREET;

  if (IsSuburbChecker::Instance()(feature))
    return TYPE_SUBURB;

  auto const type = IsLocalityChecker::Instance().GetType(feature);
  switch (type)
  {
  case LocalityType::State: return TYPE_STATE;
  case LocalityType::Country: return TYPE_COUNTRY;
  case LocalityType::City:
  case LocalityType::Town: return TYPE_CITY;
  case LocalityType::Village: return TYPE_VILLAGE;
  case LocalityType::Count: ASSERT(false, ()); [[fallthrough]];
  case LocalityType::None: return TYPE_UNCLASSIFIED;
  }

  ASSERT(false, ("Unknown locality type:", static_cast<int>(type)));
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

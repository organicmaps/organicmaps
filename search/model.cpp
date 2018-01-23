#include "search/model.hpp"

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
                    {"natural", "beach"},
                    {"emergency", "defibrillator"},
                    {"emergency", "fire_hydrant"}};

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

Model::Type Model::GetType(FeatureType const & feature) const
{
  static auto const & buildingChecker = CustomIsBuildingChecker::Instance();
  static auto const & streetChecker = IsStreetChecker::Instance();
  static auto const & localityChecker = IsLocalityChecker::Instance();
  static auto const & poiChecker = IsPoiChecker::Instance();

  if (buildingChecker(feature))
    return TYPE_BUILDING;

  if (streetChecker(feature))
    return TYPE_STREET;

  if (localityChecker(feature))
  {
    auto const type = localityChecker.GetType(feature);
    switch (type)
    {
    case NONE: ASSERT(false, ("Unknown locality.")); return TYPE_UNCLASSIFIED;
    case STATE: return TYPE_STATE;
    case COUNTRY: return TYPE_COUNTRY;
    case CITY:
    case TOWN: return TYPE_CITY;
    case VILLAGE: return TYPE_VILLAGE;
    case LOCALITY_COUNT: return TYPE_UNCLASSIFIED;
    }
  }

  if (poiChecker(feature))
    return TYPE_POI;

  return TYPE_UNCLASSIFIED;
}

string DebugPrint(Model::Type type)
{
  switch (type)
  {
  case Model::TYPE_POI: return "POI";
  case Model::TYPE_BUILDING: return "Building";
  case Model::TYPE_STREET: return "Street";
  case Model::TYPE_CITY: return "City";
  case Model::TYPE_VILLAGE: return "Village";
  case Model::TYPE_STATE: return "State";
  case Model::TYPE_COUNTRY: return "Country";
  case Model::TYPE_UNCLASSIFIED: return "Unclassified";
  case Model::TYPE_COUNT: return "Count";
  }
  ASSERT(false, ("Unknown search type:", static_cast<int>(type)));
  return string();
}
}  // namespace search

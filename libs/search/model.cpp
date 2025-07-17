#include "search/model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/stl_helpers.hpp"

#include <vector>


namespace search
{
using namespace ftypes;
using namespace std;

namespace
{

class IsComplexPoiChecker : public ftypes::BaseChecker
{
  IsComplexPoiChecker() : ftypes::BaseChecker()
  {
    // For MatchPOIsWithParent matching. Some entries may be controversial here, but keep as-is for now.
    // POI near "Complex POI" matching.
    base::StringIL const paths[] = {
        {"aeroway", "aerodrome"},
        {"amenity", "hospital"},
        {"amenity", "university"},
        {"building", "train_station"},
        {"historic", "archaeological_site"},
        {"historic", "castle"},
        {"historic", "fort"},
        {"landuse", "cemetery"},
        {"landuse", "religious"},
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
        {"tourism", "gallery"}
    };

    Classificator const & c = classif();
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
    if (!ft.GetHouseNumber().empty())
      return true;

    if (ft.GetGeomType() == feature::GeomType::Line)
      return IsAddressInterpolChecker::Instance()(ft);
    else
      return IsBuildingChecker::Instance()(ft);
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

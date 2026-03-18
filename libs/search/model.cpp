#include "search/model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

namespace search
{

Model::IsComplexPoiChecker::IsComplexPoiChecker()
{
  // For MatchPOIsWithParent matching. Some entries may be controversial here, but keep as-is for now.
  // POI near "Complex POI" matching.
  base::StringIL const paths[] = {{"aeroway", "aerodrome"},
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
                                  {"tourism", "gallery"}};

  Classificator const & c = classif();
  for (auto const & path : paths)
    m_types.push_back(c.GetTypeByPath(path));
}

bool Model::CustomIsBuildingChecker::operator()(FeatureType & ft) const
{
  if (!ft.GetHouseNumber().empty())
    return true;

  if (ft.GetGeomType() == feature::GeomType::Line)
    return m_interpol(ft);
  else
    return m_building(ft);
}

Model::Type Model::GetType(FeatureType & feature) const
{
  // Check whether object is POI first to mark POIs with address tags as POI.
  if (m_isComplexPoi(feature))
    return TYPE_COMPLEX_POI;
  if (m_isPoi(feature))
    return TYPE_SUBPOI;

  if (m_isCustomBuilding(feature))
    return TYPE_BUILDING;

  if (m_isStreetOrSquare(feature))
    return TYPE_STREET;

  if (m_isSuburb(feature))
    return TYPE_SUBURB;

  auto const type = m_isLocality.GetType(feature);
  switch (type)
  {
    using ftypes::LocalityType;
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

std::string DebugPrint(Model::Type type)
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

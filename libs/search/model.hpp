#pragma once

#include "indexer/ftypes_matcher.hpp"

#include <string>

class FeatureType;

namespace search
{

// This class is used to map feature types to a restricted set of
// different search classes (do not confuse these classes with search
// categories - they are completely different things).
class Model
{
public:
  /// @note Check ranking_info.cpp constants (kType) before changing this enum.
  enum Type : uint8_t
  {
    // Low-level features such as amenities, offices, shops, buildings without house number, etc.
    // Can be stand-alone or located inside COMPLEX_POIs. E.g. cafes/shops inside
    // airports/universities/museums.
    TYPE_SUBPOI = 0,

    // Big pois which can contain SUBPOIs. E.g. airports, train stations, malls, parks.
    TYPE_COMPLEX_POI,

    // All features with set house number.
    TYPE_BUILDING,

    TYPE_STREET,
    TYPE_SUBURB,

    // All low-level features except POI, BUILDING and STREET.
    TYPE_UNCLASSIFIED,

    TYPE_VILLAGE,
    TYPE_CITY,
    TYPE_STATE,  // US or Canadian states
    TYPE_COUNTRY,

    TYPE_COUNT
  };

  static bool IsLocalityType(Type const type) { return type >= TYPE_VILLAGE && type <= TYPE_COUNTRY; }

  static bool IsPoi(Type const type) { return type == TYPE_SUBPOI || type == TYPE_COMPLEX_POI; }
  static bool IsPoiOrBuilding(Type const type) { return IsPoi(type) || type == TYPE_BUILDING; }

  Type GetType(FeatureType & feature) const;

private:
  struct IsComplexPoiChecker : public ftypes::BaseChecker
  {
    DECLARE_CHECKER_INSTANCE(IsComplexPoiChecker);
    IsComplexPoiChecker();
  };
  IsComplexPoiChecker const & m_isComplexPoi = IsComplexPoiChecker::Instance();

  ftypes::IsPoiChecker const & m_isPoi = ftypes::IsPoiChecker::Instance();

  class CustomIsBuildingChecker
  {
    ftypes::IsAddressInterpolChecker m_interpol;
    ftypes::IsBuildingChecker m_building;

  protected:
    void PostInitialize()
    {
      m_interpol.PostInitialize();
      m_building.PostInitialize();
    }

  public:
    DECLARE_CHECKER_INSTANCE(CustomIsBuildingChecker);

    bool operator()(FeatureType & ft) const;
  };
  CustomIsBuildingChecker const & m_isCustomBuilding = CustomIsBuildingChecker::Instance();

  ftypes::IsStreetOrSquareChecker const & m_isStreetOrSquare = ftypes::IsStreetOrSquareChecker::Instance();
  ftypes::IsSuburbChecker const & m_isSuburb = ftypes::IsSuburbChecker::Instance();
  ftypes::IsLocalityChecker const & m_isLocality = ftypes::IsLocalityChecker::Instance();
};

std::string DebugPrint(Model::Type type);
}  // namespace search

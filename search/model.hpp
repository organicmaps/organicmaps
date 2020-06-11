#pragma once

#include "indexer/ftypes_matcher.hpp"

#include "base/macros.hpp"

#include <string>

class FeatureType;

namespace search
{
/// Describes 2-level POI-exception types that don't belong to any POI-common classes
/// (amenity, shop, tourism, ...). Used in search algo and search categories index generation.
class TwoLevelPOIChecker : public ftypes::BaseChecker
{
public:
  TwoLevelPOIChecker();
};

// This class is used to map feature types to a restricted set of
// different search classes (do not confuse these classes with search
// categories - they are completely different things).
class Model
{
public:
  // WARNING: after modifications to the enum below, re-check all methods in the class.
  enum Type
  {
    // Low-level features such as amenities, offices, shops, buildings without house number, etc.
    // Can be stand-alone or located inside COMPLEX_POIs. E.g. cafes/shops inside
    // airports/universities/museums.
    TYPE_SUBPOI,

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

  static bool IsLocalityType(Type const type)
  {
    return type >= TYPE_VILLAGE && type <= TYPE_COUNTRY;
  }

  static bool IsPoi(Type const type) { return type == TYPE_SUBPOI || type == TYPE_COMPLEX_POI; }

  Type GetType(FeatureType & feature) const;
};

std::string DebugPrint(Model::Type type);
}  // namespace search

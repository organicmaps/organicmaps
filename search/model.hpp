#pragma once

#include "indexer/ftypes_matcher.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

#include "base/macros.hpp"

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
    // Low-level features such as amenities, offices, shops, buildings
    // without house number, etc.
    TYPE_POI,

    // All features with set house number.
    TYPE_BUILDING,

    TYPE_STREET,

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

  Type GetType(FeatureType const & feature) const;

  void SetCianEnabled(bool enabled) { m_cianEnabled = enabled; }

private:
  bool m_cianEnabled = false;
};

string DebugPrint(Model::Type type);
}  // namespace search

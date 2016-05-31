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
class SearchModel
{
public:
  enum SearchType
  {
    // Low-level features such as amenities, offices, shops, buildings
    // without house number, etc.
    SEARCH_TYPE_POI,

    // All features with set house number.
    SEARCH_TYPE_BUILDING,

    SEARCH_TYPE_STREET,

    // All low-level features except POI, BUILDING and STREET.
    SEARCH_TYPE_UNCLASSIFIED,

    SEARCH_TYPE_VILLAGE,
    SEARCH_TYPE_CITY,
    SEARCH_TYPE_STATE,  // US or Canadian states
    SEARCH_TYPE_COUNTRY,

    SEARCH_TYPE_COUNT
  };

  static SearchModel const & Instance();

  SearchType GetSearchType(FeatureType const & feature) const;

private:
  SearchModel() = default;

  DISALLOW_COPY_AND_MOVE(SearchModel);
};

string DebugPrint(SearchModel::SearchType type);

}  // namespace search

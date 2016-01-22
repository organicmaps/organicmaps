#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

#include "base/macros.hpp"

class FeatureType;

namespace ftypes
{
class BaseChecker;
}

namespace search
{
namespace v2
{
// This class is used to map feature types to a restricted set of
// different search classes (do not confuse these classes with search
// categories - they are completely different things).
class SearchModel
{
public:
  enum SearchType
  {
    SEARCH_TYPE_POI,
    SEARCH_TYPE_BUILDING,
    SEARCH_TYPE_STREET,
    SEARCH_TYPE_CITY,
    SEARCH_TYPE_STATE, // US or Canadian states
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
}  // namespace v2
}  // namespace search

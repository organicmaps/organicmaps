#include "geocoder/geocoder.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/osm_id.hpp"

#include <algorithm>

using namespace std;

namespace geocoder
{
Geocoder::Geocoder(string pathToJsonHierarchy) : m_hierarchy(pathToJsonHierarchy) {}

void Geocoder::ProcessQuery(string const & query, vector<Result> & results) const
{
  // Only here for demonstration purposes and will be removed shortly.
  results.clear();
  if (query == "a")
  {
    results.emplace_back(osm::Id(0xC00000000026FCFDULL), 0.5 /* certainty */);
    results.emplace_back(osm::Id(0x40000000C4D63818ULL), 1.0 /* certainty */);
  }
  if (query == "b")
  {
    results.emplace_back(osm::Id(0x8000000014527125ULL), 0.8 /* certainty */);
    results.emplace_back(osm::Id(0x40000000F26943B9ULL), 0.1 /* certainty */);
  }
}

Hierarchy const & Geocoder::GetHierarchy() const { return m_hierarchy; }
}  // namespace geocoder

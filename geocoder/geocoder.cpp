#include "geocoder/geocoder.hpp"

#include "base/osm_id.hpp"

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
    results.emplace_back(osm::Id(10), 0.5 /* certainty */);
    results.emplace_back(osm::Id(11), 1.0 /* certainty */);
  }
  if (query == "b")
  {
    results.emplace_back(osm::Id(20), 0.8 /* certainty */);
    results.emplace_back(osm::Id(21), 0.1 /* certainty */);
  }
}
}  // namespace geocoder

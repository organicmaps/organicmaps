#pragma once

#include "geocoder/hierarchy.hpp"
#include "geocoder/result.hpp"

#include <string>
#include <vector>

namespace geocoder
{
// This class performs geocoding by using the data that we are currently unable
// to distribute to mobile devices. Therefore, the class is intended to be used
// on the server side.
// On the other hand, the design is largely experimental and when the dust
// settles we may reuse some parts of it in the offline mobile application.
// In this case, a partial merge with search/ and in particular with
// search/geocoder.hpp is possible.
//
// Geocoder receives a search query and returns the osm ids of the features
// that match it. Currently, the only data source for the geocoder is
// the hierarchy of features, that is, for every feature that can be found
// the geocoder expects to have the total information about this feature
// in the region subdivision graph (e.g., country, city, street that contain a
// certain house). This hierarchy is to be obtained elsewhere.
//
// Note that search index, locality index, scale index, and, generally, mwm
// features are currently not used at all.
class Geocoder
{
public:
  explicit Geocoder(std::string pathToJsonHierarchy);

  void ProcessQuery(std::string const & query, std::vector<Result> & results) const;

private:
  Hierarchy m_hierarchy;
};
}  // namespace geocoder

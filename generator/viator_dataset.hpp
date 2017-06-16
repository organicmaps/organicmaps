#pragma once

#include "generator/sponsored_object_storage.hpp"

#include "search/city_finder.hpp"

#include "indexer/index.hpp"

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include <limits>
#include <memory>
#include <string>

class FeatureBuilder1;

namespace generator
{
struct ViatorCity
{
  explicit ViatorCity(std::string const & src);

  NEWTYPE(uint32_t, ObjectId);

  static constexpr ObjectId InvalidObjectId()
  {
    return ObjectId(std::numeric_limits<typename ObjectId::RepType>::max());
  }

  ObjectId m_id{InvalidObjectId()};
  ms::LatLon m_latLon = ms::LatLon::Zero();
  std::string m_name;
};

ostream & operator<<(ostream & s, ViatorCity const & r);

NEWTYPE_SIMPLE_OUTPUT(ViatorCity::ObjectId);

class ViatorDataset
{
public:
  ViatorDataset(std::string const & dataPath);

  ViatorCity::ObjectId FindMatchingObjectId(FeatureBuilder1 const & fb) const;

  void PreprocessMatchedOsmObject(ViatorCity::ObjectId const matchedObjId, FeatureBuilder1 & fb,
                                  function<void(FeatureBuilder1 &)> const fn) const;

private:
  SponsoredObjectStorage<ViatorCity> m_storage;
  Index m_index;
  std::unique_ptr<search::CityFinder> m_cityFinder;
};
}  // namespace generator

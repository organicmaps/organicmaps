#include "generator/sponsored_dataset.hpp"

#include "generator/utils.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/data_source.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <functional>
#include <memory>
#include <string>

namespace generator
{
class AddressMatcher
{
public:
  AddressMatcher()
  {
    LoadDataSource(m_dataSource);
    m_coder = std::make_unique<search::ReverseGeocoder>(m_dataSource);
  }

  template <typename SponsoredObject>
  void operator()(SponsoredObject & object)
  {
    search::ReverseGeocoder::Address addr;
    m_coder->GetNearbyAddress(mercator::FromLatLon(object.m_latLon), addr);
    object.m_street = addr.GetStreetName();
    object.m_houseNumber = addr.GetHouseNumber();
  }

private:
  FrozenDataSource m_dataSource;
  std::unique_ptr<search::ReverseGeocoder> m_coder;
};

// SponsoredDataset --------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(std::string const & dataPath)
  : m_storage(kDistanceLimitInMeters, kMaxSelectedElements)
{
  m_storage.LoadData(dataPath);
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::BuildOsmObjects(std::function<void(feature::FeatureBuilder &)> const & fn) const
{
  for (auto const & item : m_storage.GetObjects())
    BuildObject(item.second, fn);
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::ObjectId
SponsoredDataset<SponsoredObject>::FindMatchingObjectId(feature::FeatureBuilder const & fb) const
{
  if (NecessaryMatchingConditionHolds(fb))
    return FindMatchingObjectIdImpl(fb);
  return Object::InvalidObjectId();
}
}  // namespace generator

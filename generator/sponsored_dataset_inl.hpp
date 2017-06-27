#include "generator/sponsored_dataset.hpp"

#include "generator/utils.hpp"

#include "search/reverse_geocoder.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

namespace generator
{
class AddressMatcher
{
public:
  AddressMatcher()
  {
    LoadIndex(m_index);
    m_coder = make_unique<search::ReverseGeocoder>(m_index);
  }

  template <typename SponsoredObject>
  void operator()(SponsoredObject & object)
  {
    search::ReverseGeocoder::Address addr;
    m_coder->GetNearbyAddress(MercatorBounds::FromLatLon(object.m_latLon), addr);
    object.m_street = addr.GetStreetName();
    object.m_houseNumber = addr.GetHouseNumber();
  }

private:
  Index m_index;
  std::unique_ptr<search::ReverseGeocoder> m_coder;
};

// SponsoredDataset --------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(std::string const & dataPath,
                                                    std::string const & addressReferencePath)
  : m_storage(kDistanceLimitInMeters, kMaxSelectedElements)
{
  InitStorage();
  m_storage.LoadData(dataPath, addressReferencePath);
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::InitStorage()
{
  using Container = typename SponsoredObjectStorage<SponsoredObject>::ObjectsContainer;

  m_storage.SetFillObjects([](Container & objects) {
    AddressMatcher addressMatcher;

    size_t matchedCount = 0;
    size_t emptyCount = 0;
    for (auto & item : objects)
    {
      auto & object = item.second;
      addressMatcher(object);

      if (object.m_address.empty())
        ++emptyCount;
      if (object.HasAddresParts())
        ++matchedCount;
    }

    LOG(LINFO, ("Num of objects:", objects.size(), "matched:", matchedCount,
                "empty addresses:", emptyCount));
  });
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const
{
  for (auto const & item : m_storage.GetObjects())
    BuildObject(item.second, fn);
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::ObjectId
SponsoredDataset<SponsoredObject>::FindMatchingObjectId(FeatureBuilder1 const & fb) const
{
  if (NecessaryMatchingConditionHolds(fb))
    return FindMatchingObjectIdImpl(fb);
  return Object::InvalidObjectId();
}
}  // namespace generator

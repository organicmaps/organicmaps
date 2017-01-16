#include "generator/sponsored_dataset.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"

namespace generator
{
// AddressMatcher ----------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::AddressMatcher::AddressMatcher()
{
  vector<platform::LocalCountryFile> localFiles;

  Platform & platform = GetPlatform();
  platform::FindAllLocalMapsInDirectoryAndCleanup(platform.WritableDir(), 0 /* version */,
                                                  -1 /* latestVersion */, localFiles);

  for (platform::LocalCountryFile const & localFile : localFiles)
  {
    LOG(LINFO, ("Found mwm:", localFile));
    try
    {
      m_index.RegisterMap(localFile);
    }
    catch (RootException const & ex)
    {
      CHECK(false, (ex.Msg(), "Bad mwm file:", localFile));
    }
  }

  m_coder = make_unique<search::ReverseGeocoder>(m_index);
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::AddressMatcher::operator()(Object & object)
{
  search::ReverseGeocoder::Address addr;
  m_coder->GetNearbyAddress(MercatorBounds::FromLatLon(object.m_latLon), addr);
  object.m_street = addr.GetStreetName();
  object.m_houseNumber = addr.GetHouseNumber();
}


// SponsoredDataset --------------------------------------------------------------------------------
template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(string const & dataPath, string const & addressReferencePath)
{
  if (dataPath.empty())
    return;

  ifstream dataSource(dataPath);
  if (!dataSource.is_open())
  {
    LOG(LERROR, ("Error while opening", dataPath, ":", strerror(errno)));
    return;
  }

  LoadData(dataSource, addressReferencePath);
}

template <typename SponsoredObject>
SponsoredDataset<SponsoredObject>::SponsoredDataset(istream & dataSource, string const & addressReferencePath)
{
  LoadData(dataSource, addressReferencePath);
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::Object const &
SponsoredDataset<SponsoredObject>::GetObjectById(ObjectId id) const
{
  auto const it = m_objects.find(id);
  CHECK(it != end(m_objects), ("Got wrong object id:", id));
  return it->second;
}

template <typename SponsoredObject>
typename SponsoredDataset<SponsoredObject>::Object &
SponsoredDataset<SponsoredObject>::GetObjectById(ObjectId id)
{
  auto const it = m_objects.find(id);
  CHECK(it != end(m_objects), ("Got wrong object id:", id));
  return it->second;
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const
{
  for (auto const & item : m_objects)
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

template <typename SponsoredObject>
vector<typename SponsoredDataset<SponsoredObject>::ObjectId>
SponsoredDataset<SponsoredObject>::GetNearestObjects(ms::LatLon const & latLon, size_t const limit,
                                                     double const maxDistanceMeters /* = 0.0 */) const
{
  namespace bgi = boost::geometry::index;

  vector<ObjectId> indexes;
  for_each(bgi::qbegin(m_rtree, bgi::nearest(Point(latLon.lat, latLon.lon), static_cast<unsigned>(limit))),
           bgi::qend(m_rtree), [this, &latLon, &indexes, maxDistanceMeters](Value const & v)
           {
             auto const & object = GetObjectById(v.second);
             double const dist = ms::DistanceOnEarth(latLon, object.m_latLon);
             if (maxDistanceMeters != 0.0 && dist > maxDistanceMeters /* max distance in meters */)
               return;

             indexes.emplace_back(v.second);
           });

  return indexes;
}

template <typename SponsoredObject>
void SponsoredDataset<SponsoredObject>::LoadData(istream & src, string const & addressReferencePath)
{
  m_objects.clear();
  m_rtree.clear();

  for (string line; getline(src, line);)
  {
    Object hotel(line);
    m_objects.emplace(hotel.m_id, hotel);
  }

  // Try to get object address from existing MWMs.
  if (!addressReferencePath.empty())
  {
    LOG(LINFO, ("Reference addresses for sponsored objects", addressReferencePath));
    Platform & platform = GetPlatform();
    string const backupPath = platform.WritableDir();

    // MWMs can be loaded only from a writebledir or from a resourcedir,
    // changig resourcedir can lead to probles with classificator, so
    // we change writebledir.
    platform.SetWritableDirForTests(addressReferencePath);

    AddressMatcher addressMatcher;

    size_t matchedCount = 0;
    size_t emptyCount = 0;
    for (auto & item : m_objects)
    {
      auto & object = item.second;
      addressMatcher(object);

      if (object.m_address.empty())
        ++emptyCount;
      if (object.HasAddresParts())
        ++matchedCount;
    }
    LOG(LINFO,
        ("Num of hotels:", m_objects.size(), "matched:", matchedCount, "empty addresses:", emptyCount));
    platform.SetWritableDirForTests(backupPath);
  }

  for (auto const & item : m_objects)
  {
    auto const & object = item.second;
    Box b(Point(object.m_latLon.lat, object.m_latLon.lon),
          Point(object.m_latLon.lat, object.m_latLon.lon));
    m_rtree.insert(make_pair(b, object.m_id));
  }
}
}  // namespace generator

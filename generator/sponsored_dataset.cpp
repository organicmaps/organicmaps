#include "generator/sponsored_dataset.hpp"

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/limits.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"

namespace generator
{
namespace
{
string EscapeTabs(string const & str)
{
  stringstream ss;
  for (char c : str)
  {
    if (c == '\t')
      ss << "\\t";
    else
      ss << c;
  }
  return ss.str();
}
}  // namespace

SponsoredDataset::Object::Object(string const & src)
{
  vector<string> rec;
  strings::ParseCSVRow(src, '\t', rec);
  CHECK(rec.size() == FieldsCount(), ("Error parsing hotels.tsv line:", EscapeTabs(src)));

  strings::to_uint(rec[Index(Fields::Id)], id.Get());
  strings::to_double(rec[Index(Fields::Latitude)], lat);
  strings::to_double(rec[Index(Fields::Longtitude)], lon);

  name = rec[Index(Fields::Name)];
  address = rec[Index(Fields::Address)];

  strings::to_uint(rec[Index(Fields::Stars)], stars);
  strings::to_uint(rec[Index(Fields::PriceCategory)], priceCategory);
  strings::to_double(rec[Index(Fields::RatingBooking)], ratingBooking);
  strings::to_double(rec[Index(Fields::RatingUsers)], ratingUser);

  descUrl = rec[Index(Fields::DescUrl)];

  strings::to_uint(rec[Index(Fields::Type)], type);

  translations = rec[Index(Fields::Translations)];
}

ostream & operator<<(ostream & s, SponsoredDataset::Object const & h)
{
  s << fixed << setprecision(7);
  return s << "Id: " << h.id << "\t Name: " << h.name << "\t Address: " << h.address
           << "\t lat: " << h.lat << " lon: " << h.lon;
}

SponsoredDataset::ObjectId const SponsoredDataset::kInvalidObjectId =
  SponsoredDataset::ObjectId(numeric_limits<SponsoredDataset::ObjectId::RepType>::max());

SponsoredDatasetBase::SponsoredDatasetBase(string const & dataPath, string const & addressReferencePath)
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

SponsoredDatasetBase::SponsoredDatasetBase(istream & dataSource, string const & addressReferencePath)
{
  LoadData(dataSource, addressReferencePath);
}

SponsoredDataset::Object const & SponsoredDatasetBase::GetObjectById(SponsoredDataset::ObjectId id) const
{
  auto const it = m_hotels.find(id);
  CHECK(it != end(m_hotels), ("Got wrong object id:", id));
  return it->second;
}

SponsoredDataset::Object & SponsoredDatasetBase::GetObjectById(SponsoredDataset::ObjectId id)
{
  auto const it = m_hotels.find(id);
  CHECK(it != end(m_hotels), ("Got wrong object id:", id));
  return it->second;
}

void SponsoredDatasetBase::BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const
{
  for (auto const & item : m_hotels)
    BuildObject(item.second, fn);
}

SponsoredDatasetBase::ObjectId SponsoredDatasetBase::FindMatchingObjectId(FeatureBuilder1 const & fb) const
{
  if (NecessaryMatchingConditionHolds(fb))
    return FindMatchingObjectIdImpl(fb);
  return kInvalidObjectId;
}

vector<SponsoredDataset::ObjectId> SponsoredDatasetBase::GetNearestObjects(
    ms::LatLon const & latLon, size_t const limit,
    double const maxDistance /* = 0.0 */) const
{
  namespace bgi = boost::geometry::index;

  vector<ObjectId> indexes;
  for_each(bgi::qbegin(m_rtree, bgi::nearest(TPoint(latLon.lat, latLon.lon), limit)),
           bgi::qend(m_rtree), [this, &latLon, &indexes, maxDistance](TValue const & v)
           {
             auto const & object = GetObjectById(v.second);
             double const dist = ms::DistanceOnEarth(latLon.lat, latLon.lon, object.lat, object.lon);
             if (maxDistance != 0.0 && dist > maxDistance /* max distance in meters */)
               return;

             indexes.emplace_back(v.second);
           });

  return indexes;
}

void SponsoredDatasetBase::LoadData(istream & src, string const & addressReferencePath)
{
  m_hotels.clear();
  m_rtree.clear();

  for (string line; getline(src, line);)
  {
    Object hotel(line);
    m_hotels.emplace(hotel.id, hotel);
  }

  if (!addressReferencePath.empty())
  {
    LOG(LINFO, ("Reference addresses for booking objects", addressReferencePath));
    Platform & platform = GetPlatform();
    string const backupPath = platform.WritableDir();
    platform.SetWritableDirForTests(addressReferencePath);

    //TODO(mgsergio): AddressMatcher addressMatcher;

    size_t matchedNum = 0;
    size_t emptyAddr = 0;
    for (auto & item : m_hotels)
    {
      auto & hotel = item.second;
      // TODO(mgsergio): addressMatcher(hotel);

      if (hotel.address.empty())
        ++emptyAddr;
      if (hotel.IsAddressPartsFilled())
        ++matchedNum;
    }
    LOG(LINFO,
        ("Num of hotels:", m_hotels.size(), "matched:", matchedNum, "empty addresses:", emptyAddr));
    platform.SetWritableDirForTests(backupPath);
  }

  for (auto const & item : m_hotels)
  {
    auto const & hotel = item.second;
    TBox b(TPoint(hotel.lat, hotel.lon), TPoint(hotel.lat, hotel.lon));
    m_rtree.insert(make_pair(b, hotel.id));
  }
}
}  // namespace generator

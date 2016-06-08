#include "generator/booking_dataset.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"

namespace generator
{
namespace
{
bool CheckForValues(string const & value)
{
  for (char const * val :
       {"hotel", "apartment", "camp_site", "chalet", "guest_house", "hostel", "motel", "resort"})
  {
    if (value == val)
      return true;
  }
  return false;
}
}  // namespace

BookingDataset::Hotel::Hotel(string const & src)
{
  vector<string> rec(FieldsCount());
  strings::SimpleTokenizer token(src, "\t");
  for (size_t i = 0; token && i < rec.size(); ++i, ++token)
    rec[i] = *token;

  strings::to_uint(rec[Index(Fields::Id)], id);
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
}

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h)
{
  return s << "Name: " << h.name << "\t Address: " << h.address << "\t lat: " << h.lat
           << " lon: " << h.lon;
}

BookingDataset::BookingDataset(string const & dataPath)
{
  LoadHotels(dataPath);

  size_t counter = 0;
  for (auto const & hotel : m_hotels)
  {
    TBox b(TPoint(hotel.lat, hotel.lon), TPoint(hotel.lat, hotel.lon));
    m_rtree.insert(std::make_pair(b, counter));
    ++counter;
  }
}

bool BookingDataset::BookingFilter(OsmElement const & e) const
{
  return Filter(e, [&](OsmElement const & e)
                {
                  return MatchWithBooking(e);
                });
}

bool BookingDataset::TourismFilter(OsmElement const & e) const
{
  return Filter(e, [&](OsmElement const & e)
                {
                  return true;
                });
}

BookingDataset::Hotel const & BookingDataset::GetHotel(size_t index) const
{
  ASSERT_GREATER(m_hotels.size(), index, ());
  return m_hotels[index];
}

vector<size_t> BookingDataset::GetNearestHotels(double lat, double lon, size_t limit,
                                                double maxDistance /* = 0.0 */) const
{
  namespace bgi = boost::geometry::index;

  vector<size_t> indexes;
  for_each(bgi::qbegin(m_rtree, bgi::nearest(TPoint(lat, lon), limit)), bgi::qend(m_rtree),
           [&](TValue const & v)
           {
             auto const & hotel = m_hotels[v.second];
             double const dist = ms::DistanceOnEarth(lat, lon, hotel.lat, hotel.lon);
             if (maxDistance != 0.0 && dist > maxDistance /* max distance in meters */)
               return;

             indexes.emplace_back(v.second);
           });
  return indexes;
}

bool BookingDataset::MatchByName(string const & osmName,
                                 vector<size_t> const & bookingIndexes) const
{
  return false;

  // Match name.
  //  vector<strings::UniString> osmTokens;
  //  NormalizeAndTokenizeString(name, osmTokens, search::Delimiters());
  //
  //  cout << "\n------------- " << name << endl;
  //
  //  bool matched = false;
  //  for (auto const & index : indexes)
  //  {
  //    vector<strings::UniString> bookingTokens;
  //    NormalizeAndTokenizeString(m_hotels[index].name, bookingTokens, search::Delimiters());
  //
  //    map<size_t, vector<pair<size_t, size_t>>> weightPair;
  //
  //    for (size_t j = 0; j < osmTokens.size(); ++j)
  //    {
  //      for (size_t i = 0; i < bookingTokens.size(); ++i)
  //      {
  //        size_t distance = strings::EditDistance(osmTokens[j].begin(), osmTokens[j].end(),
  //                                                bookingTokens[i].begin(),
  //                                                bookingTokens[i].end());
  //        if (distance < 3)
  //          weightPair[distance].emplace_back(i, j);
  //      }
  //    }
  //
  //    if (!weightPair.empty())
  //    {
  //      cout << m_hotels[e.second] << endl;
  //      matched = true;
  //    }
  //  }
}

void BookingDataset::BuildFeatures(function<void(OsmElement *)> const & fn) const
{
  for (auto const & hotel : m_hotels)
  {
    OsmElement e;
    e.type = OsmElement::EntityType::Node;
    e.id = 1;

    e.lat = hotel.lat;
    e.lon = hotel.lon;

    e.AddTag("sponsored", "booking");
    e.AddTag("name", hotel.name);
    e.AddTag("ref:sponsored", strings::to_string(hotel.id));
    e.AddTag("website", hotel.descUrl);
    e.AddTag("rating:sponsored", strings::to_string(hotel.ratingUser));
    e.AddTag("stars", strings::to_string(hotel.stars));
    e.AddTag("price_rate", strings::to_string(hotel.priceCategory));
    e.AddTag("addr:full", hotel.address);

    switch (hotel.type)
    {
    case 19:
    case 205: e.AddTag("tourism", "motel"); break;

    case 21:
    case 206:
    case 212: e.AddTag("tourism", "resort"); break;

    case 3:
    case 23:
    case 24:
    case 25:
    case 202:
    case 207:
    case 208:
    case 209:
    case 210:
    case 216:
    case 220:
    case 223: e.AddTag("tourism", "guest_house"); break;

    case 14:
    case 204:
    case 213:
    case 218:
    case 219:
    case 226:
    case 222: e.AddTag("tourism", "hotel"); break;

    case 211:
    case 224:
    case 228: e.AddTag("tourism", "chalet"); break;

    case 13:
    case 225:
    case 203: e.AddTag("tourism", "hostel"); break;

    case 215:
    case 221:
    case 227:
    case 2:
    case 201: e.AddTag("tourism", "apartment"); break;

    case 214: e.AddTag("tourism", "camp_site"); break;

    default: e.AddTag("tourism", "hotel"); break;
    }

    fn(&e);
  }
}

// static
double BookingDataset::ScoreByLinearNormDistance(double distance)
{
  distance = my::clamp(distance, 0, kDistanceLimitInMeters);
  return 1.0 - distance / kDistanceLimitInMeters;
}

void BookingDataset::LoadHotels(string const & path)
{
  m_hotels.clear();

  if (path.empty())
    return;

  ifstream src(path);
  if (!src.is_open())
  {
    LOG(LERROR, ("Error while opening", path, ":", strerror(errno)));
    return;
  }

  for (string line; getline(src, line);)
    m_hotels.emplace_back(line);
}

bool BookingDataset::MatchWithBooking(OsmElement const & e) const
{
  string name;
  for (auto const & tag : e.Tags())
  {
    if (tag.key == "name")
    {
      name = tag.value;
      break;
    }
  }

  if (name.empty())
    return false;

  // Find |kMaxSelectedElements| nearest values to a point.
  auto const bookingIndexes = GetNearestHotels(e.lat, e.lon, kMaxSelectedElements, kDistanceLimitInMeters);

  bool matched = false;

  for (size_t const j : bookingIndexes)
  {
    auto const & hotel = GetHotel(j);
    double const distanceMeters = ms::DistanceOnEarth(e.lat, e.lon, hotel.lat, hotel.lon);
    double score = ScoreByLinearNormDistance(distanceMeters);
    matched = score > kOptimalThreshold;
    if (matched)
      break;
  }

  return matched;
}

bool BookingDataset::Filter(OsmElement const & e,
                            function<bool(OsmElement const &)> const & fn) const
{
  if (e.type != OsmElement::EntityType::Node)
    return false;

  if (e.Tags().empty())
    return false;

  bool matched = false;
  for (auto const & tag : e.Tags())
  {
    if (tag.key == "tourism" && CheckForValues(tag.value))
    {
      matched = fn(e);
      break;
    }
  }

  // TODO: Need to write file with dropped osm features.

  return matched;
}

}  // namespace generator

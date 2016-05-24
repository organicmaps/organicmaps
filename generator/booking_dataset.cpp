#include "generator/booking_dataset.hpp"

#include "base/string_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"

BookingDataset::BookingHotel::BookingHotel(string const & src)
{
  stringstream ss(src);
  string elem;
  vector<string> rec(FieldsCount());
  for (size_t i = 0; getline(ss, elem, '\t') && i < rec.size(); ++i)
    rec[i] = elem;

  id = static_cast<uint32_t>(strtoul(rec[Index(Fields::Id)].c_str(), nullptr, 10));

  lat = strtod(rec[Index(Fields::Latitude)].c_str(), nullptr);
  lon = strtod(rec[Index(Fields::Longtitude)].c_str(), nullptr);
  name = rec[Index(Fields::Name)];
  address = rec[Index(Fields::Address)];

  stars = rec[Index(Fields::Stars)].empty()
              ? 0
              : static_cast<uint32_t>(strtoul(rec[Index(Fields::Stars)].c_str(), nullptr, 10));

  priceCategory =
      rec[Index(Fields::PriceCategory)].empty()
          ? 0
          : static_cast<uint32_t>(strtoul(rec[Index(Fields::PriceCategory)].c_str(), nullptr, 10));

  ratingBooking = rec[Index(Fields::RatingBooking)].empty()
                      ? 0
                      : strtod(rec[Index(Fields::RatingBooking)].c_str(), nullptr);

  ratingUser = rec[Index(Fields::RatingUsers)].empty()
                   ? 0
                   : strtod(rec[Index(Fields::RatingUsers)].c_str(), nullptr);

  descUrl = rec[Index(Fields::DescUrl)];

  type = rec[Index(Fields::Type)].empty()
             ? 0
             : static_cast<uint32_t>(strtoul(rec[Index(Fields::Type)].c_str(), nullptr, 10));
}

ostream & operator<<(ostream & s, BookingDataset::BookingHotel const & h)
{
  return s << "Name: " << h.name << " lon: " << h.lon << " lat: " << h.lat;
}

void BookingDataset::LoadBookingHotels(string const & path)
{
  m_hotels.clear();
  
  if(path.empty())
    return;
  
  ifstream src(path);
  for (string elem; getline(src, elem);)
    m_hotels.emplace_back(elem);
}

BookingDataset::BookingDataset(string const & dataPath)
{
  LoadBookingHotels(dataPath);

  size_t counter = 0;
  for (auto const & hotel : m_hotels)
  {
    TBox b(TPoint(hotel.lon, hotel.lat), TPoint(hotel.lon, hotel.lat));
    m_rtree.insert(std::make_pair(b, counter++));
  }
}

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

  // Find 3 nearest values to a point.
  vector<TValue> result;
  for_each(boost::geometry::index::qbegin(m_rtree,
                                          boost::geometry::index::nearest(TPoint(e.lon, e.lat), 3)),
           boost::geometry::index::qend(m_rtree), [&](TValue const & v)
           {
             auto const & hotel = m_hotels[v.second];
             double dist = ms::DistanceOnEarth(e.lon, e.lat, hotel.lon, hotel.lat);
             if (dist > 150 /* max distance in meters */)
               return;

             result.emplace_back(v);
           });

  if (result.empty())
    return false;

  // Match name.
  vector<strings::UniString> osmTokens;
  NormalizeAndTokenizeString(name, osmTokens, search::Delimiters());

  //  cout << "\n------------- " << name << endl;

  bool matched = false;
  for (auto const & e : result)
  {
    vector<strings::UniString> bookingTokens;
    NormalizeAndTokenizeString(m_hotels[e.second].name, bookingTokens, search::Delimiters());

    map<size_t, vector<pair<size_t, size_t>>> weightPair;

    for (size_t j = 0; j < osmTokens.size(); ++j)
    {
      for (size_t i = 0; i < bookingTokens.size(); ++i)
      {
        size_t distance = strings::EditDistance(osmTokens[j].begin(), osmTokens[j].end(),
                                                bookingTokens[i].begin(), bookingTokens[i].end());
        if (distance < 3)
          weightPair[distance].emplace_back(i, j);
      }
    }

    if (!weightPair.empty())
    {
      //      cout << m_hotels[e.second] << endl;
      matched = true;
    }
  }
  return matched;
}

bool BookingDataset::Filter(OsmElement const & e) const
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
      matched = MatchWithBooking(e);
      break;
    }
  }

  // TODO: Need to write file with dropped osm features.

  return matched;
}

void BookingDataset::BuildFeatures(function<void(OsmElement *)> const & fn) const
{
  for (auto const & hotel : m_hotels)
  {
    OsmElement e;
    e.type = OsmElement::EntityType::Node;
    e.id = 1;

    e.lon = hotel.lon;
    e.lat = hotel.lat;

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

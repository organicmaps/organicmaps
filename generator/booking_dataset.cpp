#include "generator/booking_dataset.hpp"

#include "generator/booking_scoring.hpp"
#include "generator/feature_builder.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

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

BookingDataset::Hotel::Hotel(string const & src)
{
  vector<string> rec;
  strings::ParseCSVRow(src, '\t', rec);
  CHECK(rec.size() == FieldsCount(), ("Error parsing hotels.tsv line:", EscapeTabs(src)));

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

  translations = rec[Index(Fields::Translations)];
}

ostream & operator<<(ostream & s, BookingDataset::Hotel const & h)
{
  s << fixed << setprecision(7);
  return s << "Name: " << h.name << "\t Address: " << h.address << "\t lat: " << h.lat
           << " lon: " << h.lon;
}

BookingDataset::AddressMatcher::AddressMatcher()
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
      CHECK(false, ("Bad mwm file:", localFile));
    }
  }

  m_coder = make_unique<search::ReverseGeocoder>(m_index);
}

void BookingDataset::AddressMatcher::operator()(Hotel & hotel)
{
  search::ReverseGeocoder::Address addr;
  m_coder->GetNearbyAddress(MercatorBounds::FromLatLon(hotel.lat, hotel.lon), addr);
  hotel.street = addr.GetStreetName();
  hotel.houseNumber = addr.GetHouseNumber();
}

BookingDataset::BookingDataset(string const & dataPath, string const & addressReferencePath)
{
  if (dataPath.empty())
    return;

  ifstream dataSource(dataPath);
  if (!dataSource.is_open())
  {
    LOG(LERROR, ("Error while opening", dataPath, ":", strerror(errno)));
    return;
  }

  LoadHotels(dataSource, addressReferencePath);
}

BookingDataset::BookingDataset(istream & dataSource, string const & addressReferencePath)
{
  LoadHotels(dataSource, addressReferencePath);
}

size_t BookingDataset::GetMatchingHotelIndex(FeatureBuilder1 const & fb) const
{
  if (CanBeBooking(fb))
    return MatchWithBooking(fb);
  return kInvalidHotelIndex;
}

bool BookingDataset::CanBeBooking(FeatureBuilder1 const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsHotelChecker::Instance()(fb.GetTypes());
}

BookingDataset::Hotel const & BookingDataset::GetHotel(size_t index) const
{
  ASSERT_LESS(index, m_hotels.size(), ());
  return m_hotels[index];
}

BookingDataset::Hotel & BookingDataset::GetHotel(size_t index)
{
  ASSERT_LESS(index, m_hotels.size(), ());
  return m_hotels[index];
}

vector<size_t> BookingDataset::GetNearestHotels(ms::LatLon const & latLon, size_t const limit,
                                                double const maxDistance /* = 0.0 */) const
{
  namespace bgi = boost::geometry::index;

  vector<size_t> indexes;
  for_each(bgi::qbegin(m_rtree, bgi::nearest(TPoint(latLon.lat, latLon.lon), limit)),
           bgi::qend(m_rtree), [&](TValue const & v)
           {
             auto const & hotel = GetHotel(v.second);
             double const dist = ms::DistanceOnEarth(latLon.lat, latLon.lon, hotel.lat, hotel.lon);
             if (maxDistance != 0.0 && dist > maxDistance /* max distance in meters */)
               return;

             indexes.emplace_back(v.second);
           });

  return indexes;
}

void BookingDataset::BuildHotel(size_t const hotelIndex,
                                function<void(FeatureBuilder1 &)> const & fn) const
{
  auto const & hotel = m_hotels[hotelIndex];

  FeatureBuilder1 fb;
  FeatureParams params;

  fb.SetCenter(MercatorBounds::FromLatLon(hotel.lat, hotel.lon));

  auto & metadata = params.GetMetadata();
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(hotel.id));
  metadata.Set(feature::Metadata::FMD_WEBSITE, hotel.descUrl);
  metadata.Set(feature::Metadata::FMD_RATING, strings::to_string(hotel.ratingUser));
  metadata.Set(feature::Metadata::FMD_STARS, strings::to_string(hotel.stars));
  metadata.Set(feature::Metadata::FMD_PRICE_RATE, strings::to_string(hotel.priceCategory));

  // params.AddAddress(hotel.address);
  // TODO(mgsergio): addr:full ???

  if (!hotel.street.empty())
    fb.AddStreet(hotel.street);

  if (!hotel.houseNumber.empty())
    fb.AddHouseNumber(hotel.houseNumber);

  params.AddName(StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode),
                 hotel.name);
  if (!hotel.translations.empty())
  {
    // TODO(mgsergio): Move parsing to the hotel costruction stage.
    vector<string> parts;
    strings::ParseCSVRow(hotel.translations, '|', parts);
    CHECK_EQUAL(parts.size() % 3, 0, ("Invalid translation string:", hotel.translations));
    for (auto i = 0; i < parts.size(); i += 3)
    {
      auto const langCode = StringUtf8Multilang::GetLangIndex(parts[i]);
      params.AddName(StringUtf8Multilang::GetLangByCode(langCode), parts[i + 1]);
      // TODO(mgsergio): e.AddTag("addr:full:" + parts[i], parts[i + 2]);
    }
  }

  auto const & clf = classif();
  params.AddType(clf.GetTypeByPath({"sponsored", "booking"}));
  // Matching booking.com hotel types to OpenStreetMap values.
  // Booking types are listed in the closed API docs.
  switch (hotel.type)
  {
    case 19:
    case 205: params.AddType(clf.GetTypeByPath({"tourism", "motel"})); break;

    case 21:
    case 206:
    case 212: params.AddType(clf.GetTypeByPath({"tourism", "resort"})); break;

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
    case 223: params.AddType(clf.GetTypeByPath({"tourism", "guest_house"})); break;

    case 14:
    case 204:
    case 213:
    case 218:
    case 219:
    case 226:
    case 222: params.AddType(clf.GetTypeByPath({"tourism", "hotel"})); break;

    case 211:
    case 224:
    case 228: params.AddType(clf.GetTypeByPath({"tourism", "chalet"})); break;

    case 13:
    case 225:
    case 203: params.AddType(clf.GetTypeByPath({"tourism", "hostel"})); break;

    case 215:
    case 221:
    case 227:
    case 2:
    case 201: params.AddType(clf.GetTypeByPath({"tourism", "apartment"})); break;

    case 214: params.AddType(clf.GetTypeByPath({"tourism", "camp_site"})); break;

    default: params.AddType(clf.GetTypeByPath({"tourism", "hotel"})); break;
  }

  fb.SetParams(params);

  fn(fb);
}

void BookingDataset::LoadHotels(istream & src, string const & addressReferencePath)
{
  m_hotels.clear();
  m_rtree.clear();

  for (string line; getline(src, line);)
    m_hotels.emplace_back(line);

  if (!addressReferencePath.empty())
  {
    LOG(LINFO, ("Reference addresses for booking objects", addressReferencePath));
    Platform & platform = GetPlatform();
    string const backupPath = platform.WritableDir();
    platform.SetWritableDirForTests(addressReferencePath);

    AddressMatcher addressMatcher;

    size_t matchedNum = 0;
    size_t emptyAddr = 0;
    for (Hotel & hotel : m_hotels)
    {
      addressMatcher(hotel);

      if (hotel.address.empty())
        ++emptyAddr;
      if (hotel.IsAddressPartsFilled())
        ++matchedNum;
    }
    LOG(LINFO,
        ("Num of hotels:", m_hotels.size(), "matched:", matchedNum, "empty addresses:", emptyAddr));
    platform.SetWritableDirForTests(backupPath);
  }

  size_t counter = 0;
  for (auto const & hotel : m_hotels)
  {
    TBox b(TPoint(hotel.lat, hotel.lon), TPoint(hotel.lat, hotel.lon));
    m_rtree.insert(make_pair(b, counter));
    ++counter;
  }
}

size_t BookingDataset::MatchWithBooking(FeatureBuilder1 const & fb) const
{
  auto const name = fb.GetName(StringUtf8Multilang::kDefaultCode);

  if (name.empty())
    return false;

  // Find |kMaxSelectedElements| nearest values to a point.
  auto const bookingIndexes = GetNearestHotels(MercatorBounds::ToLatLon(fb.GetKeyPoint()),
                                               kMaxSelectedElements, kDistanceLimitInMeters);

  for (size_t const j : bookingIndexes)
  {
    if (booking_scoring::Match(GetHotel(j), fb).IsMatched())
      return j;
  }

  return kInvalidHotelIndex;
}
}  // namespace generator

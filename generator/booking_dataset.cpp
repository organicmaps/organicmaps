#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "boost/algorithm/string/replace.hpp"


namespace generator
{
using namespace feature;

// BookingHotel ------------------------------------------------------------------------------------
BookingHotel::BookingHotel(std::string src)
{
  /// @todo For fast parsing we can preprocess src (quotes) and return string_view's.
  std::vector<std::string> rec;
  strings::ParseCSVRow(src, '\t', rec);

  CHECK_EQUAL(rec.size(), Fields::Counter,
              ("Error parsing hotels entry:", boost::replace_all_copy(src, "\t", "\\t")));

  // Assign id in the end in case of possible errors.
  uint32_t id;
  CLOG(LDEBUG, strings::to_uint(rec[Fields::Id], id), ());
  CLOG(LDEBUG, strings::to_double(rec[Fields::Latitude], m_latLon.m_lat), ());
  CLOG(LDEBUG, strings::to_double(rec[Fields::Longitude], m_latLon.m_lon), ());

  m_name = rec[Fields::Name];
  m_address = rec[Fields::Address];

  CLOG(LDEBUG, strings::to_uint(rec[Fields::Stars], m_stars), ());
  CLOG(LDEBUG, strings::to_uint(rec[Fields::PriceCategory], m_priceCategory), ());
  CLOG(LDEBUG, strings::to_double(rec[Fields::RatingBooking], m_ratingBooking), ());
  CLOG(LDEBUG, strings::to_double(rec[Fields::RatingUsers], m_ratingUser), ());

  m_descUrl = rec[Fields::DescUrl];

  CLOG(LDEBUG, strings::to_uint(rec[Fields::Type], m_type), ());

  m_translations = rec[Fields::Translations];

  m_id.Set(id);
}


// BookingDataset ----------------------------------------------------------------------------------
template <>
bool BookingDataset::IsSponsoredCandidate(FeatureBuilder const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsHotelChecker::Instance()(fb.GetTypes());
}

template <>
void BookingDataset::PreprocessMatchedOsmObject(ObjectId, FeatureBuilder & fb, FBuilderFnT const fn) const
{
  // Turn a hotel into a simple building.
  if (fb.GetGeomType() == GeomType::Area)
  {
    // Remove all information about the hotel.
    auto & meta = fb.GetMetadata();
    meta.Drop(Metadata::EType::FMD_STARS);
    meta.Drop(Metadata::EType::FMD_WEBSITE);
    meta.Drop(Metadata::EType::FMD_PHONE_NUMBER);

    auto & params = fb.GetParams();
    params.ClearName();

    auto const tourism = classif().GetTypeByPath({"tourism"});
    base::EraseIf(params.m_types, [tourism](uint32_t type)
    {
      ftype::TruncValue(type, 1);
      return type == tourism;
    });
  }

  fn(fb);
}

template <>
void BookingDataset::BuildObject(Object const & hotel, FBuilderFnT const & fn) const
{
  FeatureBuilder fb;

  fb.SetCenter(mercator::FromLatLon(hotel.m_latLon.m_lat, hotel.m_latLon.m_lon));

  /// @todo SRC_BOOKING
  //fb.SetHotelInfo(Metadata::SRC_KAYAK, hotel.m_id.Get(), hotel.m_ratingUser, hotel.m_priceCategory);
  auto & metadata = fb.GetMetadata();
  metadata.Set(Metadata::FMD_WEBSITE, hotel.m_descUrl);
  metadata.Set(Metadata::FMD_STARS, strings::to_string(hotel.m_stars));

  auto & params = fb.GetParams();
  if (!hotel.m_street.empty())
    params.SetStreet(hotel.m_street);

  if (!hotel.m_houseNumber.empty())
    params.AddHouseNumber(hotel.m_houseNumber);

  if (!hotel.m_translations.empty())
  {
    // TODO(mgsergio): Move parsing to the hotel costruction stage.
    std::vector<std::string> parts;
    strings::ParseCSVRow(hotel.m_translations, '|', parts);
    CHECK_EQUAL(parts.size() % 3, 0, ("Invalid translation string:", hotel.m_translations));
    for (size_t i = 0; i < parts.size(); i += 3)
    {
      auto const langCode = StringUtf8Multilang::GetLangIndex(parts[i]);
      params.AddName(StringUtf8Multilang::GetLangByCode(langCode), parts[i + 1]);
      // TODO(mgsergio): e.AddTag("addr:full:" + parts[i], parts[i + 2]);
    }
  }
  params.AddName(StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kEnglishCode), hotel.m_name);

  auto const & clf = classif();
  params.AddType(clf.GetTypeByPath({"sponsored", "booking"}));
  // Matching booking.com hotel types to OpenStreetMap values.
  // Booking types are listed in the closed API docs.
  switch (hotel.m_type)
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

  fn(fb);
}

} // namespace generator

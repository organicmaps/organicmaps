#include "generator/booking_dataset.hpp"

#include "generator/booking_scoring.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/string_utils.hpp"

namespace generator
{
// BookingDataset::AddressMatcher::AddressMatcher()
// {
//   vector<platform::LocalCountryFile> localFiles;

//   Platform & platform = GetPlatform();
//   platform::FindAllLocalMapsInDirectoryAndCleanup(platform.WritableDir(), 0 /* version */,
//                                                   -1 /* latestVersion */, localFiles);

//   for (platform::LocalCountryFile const & localFile : localFiles)
//   {
//     LOG(LINFO, ("Found mwm:", localFile));
//     try
//     {
//       m_index.RegisterMap(localFile);
//     }
//     catch (RootException const & ex)
//     {
//       CHECK(false, ("Bad mwm file:", localFile));
//     }
//   }

//   m_coder = make_unique<search::ReverseGeocoder>(m_index);
// }

// void BookingDataset::AddressMatcher::operator()(Hotel & hotel)
// {
//   search::ReverseGeocoder::Address addr;
//   m_coder->GetNearbyAddress(MercatorBounds::FromLatLon(hotel.lat, hotel.lon), addr);
//   hotel.street = addr.GetStreetName();
//   hotel.houseNumber = addr.GetHouseNumber();
// }

bool BookingDataset::NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const
{
  if (fb.GetName(StringUtf8Multilang::kDefaultCode).empty())
    return false;

  return ftypes::IsHotelChecker::Instance()(fb.GetTypes());
}

void BookingDataset::BuildObject(SponsoredDataset::Object const & hotel,
                                 function<void(FeatureBuilder1 &)> const & fn) const
{
  FeatureBuilder1 fb;
  FeatureParams params;

  fb.SetCenter(MercatorBounds::FromLatLon(hotel.lat, hotel.lon));

  auto & metadata = params.GetMetadata();
  metadata.Set(feature::Metadata::FMD_SPONSORED_ID, strings::to_string(hotel.id.Get()));
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

BookingDataset::ObjectId BookingDataset::FindMatchingObjectIdImpl(FeatureBuilder1 const & fb) const
{
  auto const name = fb.GetName(StringUtf8Multilang::kDefaultCode);

  if (name.empty())
    return kInvalidObjectId;

  // Find |kMaxSelectedElements| nearest values to a point.
  auto const bookingIndexes = GetNearestObjects(MercatorBounds::ToLatLon(fb.GetKeyPoint()),
                                                kMaxSelectedElements, kDistanceLimitInMeters);

  for (auto const j : bookingIndexes)
  {
    if (booking_scoring::Match(GetObjectById(j), fb).IsMatched())
      return j;
  }

  return kInvalidObjectId;
}
}  // namespace generator

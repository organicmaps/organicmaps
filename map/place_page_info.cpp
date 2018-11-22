#include "map/place_page_info.hpp"
#include "map/bookmark_helpers.hpp"
#include "map/reachable_by_taxi_checker.hpp"

#include "descriptions/loader.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"
#include "partners_api/partners.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/feature_source.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_sponsored.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include <sstream>

#include "private.h"

namespace place_page
{
namespace
{
auto constexpr kTopRatingBound = 10.0f;
}  // namespace

char const * const Info::kSubtitleSeparator = " • ";
char const * const Info::kStarSymbol = "★";
char const * const Info::kMountainSymbol = "▲";
char const * const Info::kPricingSymbol = "$";
char const * const kWheelchairSymbol = u8"\u267F";

bool Info::ShouldShowAddPlace() const
{
  auto const isPointOrBuilding = IsPointType() || IsBuilding();
  return m_canEditOrAdd && !(IsFeature() && isPointOrBuilding);
}

bool Info::ShouldShowUGC() const
{
  return ftraits::UGC::IsUGCAvailable(m_sortedTypes) &&
         (m_featureStatus == FeatureStatus::Untouched ||
          m_featureStatus == FeatureStatus::Modified);
}

void Info::SetFromFeatureType(FeatureType & ft)
{
  MapObject::SetFromFeatureType(ft);
  std::string primaryName;
  std::string secondaryName;
  GetPrefferedNames(primaryName, secondaryName);
  m_sortedTypes = m_types;
  m_sortedTypes.SortBySpec();
  m_primaryFeatureName = primaryName;
  if (IsBookmark())
  {
    m_uiTitle = GetBookmarkName();

    auto const secondaryTitle = m_customName.empty() ? primaryName : m_customName;
    if (m_uiTitle != secondaryTitle)
      m_uiSecondaryTitle = secondaryTitle;

    m_uiSubtitle = FormatSubtitle(true /* withType */);
    m_uiAddress = m_address;
  }
  else if (!primaryName.empty())
  {
    m_uiTitle = primaryName;
    m_uiSecondaryTitle = secondaryName;
    m_uiSubtitle = FormatSubtitle(true /* withType */);
    m_uiAddress = m_address;
  }
  else if (IsBuilding())
  {
    bool const isAddressEmpty = m_address.empty();
    m_uiTitle = isAddressEmpty ? GetLocalizedType() : m_address;
    m_uiSubtitle = FormatSubtitle(!isAddressEmpty /* withType */);
  }
  else
  {
    m_uiTitle = GetLocalizedType();
    m_uiSubtitle = FormatSubtitle(false /* withType */);
    m_uiAddress = m_address;
  }

  m_hotelType = ftypes::IsHotelChecker::Instance().GetHotelType(ft);
}

string Info::FormatSubtitle(bool withType) const
{
  std::vector<std::string> subtitle;

  if (IsBookmark())
    subtitle.push_back(m_bookmarkCategoryName);

  if (withType)
    subtitle.push_back(GetLocalizedType());
  // Flats.
  string const flats = GetFlats();
  if (!flats.empty())
    subtitle.push_back(flats);

  // Cuisines.
  for (string const & cuisine : GetLocalizedCuisines())
    subtitle.push_back(cuisine);

  // Airport IATA code.
  string const iata = GetAirportIata();
  if (!iata.empty())
    subtitle.push_back(iata);

  // Stars.
  string const stars = FormatStars();
  if (!stars.empty())
    subtitle.push_back(stars);

  // Operator.
  string const op = GetOperator();
  if (!op.empty())
    subtitle.push_back(op);

  // Elevation.
  string const eleStr = GetElevationFormatted();
  if (!eleStr.empty())
    subtitle.push_back(kMountainSymbol + eleStr);
  if (HasWifi())
    subtitle.push_back(m_localizedWifiString);

  // Wheelchair
  if (GetWheelchairType() == ftraits::WheelchairAvailability::Yes)
    subtitle.push_back(kWheelchairSymbol);

  return strings::JoinStrings(subtitle, kSubtitleSeparator);
}

void Info::GetPrefferedNames(std::string & primaryName, std::string & secondaryName) const
{
  auto const mwmInfo = GetID().m_mwmId.GetInfo();
  if (mwmInfo)
  {
    auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
    feature::GetPreferredNames(mwmInfo->GetRegionData(), m_name, deviceLang,
                               true /* allowTranslit */, primaryName, secondaryName);
  }
}

std::string Info::GetBookmarkName()
{
  std::string bookmarkName;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();
  if (mwmInfo)
  {
    bookmarkName = GetPreferredBookmarkStr(m_bookmarkData.m_customName, mwmInfo->GetRegionData());
    if (bookmarkName.empty())
      bookmarkName = GetPreferredBookmarkStr(m_bookmarkData.m_name, mwmInfo->GetRegionData());
  }

  if (bookmarkName.empty())
    bookmarkName = GetPreferredBookmarkName(m_bookmarkData);

  return bookmarkName;
}

void Info::SetCustomName(std::string const & name)
{
  if (IsBookmark())
    SetTitlesForBookmark();
  else
    m_uiTitle = name;

  m_customName = name;
}

void Info::SetCustomNameWithCoordinates(m2::PointD const & mercator, std::string const & name)
{
  if (IsBookmark())
  {
    SetTitlesForBookmark();
  }
  else
  {
    m_uiTitle = name;
    m_uiSubtitle = measurement_utils::FormatLatLon(MercatorBounds::YToLat(mercator.y),
                                                   MercatorBounds::XToLon(mercator.x),
                                                   true /* withSemicolon */);
  }
  m_customName = name;
}

void Info::SetBookmarkId(kml::MarkId markId)
{
  m_markId = markId;
  m_uiSubtitle = FormatSubtitle(IsFeature() /* withType */);
}

bool Info::IsNotEditableSponsored() const
{
  return m_sponsoredType == SponsoredType::Booking ||
         m_sponsoredType == SponsoredType::Holiday;
}

bool Info::ShouldShowEditPlace() const
{
  return m_canEditOrAdd &&
         // TODO(mgsergio): Does IsFeature() imply !IsMyPosition()?
         !IsMyPosition() && IsFeature();
}

ftraits::UGCRatingCategories Info::GetRatingCategories() const
{
  return ftraits::UGC::GetCategories(m_sortedTypes);
}

kml::LocalizableString Info::FormatNewBookmarkName() const
{
  kml::LocalizableString bookmarkName;
  if (IsFeature())
  {
    m_name.ForEach([&bookmarkName](int8_t langCode, std::string const & localName)
    {
      if (!localName.empty())
        bookmarkName[langCode] = localName;
    });

    if (bookmarkName.empty() && IsBuilding() && !m_address.empty())
      kml::SetDefaultStr(bookmarkName, m_address);
  }
  else if (!m_uiTitle.empty())
  {
    kml::SetDefaultStr(bookmarkName, m_uiTitle);
  }

  return bookmarkName;
}

string Info::FormatStars() const
{
  string stars;
  for (int i = 0; i < GetStars(); ++i)
    stars.append(kStarSymbol);
  return stars;
}

void Info::SetTitlesForBookmark()
{
  m_uiTitle = GetBookmarkName();

  std::vector<std::string> subtitle;
  subtitle.push_back(m_bookmarkCategoryName);
  if (!m_bookmarkData.m_featureTypes.empty())
    subtitle.push_back(GetLocalizedBookmarkType(m_bookmarkData.m_featureTypes));
  m_uiSubtitle = strings::JoinStrings(subtitle, kSubtitleSeparator);
}

string Info::GetFormattedCoordinate(bool isDMS) const
{
  auto const & ll = GetLatLon();
  return isDMS ? measurement_utils::FormatLatLon(ll.lat, ll.lon, true)
               : measurement_utils::FormatLatLonAsDMS(ll.lat, ll.lon, 2);
}

float Info::GetRatingRawValue() const
{
  if (!IsSponsored() && !ShouldShowUGC())
    return kIncorrectRating;

  // Only sponsored rating is stored in metadata. UGC rating will be stored in special section and will be ready soon.
  auto const rating = GetMetadata().Get(feature::Metadata::FMD_RATING);
  float raw;
  if (!strings::to_float(rating, raw))
    return kIncorrectRating;

  return raw;
}

string Info::GetApproximatePricing() const
{
  if (!IsSponsored())
    return string();

  int pricing;
  if (!strings::to_int(GetMetadata().Get(feature::Metadata::FMD_PRICE_RATE), pricing))
    return string();

  string result;
  for (auto i = 0; i < pricing; i++)
    result.append(kPricingSymbol);

  return result;
}

boost::optional<int> Info::GetRawApproximatePricing() const
{
  if (!IsSponsored())
    return {};

  int pricing;
  if (strings::to_int(GetMetadata().Get(feature::Metadata::FMD_PRICE_RATE), pricing))
    return pricing;

  return {};
}

bool Info::HasBanner() const
{
  if (!m_adsEngine)
    return false;

  if (IsMyPosition())
    return false;

  return m_adsEngine->HasBanner(m_types, m_topmostCountryIds, languages::GetCurrentNorm());
}

vector<ads::Banner> Info::GetBanners() const
{
  if (!m_adsEngine)
    return {};

  return m_adsEngine->GetBanners(m_types, m_topmostCountryIds, languages::GetCurrentNorm());
}

void Info::SetPartnerIndex(int index)
{
  m_partnerIndex = index;
  m_partnerName = GetPartnerByIndex(m_partnerIndex).m_name;
}

namespace rating
{
namespace
{
std::string const kEmptyRatingSymbol = "-";
}  // namespace

FilterRating GetFilterRating(float const rawRating)
{
  CHECK_LESS_OR_EQUAL(rawRating, kTopRatingBound, ());
  CHECK_GREATER_OR_EQUAL(rawRating, 0, ());

  auto const rounded = static_cast<int>(rawRating);
  if (rounded < 7)
    return FilterRating::Any;
  if (rounded < 8)
    return FilterRating::Good;
  if (rounded < 9)
    return FilterRating::VeryGood;

  return FilterRating::Excellent;
}

Impress GetImpress(float const rawRating)
{
  CHECK_LESS_OR_EQUAL(rawRating, kTopRatingBound, ());
  CHECK_GREATER_OR_EQUAL(rawRating, kIncorrectRating, ());

  if (rawRating == kIncorrectRating)
    return Impress::None;
  if (rawRating < 0.2f * kTopRatingBound)
    return Impress::Horrible;
  if (rawRating < 0.4f * kTopRatingBound)
    return Impress::Bad;
  if (rawRating < 0.6f * kTopRatingBound)
    return Impress::Normal;
  if (rawRating < 0.8f * kTopRatingBound)
    return Impress::Good;

  return Impress::Excellent;
}

std::string GetRatingFormatted(float const rawRating)
{
  CHECK_LESS_OR_EQUAL(rawRating, kTopRatingBound, ());
  CHECK_GREATER_OR_EQUAL(rawRating, kIncorrectRating, ());

  if (rawRating == kIncorrectRating)
    return kEmptyRatingSymbol;

  std::ostringstream oss;
  oss << std::setprecision(2) << rawRating;
  return oss.str();
}
}  // namespace rating
}  // namespace place_page

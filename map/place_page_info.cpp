#include "map/place_page_info.hpp"
#include "map/reachable_by_taxi_checker.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"

#include "indexer/feature_utils.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include <sstream>

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

void Info::SetFromFeatureType(FeatureType const & ft)
{
  MapObject::SetFromFeatureType(ft);
  std::string primaryName;
  std::string secondaryName;
  GetPrefferedNames(primaryName, secondaryName);
  if (IsBookmark())
  {
    m_uiTitle = m_bookmarkData.GetName();

    std::string secondary;
    if (m_customName.empty())
      secondary = primaryName.empty() ? secondaryName : primaryName;
    else
      secondary = m_customName;

    if (m_uiTitle != secondary)
      m_uiSecondaryTitle = secondary;

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
  else if (!secondaryName.empty())
  {
    m_uiTitle = secondaryName;
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

void Info::SetCustomName(std::string const & name)
{
  if (IsBookmark())
  {
    m_uiTitle = GetBookmarkData().GetName();
    m_uiSubtitle = m_bookmarkCategoryName;
  }
  else
  {
    m_uiTitle = name;
  }

  m_customName = name;
}

void Info::SetCustomNameWithCoordinates(m2::PointD const & mercator, std::string const & name)
{
  if (IsBookmark())
  {
    m_uiTitle = GetBookmarkData().GetName();
    m_uiSubtitle = m_bookmarkCategoryName;
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

void Info::SetBac(BookmarkAndCategory const & bac)
{
  m_bac = bac;
  m_uiSubtitle = FormatSubtitle(IsFeature() /* withType */);
}

bool Info::ShouldShowEditPlace() const
{
  return m_canEditOrAdd &&
         // TODO(mgsergio): Does IsFeature() imply !IsMyPosition()?
         !IsMyPosition() && IsFeature();
}

ftraits::UGCRatingCategories Info::GetRatingCategories() const
{
  return ftraits::UGC::GetCategories(m_types);
}

string Info::FormatNewBookmarkName() const
{
  string const title = GetTitle();
  if (title.empty())
    return GetLocalizedType();
  return title;
}

string Info::FormatStars() const
{
  string stars;
  for (int i = 0; i < GetStars(); ++i)
    stars.append(kStarSymbol);
  return stars;
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

bool Info::HasBanner() const
{
  if (!m_adsEngine)
    return false;

  if (m_sponsoredType == SponsoredType::Cian)
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

namespace rating
{
namespace
{
std::string const kEmptyRatingSymbol = "-";
}  // namespace

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

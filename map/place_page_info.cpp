#include "map/place_page_info.hpp"
#include "map/reachable_by_taxi_checker.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"

#include "indexer/feature_utils.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

namespace place_page
{
char const * const Info::kSubtitleSeparator = " • ";
char const * const Info::kStarSymbol = "★";
char const * const Info::kMountainSymbol = "▲";
char const * const Info::kEmptyRatingSymbol = "-";
char const * const Info::kPricingSymbol = "$";

bool Info::IsFeature() const { return m_featureID.IsValid(); }
bool Info::IsBookmark() const { return m_bac.IsValid(); }
bool Info::IsMyPosition() const { return m_isMyPosition; }
bool Info::IsSponsored() const { return m_sponsoredType != SponsoredType::None; }
bool Info::IsNotEditableSponsored() const
{
  return m_sponsoredType != SponsoredType::None && m_sponsoredType != SponsoredType::Opentable;
}

bool Info::ShouldShowAddPlace() const
{
  auto const isPointOrBuilding = IsPointType() || IsBuilding();
  return m_canEditOrAdd && !(IsFeature() && isPointOrBuilding);
}

bool Info::ShouldShowAddBusiness() const { return m_canEditOrAdd && IsBuilding(); }

bool Info::ShouldShowEditPlace() const
{
  return m_canEditOrAdd &&
         // TODO(mgsergio): Does IsFeature() imply !IsMyPosition()?
         !IsMyPosition() && IsFeature();
}

bool Info::HasApiUrl() const { return !m_apiUrl.empty(); }
bool Info::HasWifi() const { return GetInternet() == osm::Internet::Wlan; }

string Info::FormatNewBookmarkName() const
{
  string const title = GetTitle();
  if (title.empty())
    return GetLocalizedType();
  return title;
}

string Info::GetTitle() const
{
  if (!m_customName.empty())
    return m_customName;

  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  string primaryName;
  if (mwmInfo)
  {
    auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
    string secondaryName;
    feature::GetPreferredNames(mwmInfo->GetRegionData(), m_name, deviceLang, true /* allowTranslit */, primaryName, secondaryName);
  }
  return primaryName;
}

string Info::GetSecondaryTitle() const
{
  auto const mwmInfo = GetID().m_mwmId.GetInfo();

  string secondaryName;
  if (mwmInfo)
  {
    auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
    string primaryName;
    feature::GetPreferredNames(mwmInfo->GetRegionData(), m_name, deviceLang, true /* allowTranslit */, primaryName, secondaryName);
  }
  return secondaryName;
}

string Info::GetSubtitle() const
{
  if (!IsFeature())
  {
    if (IsBookmark())
      return m_bookmarkCategoryName;
    return {};
  }

  vector<string> values;

  // Bookmark category.
  if (IsBookmark())
    values.push_back(m_bookmarkCategoryName);

  // Type.
  values.push_back(GetLocalizedType());

  // Flats.
  string const flats = GetFlats();
  if (!flats.empty())
    values.push_back(flats);

  // Cuisines.
  for (string const & cuisine : GetLocalizedCuisines())
    values.push_back(cuisine);

  // Stars.
  string const stars = FormatStars();
  if (!stars.empty())
    values.push_back(stars);

  // Operator.
  string const op = GetOperator();
  if (!op.empty())
    values.push_back(op);

  // Elevation.
  string const eleStr = GetElevationFormatted();
  if (!eleStr.empty())
    values.push_back(kMountainSymbol + eleStr);
  if (HasWifi())
    values.push_back(m_localizedWifiString);

  return strings::JoinStrings(values, kSubtitleSeparator);
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

string Info::GetCustomName() const { return m_customName; }
BookmarkAndCategory Info::GetBookmarkAndCategory() const { return m_bac; }
string Info::GetBookmarkCategoryName() const { return m_bookmarkCategoryName; }
string const & Info::GetApiUrl() const { return m_apiUrl; }
string const & Info::GetSponsoredUrl() const { return m_sponsoredUrl; }
string const & Info::GetSponsoredDescriptionUrl() const { return m_sponsoredDescriptionUrl; }
string const & Info::GetSponsoredReviewUrl() const { return m_sponsoredReviewUrl; }

string Info::GetRatingFormatted() const
{
  if (!IsSponsored())
    return string();

  auto const r = GetMetadata().Get(feature::Metadata::FMD_RATING);
  char const * rating = r.empty() ? kEmptyRatingSymbol : r.c_str();
  int const size = snprintf(nullptr, 0, m_localizedRatingString.c_str(), rating);
  if (size < 0)
  {
    LOG(LERROR, ("Incorrect size for string:", m_localizedRatingString, ", rating:", rating));
    return string();
  }

  vector<char> buf(size + 1);
  snprintf(buf.data(), buf.size(), m_localizedRatingString.c_str(), rating);
  return string(buf.begin(), buf.end());
}

string Info::GetApproximatePricing() const
{
  if (!IsSponsored())
    return string();

  int pricing;
  strings::to_int(GetMetadata().Get(feature::Metadata::FMD_PRICE_RATE), pricing);
  string result;
  for (auto i = 0; i < pricing; i++)
    result.append(kPricingSymbol);

  return result;
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

bool Info::IsReachableByTaxi() const
{
  return IsReachableByTaxiChecker::Instance()(m_types);
}

void Info::SetMercator(m2::PointD const & mercator) { m_mercator = mercator; }
vector<string> Info::GetRawTypes() const { return m_types.ToObjectNames(); }
string const & Info::GetBookingSearchUrl() const { return m_bookingSearchUrl; }
LocalAdsStatus Info::GetLocalAdsStatus() const { return m_localAdsStatus; }
string const & Info::GetLocalAdsUrl() const { return m_localAdsUrl; }
}  // namespace place_page

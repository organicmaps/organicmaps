#pragma once

#include "map/bookmark.hpp"

#include "partners_api/taxi_provider.hpp"

#include "ugc/api.hpp"

#include "map/routing_mark.hpp"

#include "storage/index.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/map_object.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "defines.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace ads
{
struct Banner;
class Engine;
}

namespace place_page
{
enum class SponsoredType
{
  None,
  Booking,
  Opentable,
  Viator,
  Partner,
  Holiday
};

enum class LocalAdsStatus
{
  NotAvailable,
  Candidate,
  Customer
};

enum class LocalsStatus
{
  NotAvailable,
  Available
};

auto constexpr kIncorrectRating = kInvalidRatingValue;

class Info : public osm::MapObject
{
public:
  static char const * const kSubtitleSeparator;
  static char const * const kStarSymbol;
  static char const * const kMountainSymbol;
  static char const * const kPricingSymbol;

  /// Place traits
  bool IsFeature() const { return m_featureID.IsValid(); }
  bool IsBookmark() const { return m_markGroupId != kml::kInvalidMarkGroupId && m_markId != kml::kInvalidMarkId; }
  bool IsMyPosition() const { return m_isMyPosition; }
  bool IsRoutePoint() const { return m_isRoutePoint; }

  /// Edit and add
  bool ShouldShowAddPlace() const;
  bool ShouldShowAddBusiness() const { return m_canEditOrAdd && IsBuilding(); }
  bool ShouldShowEditPlace() const;

  /// UGC
  bool ShouldShowUGC() const;
  bool CanBeRated() const { return ftraits::UGC::IsRatingAvailable(m_sortedTypes); }
  bool CanBeReviewed() const { return ftraits::UGC::IsReviewsAvailable(m_sortedTypes); }
  bool CanHaveExtendedReview() const { return ftraits::UGC::IsDetailsAvailable(m_sortedTypes); }
  ftraits::UGCRatingCategories GetRatingCategories() const;

  /// @returns true if Back API button should be displayed.
  bool HasApiUrl() const { return !m_apiUrl.empty(); }
  /// TODO: Support all possible Internet types in UI. @See MapObject::GetInternet().
  bool HasWifi() const { return GetInternet() == osm::Internet::Wlan; }
  /// Should be used by UI code to generate cool name for new bookmarks.
  // TODO: Tune new bookmark name. May be add address or some other data.
  kml::LocalizableString FormatNewBookmarkName() const;

  /// For showing in UI
  std::string const & GetTitle() const { return m_uiTitle; };
  /// Convenient wrapper for secondary feature name.
  std::string const & GetSecondaryTitle() const { return m_uiSecondaryTitle; };
  /// Convenient wrapper for type, cuisines, elevation, stars, wifi etc.
  std::string const & GetSubtitle() const { return m_uiSubtitle; };
  std::string const & GetAddress() const { return m_uiAddress; }
  std::string const & GetDescription() const { return m_description; }
  /// @returns coordinate in DMS format if isDMS is true
  std::string GetFormattedCoordinate(bool isDMS) const;
  /// @return rating raw value or kInvalidRating if there is no data.
  float GetRatingRawValue() const;
  /// @returns string with |kPricingSymbol| signs or empty std::string if it isn't booking object
  std::string GetApproximatePricing() const;
  boost::optional<int> GetRawApproximatePricing() const;

  /// UI setters
  void SetCustomName(std::string const & name);
  void SetCustomNameWithCoordinates(m2::PointD const & mercator, std::string const & name);
  void SetAddress(std::string const & address) { m_address = address; }
  void SetIsMyPosition() { m_isMyPosition = true; }
  void SetCanEditOrAdd(bool canEditOrAdd) { m_canEditOrAdd = canEditOrAdd; }
  void SetLocalizedWifiString(std::string const & str) { m_localizedWifiString = str; }

  /// Bookmark
  void SetBookmarkId(kml::MarkId markId);
  kml::MarkId GetBookmarkId() const { return m_markId; }
  void SetBookmarkCategoryId(kml::MarkGroupId markGroupId) { m_markGroupId = markGroupId; }
  kml::MarkGroupId GetBookmarkCategoryId() const { return m_markGroupId; }
  std::string const & GetBookmarkCategoryName() const { return m_bookmarkCategoryName; }
  void SetBookmarkCategoryName(std::string const & name) { m_bookmarkCategoryName = name; }
  void SetBookmarkData(kml::BookmarkData const & data) { m_bookmarkData = data; }
  kml::BookmarkData const & GetBookmarkData() const { return m_bookmarkData; }

  /// Api
  void SetApiId(std::string const & apiId) { m_apiId = apiId; }
  void SetApiUrl(std::string const & url) { m_apiUrl = url; }
  std::string const & GetApiUrl() const { return m_apiUrl; }

  /// Sponsored
  bool IsSponsored() const { return m_sponsoredType != SponsoredType::None; }
  bool IsNotEditableSponsored() const;
  void SetBookingSearchUrl(std::string const & url) { m_bookingSearchUrl = url; }
  std::string const & GetBookingSearchUrl() const { return m_bookingSearchUrl; }
  void SetSponsoredUrl(std::string const & url) { m_sponsoredUrl = url; }
  std::string const & GetSponsoredUrl() const { return m_sponsoredUrl; }
  void SetSponsoredDeepLink(std::string const & url) { m_sponsoredDeepLink = url; }
  std::string const & GetSponsoredDeepLink() const { return m_sponsoredDeepLink; }
  void SetSponsoredDescriptionUrl(std::string const & url) { m_sponsoredDescriptionUrl = url; }
  std::string const & GetSponsoredDescriptionUrl() const { return m_sponsoredDescriptionUrl; }
  void SetSponsoredReviewUrl(std::string const & url) { m_sponsoredReviewUrl = url; }
  std::string const & GetSponsoredReviewUrl() const { return m_sponsoredReviewUrl; }
  void SetSponsoredType(SponsoredType type) { m_sponsoredType = type; }
  SponsoredType GetSponsoredType() const { return m_sponsoredType; }
  bool IsPreviewExtended() const { return m_sponsoredType == SponsoredType::Viator; }

  /// Partners
  int GetPartnerIndex() const { return m_partnerIndex; }
  std::string const & GetPartnerName() const { return m_partnerName; }
  void SetPartnerIndex(int index);

  /// Feature status
  void SetFeatureStatus(FeatureStatus const status) { m_featureStatus = status; }

  /// Banner
  bool HasBanner() const;
  std::vector<ads::Banner> GetBanners() const;

  /// Taxi
  void SetReachableByTaxiProviders(std::vector<taxi::Provider::Type> const & providers)
  {
    m_reachableByProviders = providers;
  }

  std::vector<taxi::Provider::Type> const & ReachableByTaxiProviders() const
  {
    return m_reachableByProviders;
  }

  /// Local experts
  void SetLocalsStatus(LocalsStatus status) { m_localsStatus = status; }
  LocalsStatus GetLocalsStatus() const { return m_localsStatus; }
  void SetLocalsPageUrl(std::string const & url) { m_localsUrl = url; }
  std::string const & GetLocalsPageUrl() const { return m_localsUrl; }

  /// Local ads
  void SetLocalAdsStatus(LocalAdsStatus status) { m_localAdsStatus = status; }
  LocalAdsStatus GetLocalAdsStatus() const { return m_localAdsStatus; }
  void SetLocalAdsUrl(std::string const & url) { m_localAdsUrl = url; }
  std::string const & GetLocalAdsUrl() const { return m_localAdsUrl; }
  void SetAdsEngine(ads::Engine * const engine) { m_adsEngine = engine; }

  /// Routing
  void SetRouteMarkType(RouteMarkType type) { m_routeMarkType = type; }
  RouteMarkType GetRouteMarkType() const { return m_routeMarkType; }
  void SetIntermediateIndex(size_t index) { m_intermediateIndex = index; }
  size_t GetIntermediateIndex() const { return m_intermediateIndex; }
  void SetIsRoutePoint() { m_isRoutePoint = true; }

  /// CountryId
  /// Which mwm this MapObject is in.
  /// Exception: for a country-name point it will be set to the topmost node for the mwm.
  /// TODO(@a): use m_topmostCountryIds in exceptional case.
  void SetCountryId(storage::TCountryId const & countryId) { m_countryId = countryId; }
  storage::TCountryId const & GetCountryId() const { return m_countryId; }
  template <typename Countries>
  void SetTopmostCountryIds(Countries && ids)
  {
    m_topmostCountryIds = std::forward<Countries>(ids);
  }
  storage::TCountriesVec const & GetTopmostCountryIds() const { return m_topmostCountryIds; }

  /// MapObject
  void SetFromFeatureType(FeatureType & ft);

  void SetDescription(std::string && description) { m_description = std::move(description); }

  void SetMercator(m2::PointD const & mercator) { m_mercator = mercator; }
  std::vector<std::string> GetRawTypes() const { return m_types.ToObjectNames(); }

  boost::optional<ftypes::IsHotelChecker::Type> GetHotelType() const { return m_hotelType; }

  void SetPopularity(uint8_t popularity) { m_popularity = popularity; }
  uint8_t GetPopularity() const { return m_popularity; }
  std::string const & GetPrimaryFeatureName() const { return m_primaryFeatureName; };

private:
  std::string FormatSubtitle(bool withType) const;
  void GetPrefferedNames(std::string & primaryName, std::string & secondaryName) const;
  std::string GetBookmarkName();
  /// @returns empty string or GetStars() count of ★ symbol.
  std::string FormatStars() const;
  void SetTitlesForBookmark();

  /// UI
  std::string m_uiTitle;
  std::string m_uiSubtitle;
  std::string m_uiSecondaryTitle;
  std::string m_uiAddress;
  std::string m_description;
  // TODO(AlexZ): Temporary solution. It's better to use a wifi icon in UI instead of text.
  std::string m_localizedWifiString;
  /// Booking rating string
  std::string m_localizedRatingString;

  /// CountryId
  storage::TCountryId m_countryId = storage::kInvalidCountryId;
  /// The topmost downloader nodes this MapObject is in, i.e.
  /// the country name for an object whose mwm represents only
  /// one part of the country (or several countries for disputed territories).
  storage::TCountriesVec m_topmostCountryIds;

  /// Comes from API, shared links etc.
  std::string m_customName;

  /// Bookmarks
  /// If not invalid, bookmark is bound to this place page.
  kml::MarkId m_markId = kml::kInvalidMarkId;
  kml::MarkGroupId m_markGroupId = kml::kInvalidMarkGroupId;;
  /// Bookmark category name. Empty, if it's not bookmark;
  std::string m_bookmarkCategoryName;
  kml::BookmarkData m_bookmarkData;

  /// Api ID passed for the selected object. It's automatically included in api url below.
  std::string m_apiId;
  /// [Deep] link to open when "Back" button is pressed in a Place Page.
  std::string m_apiUrl;
  /// Formatted feature address for inner using.
  std::string m_address;

  /// Routing
  RouteMarkType m_routeMarkType;
  size_t m_intermediateIndex = 0;
  bool m_isRoutePoint = false;

  bool m_isMyPosition = false;

  /// Editor
  /// True if editing of a selected point is allowed by basic logic.
  /// See initialization in framework.
  bool m_canEditOrAdd = false;

  /// Ads
  std::vector<taxi::Provider::Type> m_reachableByProviders;
  std::string m_localAdsUrl;
  LocalAdsStatus m_localAdsStatus = LocalAdsStatus::NotAvailable;
  /// Ads source.
  ads::Engine * m_adsEngine = nullptr;
  /// Sponsored type or None.
  SponsoredType m_sponsoredType = SponsoredType::None;

  /// Feature status
  FeatureStatus m_featureStatus = FeatureStatus::Untouched;

  /// Sponsored feature urls.
  std::string m_sponsoredUrl;
  std::string m_sponsoredDeepLink;
  std::string m_sponsoredDescriptionUrl;
  std::string m_sponsoredReviewUrl;

  /// Booking
  std::string m_bookingSearchUrl;

  /// Local experts
  std::string m_localsUrl;
  LocalsStatus m_localsStatus = LocalsStatus::NotAvailable;

  /// Partners
  int m_partnerIndex = -1;
  std::string m_partnerName;

  feature::TypesHolder m_sortedTypes;

  boost::optional<ftypes::IsHotelChecker::Type> m_hotelType;

  uint8_t m_popularity = 0;

  std::string m_primaryFeatureName;
};

namespace rating
{
enum class FilterRating
{
  Any,
  Good,
  VeryGood,
  Excellent
};

enum Impress
{
  None,
  Horrible,
  Bad,
  Normal,
  Good,
  Excellent
};

FilterRating GetFilterRating(float const rawRating);

Impress GetImpress(float const rawRating);
std::string GetRatingFormatted(float const rawRating);
}  // namespace rating
}  // namespace place_page

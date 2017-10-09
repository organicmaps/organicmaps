#import "MWMPlacePageActionBar.h"

#include "partners_api/taxi_provider.hpp"

#include "storage/index.hpp"

#include "map/routing_mark.hpp"

#include <vector>

@class MWMPlacePageData;
@class MWMUGCReviewVM;
@class MWMCianItemModel;

struct BookmarkAndCategory;
struct FeatureID;

namespace ugc
{
struct Review;
}  // ugc

namespace local_ads
{
enum class EventType;
}  // local_ads

namespace booking
{
struct HotelReview;
struct HotelFacility;
}  // namespace booking

namespace place_page
{
class Info;

enum class Sections
{
  Preview,
  Bookmark,
  HotelPhotos,
  HotelDescription,
  HotelFacilities,
  HotelReviews,
  SpecialProjects,
  Metainfo,
  Ad,
  UGC,
  Buttons
};

enum class PreviewRows
{
  Title,
  ExternalTitle,
  Subtitle,
  Schedule,
  Booking,
  Address,
  Space,
  Banner
};

enum class HotelDescriptionRow
{
  Regular,
  ShowMore
};

enum class HotelPhotosRow
{
  Regular
};

enum class HotelFacilitiesRow
{
  Regular,
  ShowMore
};

enum class HotelReviewsRow
{
  Header,
  Regular,
  ShowMore
};

enum class SpecialProject
{
  Viator,
  Cian
};

enum class MetainfoRows
{
  OpeningHours,
  ExtendedOpeningHours,
  Phone,
  Address,
  Website,
  Email,
  Cuisine,
  Operator,
  Internet,
  Coordinate,
  LocalAdsCandidate,
  LocalAdsCustomer
};

enum class AdRows
{
  Taxi
};

enum class UGCRow
{
  SelectImpression,
  Comment,
  ShowMore
};

enum class ButtonsRows
{
  AddBusiness,
  EditPlace,
  AddPlace,
  HotelDescription,
  Other
};

enum class OpeningHours
{
  AllDay,
  Open,
  Closed,
  Unknown
};

using NewSectionsAreReady = void (^)(NSRange const & range, MWMPlacePageData * data, BOOL isSection);
using CianIsReady = void (^)(NSArray<MWMCianItemModel *> * items);

}  // namespace place_page

@class MWMGalleryItemModel;
@class MWMViatorItemModel;
@class MWMCianItemModel;
@protocol MWMBanner;

/// ViewModel for place page.
@interface MWMPlacePageData : NSObject<MWMActionBarSharedData>

@property(copy, nonatomic) place_page::NewSectionsAreReady sectionsAreReadyCallback;
@property(copy, nonatomic) MWMVoidBlock bannerIsReadyCallback;
@property(copy, nonatomic) place_page::CianIsReady cianIsReadyCallback;

// ready callback will be called from main queue.
- (instancetype)initWithPlacePageInfo:(place_page::Info const &)info;

- (void)fillSections;

- (void)updateBookmarkStatus:(BOOL)isBookmark;

/// Country id for changing place page's fields which depend on application state.
- (storage::TCountryId const &)countryId;
- (FeatureID const &)featureId;

// Regular
- (NSString *)title;
- (NSString *)subtitle;
- (place_page::OpeningHours)schedule;
- (NSString *)address;

// Booking
- (void)fillOnlineBookingSections;
- (NSString *)bookingRating;
- (NSString *)bookingApproximatePricing;
- (NSURL *)sponsoredURL;
- (NSURL *)sponsoredDescriptionURL;
- (NSURL *)bookingSearchURL;
- (NSString *)sponsoredId;
- (void)assignOnlinePriceToLabel:(UILabel *)label;
- (NSString *)hotelDescription;
- (vector<booking::HotelFacility> const &)facilities;
- (vector<booking::HotelReview> const &)hotelReviews;
- (NSUInteger)numberOfHotelReviews;
- (NSURL *)URLToAllReviews;
- (NSArray<MWMGalleryItemModel *> *)photos;

// Viator
- (void)fillOnlineViatorSection;
- (NSArray<MWMViatorItemModel *> *)viatorItems;

// CIAN
- (void)fillOnlineCianSection;

// UGC
- (MWMUGCReviewVM *)reviewViewModel;
- (std::vector<ugc::Review> const &)ugcReviews;

// Route points
- (RouteMarkType)routeMarkType;
- (size_t)intermediateIndex;

// Taxi
- (std::vector<taxi::Provider::Type> const &)taxiProviders;

// Banner
- (id<MWMBanner>)nativeAd;

// API
- (NSString *)apiURL;

// Bookmark
- (NSString *)externalTitle;
- (NSString *)bookmarkColor;
- (NSString *)bookmarkDescription;
- (NSString *)bookmarkCategory;
- (BookmarkAndCategory)bookmarkAndCategory;

// Local Ads
- (NSString *)localAdsURL;
- (void)logLocalAdsEvent:(local_ads::EventType)type;

// Table view's data
- (std::vector<place_page::Sections> const &)sections;
- (std::vector<place_page::PreviewRows> const &)previewRows;
- (std::vector<place_page::HotelPhotosRow> const &)photosRows;
- (std::vector<place_page::HotelDescriptionRow> const &)descriptionRows;
- (std::vector<place_page::HotelFacilitiesRow> const &)hotelFacilitiesRows;
- (std::vector<place_page::HotelReviewsRow> const &)hotelReviewsRows;
- (std::vector<place_page::MetainfoRows> const &)metainfoRows;
- (std::vector<place_page::SpecialProject> const &)specialProjectRows;
- (std::vector<place_page::AdRows> const &)adRows;
- (std::vector<place_page::UGCRow> const &)ugcRows;
- (std::vector<place_page::ButtonsRows> const &)buttonsRows;

// Table view metainfo rows
- (NSString *)stringForRow:(place_page::MetainfoRows)row;

// Helpers
- (NSString *)phoneNumber;
- (BOOL)isBookmark;
- (BOOL)isApi;
- (BOOL)isBooking;
- (BOOL)isOpentable;
- (BOOL)isViator;
- (BOOL)isCian;
- (BOOL)isThor;
- (BOOL)isBookingSearch;
- (BOOL)isHTMLDescription;
- (BOOL)isMyPosition;
- (BOOL)isRoutePoint;

// Coordinates
- (m2::PointD const &)mercator;
- (ms::LatLon)latLon;

- (NSString *)statisticsTags;

// TODO(Vlad): Use MWMSettings to store coordinate format.
+ (void)toggleCoordinateSystem;

@end

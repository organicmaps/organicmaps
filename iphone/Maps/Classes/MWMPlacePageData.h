#include "map/place_page_info.hpp"

#include "std/vector.hpp"

namespace place_page
{
enum class Sections
{
  Preview,
  Bookmark,
  Metainfo,
  Buttons
};

enum class MetainfoRows
{
  OpeningHours,
  Phone,
  Address,
  Website,
  Email,
  Cuisine,
  Operator,
  Internet,
  Coordinate
};

enum class ButtonsRows
{
  AddBusiness,
  EditPlace,
  AddPlace,
  HotelDescription
};

enum class OpeningHours
{
  AllDay,
  Open,
  Closed,
  Unknown
};

}  // namespace place_page_data

/// ViewModel for place page.
@interface MWMPlacePageData : NSObject

// ready callback will be called from main queue.
- (instancetype)initWithPlacePageInfo:(place_page::Info const &)info;

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
- (NSString *)bookingRating;
- (NSString *)bookingApproximatePricing;
- (NSURL *)sponsoredURL;
- (NSURL *)sponsoredDescriptionURL;
- (NSString *)sponsoredId;
- (void)assignOnlinePriceToLabel:(UILabel *)label;

// API
- (NSString *)apiURL;

// Bookmark
- (NSString *)externalTitle;
- (NSString *)bookmarkColor;
- (NSString *)bookmarkDescription;
- (NSString *)bookmarkCategory;
- (BookmarkAndCategory)bac;

// Table view's data
- (vector<place_page::Sections> const &)sections;
- (vector<place_page::MetainfoRows> const &)metainfoRows;
- (vector<place_page::ButtonsRows> const &)buttonsRows;

// Table view metainfo rows
- (NSString *)stringForRow:(place_page::MetainfoRows)row;

// Helpers
- (NSString *)phoneNumber;
- (BOOL)isBookmark;
- (BOOL)isApi;
- (BOOL)isBooking;
- (BOOL)isOpentable;
- (BOOL)isHTMLDescription;
- (BOOL)isMyPosition;

// Coordinates
- (m2::PointD const &)mercator;
- (ms::LatLon)latLon;

// TODO(Vlad): Use MWMSettings to store coordinate format.
+ (void)toggleCoordinateSystem;

@end

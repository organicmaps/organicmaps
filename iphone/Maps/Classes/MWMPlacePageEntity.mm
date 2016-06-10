#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"

#include "Framework.h"

#include "indexer/osm_editor.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

using feature::Metadata;

extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";

namespace
{

NSUInteger gMetaFieldsMap[MWMPlacePageCellTypeCount] = {};

void putFields(NSUInteger eTypeValue, NSUInteger ppValue)
{
  gMetaFieldsMap[eTypeValue] = ppValue;
  gMetaFieldsMap[ppValue] = eTypeValue;
}

void initFieldsMap()
{
  putFields(Metadata::FMD_URL, MWMPlacePageCellTypeURL);
  putFields(Metadata::FMD_WEBSITE, MWMPlacePageCellTypeWebsite);
  putFields(Metadata::FMD_PHONE_NUMBER, MWMPlacePageCellTypePhoneNumber);
  putFields(Metadata::FMD_OPEN_HOURS, MWMPlacePageCellTypeOpenHours);
  putFields(Metadata::FMD_EMAIL, MWMPlacePageCellTypeEmail);
  putFields(Metadata::FMD_POSTCODE, MWMPlacePageCellTypePostcode);
  putFields(Metadata::FMD_INTERNET, MWMPlacePageCellTypeWiFi);
  putFields(Metadata::FMD_CUISINE, MWMPlacePageCellTypeCuisine);

  ASSERT_EQUAL(gMetaFieldsMap[Metadata::FMD_URL], MWMPlacePageCellTypeURL, ());
  ASSERT_EQUAL(gMetaFieldsMap[MWMPlacePageCellTypeURL], Metadata::FMD_URL, ());
  ASSERT_EQUAL(gMetaFieldsMap[Metadata::FMD_WEBSITE], MWMPlacePageCellTypeWebsite, ());
  ASSERT_EQUAL(gMetaFieldsMap[MWMPlacePageCellTypeWebsite], Metadata::FMD_WEBSITE, ());
  ASSERT_EQUAL(gMetaFieldsMap[Metadata::FMD_POSTCODE], MWMPlacePageCellTypePostcode, ());
  ASSERT_EQUAL(gMetaFieldsMap[MWMPlacePageCellTypePostcode], Metadata::FMD_POSTCODE, ());
  ASSERT_EQUAL(gMetaFieldsMap[Metadata::FMD_MAXSPEED], 0, ());
}
} // namespace

@implementation MWMPlacePageEntity
{
  MWMPlacePageCellTypeValueMap m_values;
  place_page::Info m_info;
}

- (instancetype)initWithInfo:(const place_page::Info &)info
{
  self = [super init];
  if (self)
  {
    m_info = info;
    initFieldsMap();
    [self config];
  }
  return self;
}

- (void)config
{
  [self configureDefault];

  if (m_info.IsFeature())
    [self configureFeature];
  if (m_info.IsBookmark())
    [self configureBookmark];
}

- (void)setMetaField:(NSUInteger)key value:(string const &)value
{
  NSAssert(key >= Metadata::FMD_COUNT, @"Incorrect enum value");
  MWMPlacePageCellType const cellType = static_cast<MWMPlacePageCellType>(key);
  if (value.empty())
    m_values.erase(cellType);
  else
    m_values[cellType] = value;
}

- (void)configureDefault
{
  self.title = @(m_info.GetTitle().c_str());
  self.address = @(m_info.GetAddress().c_str());
  self.subtitle = @(m_info.GetSubtitle().c_str());
  self.bookingRating = @(m_info.GetRatingFormatted().c_str());
  self.bookingPrice = @(m_info.GetApproximatePricing().c_str());
}

- (void)configureFeature
{
  // Category can also be custom-formatted, please check m_info getters.
  // TODO(Vlad): Refactor using osm::Props instead of direct Metadata access.
  feature::Metadata const & md = m_info.GetMetadata();
  auto const types = md.GetPresentTypes();
  for (auto const type : types)
  {
    switch (type)
    {
      case Metadata::FMD_URL:
      case Metadata::FMD_WEBSITE:
      case Metadata::FMD_PHONE_NUMBER:
      case Metadata::FMD_OPEN_HOURS:
      case Metadata::FMD_EMAIL:
      case Metadata::FMD_POSTCODE:
        [self setMetaField:gMetaFieldsMap[type] value:md.Get(type)];
        break;
      case Metadata::FMD_INTERNET:
        [self setMetaField:gMetaFieldsMap[type] value:L(@"WiFi_available").UTF8String];
        break;
      default:
        break;
    }
  }
}

- (void)onlinePricingWithCompletionBlock:(TMWMVoidBlock)completion failure:(TMWMVoidBlock)failure
{
  if (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE || !self.isBooking)
  {
    failure();
    return;
  }

  NSNumberFormatter * currencyFormatter = [[NSNumberFormatter alloc] init];
  [currencyFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
  [currencyFormatter setLocale:[NSLocale currentLocale]];
  string const currency = currencyFormatter.currencyCode.UTF8String;
  GetFramework().GetBookingApi().GetMinPrice(m_info.GetMetadata().Get(Metadata::FMD_SPONSORED_ID),
                                             currency,
                                             [self, completion, failure, currency, currencyFormatter](string const & minPrice, string const & priceCurrency)
  {
    if (currency != priceCurrency)
    {
      failure();
      return;
    }
    NSNumberFormatter * decimalFormatter = [[NSNumberFormatter alloc] init];
    decimalFormatter.numberStyle = NSNumberFormatterDecimalStyle;
    // TODO(Vlad): We will replace this string with [NSString stringWithFormat:L(@"place_page_starting_from"), currency]
    // as soon as string is ready.
    self.bookingOnlinePrice = [currencyFormatter stringFromNumber:[decimalFormatter numberFromString:@(minPrice.c_str())]];
    completion();
  });
}

- (void)configureBookmark
{
  auto const bac = m_info.GetBookmarkAndCategory();
  BookmarkCategory * cat = GetFramework().GetBmCategory(bac.first);
  BookmarkData const & data = static_cast<Bookmark const *>(cat->GetUserMark(bac.second))->GetData();

  self.bookmarkTitle = @(data.GetName().c_str());
  self.bookmarkCategory = @(m_info.GetBookmarkCategoryName().c_str());
  string const & description = data.GetDescription();
  self.bookmarkDescription = @(description.c_str());
  _isHTMLDescription = strings::IsHTML(description);
  self.bookmarkColor = @(data.GetType().c_str());
}

- (void)toggleCoordinateSystem
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:![ud boolForKey:kUserDefaultsLatLonAsDMSKey] forKey:kUserDefaultsLatLonAsDMSKey];
  [ud synchronize];
}

#pragma mark - Getters

- (NSString *)getCellValue:(MWMPlacePageCellType)cellType
{
  auto const s = MapsAppDelegate.theApp.mapViewController.controlsManager.navigationState;
  BOOL const navigationIsHidden = s == MWMNavigationDashboardStateHidden;
  switch (cellType)
  {
    case MWMPlacePageCellTypeName:
      return self.title;
    case MWMPlacePageCellTypeCoordinate:
      return [self coordinate];
    case MWMPlacePageCellTypeAddPlaceButton:
      return navigationIsHidden && m_info.ShouldShowAddPlace() ? @"" : nil;
    case MWMPlacePageCellTypeBookmark:
      return m_info.IsBookmark() ? @"" : nil;
    case MWMPlacePageCellTypeEditButton:
      // TODO(Vlad): It's a really strange way to "display" cell if returned text is not nil.
      return navigationIsHidden && m_info.ShouldShowEditPlace() ? @"" : nil;
    case MWMPlacePageCellTypeAddBusinessButton:
      return navigationIsHidden && m_info.ShouldShowAddBusiness() ? @"" : nil;
    case MWMPlacePageCellTypeWebsite:
      return m_info.IsSponsoredHotel() ? nil : [self getDefaultField:cellType];
    case MWMPlacePageCellTypeBookingMore:
      return m_info.IsSponsoredHotel() ? @(m_info.GetSponsoredDescriptionUrl().c_str()) : nil;
    default:
      return [self getDefaultField:cellType];
  }
}

- (NSString *)getDefaultField:(MWMPlacePageCellType)cellType
{
  auto const it = m_values.find(cellType);
  BOOL const haveField = (it != m_values.end());
  return haveField ? @(it->second.c_str()) : nil;
}

- (NSURL *)bookingUrl
{
  auto const & url = m_info.GetSponsoredBookingUrl();
  return url.empty() ? nil : [NSURL URLWithString:@(url.c_str())];
}

- (place_page::Info const &)info
{
  return m_info;
}

- (FeatureID const &)featureID
{
  return m_info.GetID();
}

- (storage::TCountryId const &)countryId
{
   return m_info.m_countryId;
}

- (BOOL)isMyPosition
{
  return m_info.IsMyPosition();
}

- (BOOL)isBookmark
{
  return m_info.IsBookmark();
}

- (BOOL)isApi
{
  return m_info.HasApiUrl();
}

- (BOOL)isBooking
{
  return m_info.IsSponsoredHotel();
}

- (ms::LatLon)latlon
{
  return m_info.GetLatLon();
}

- (m2::PointD const &)mercator
{
  return m_info.GetMercator();
}

- (NSString *)apiURL
{
  return @(m_info.GetApiUrl().c_str());
}

- (string)titleForNewBookmark
{
  return m_info.FormatNewBookmarkName();
}

- (NSString *)coordinate
{
  BOOL const useDMSFormat =
      [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  ms::LatLon const latlon = self.latlon;
  return @((useDMSFormat ? MeasurementUtils::FormatLatLon(latlon.lat, latlon.lon)
                         : MeasurementUtils::FormatLatLonAsDMS(latlon.lat, latlon.lon, 2)).c_str());
}

#pragma mark - Bookmark editing

- (void)setBac:(BookmarkAndCategory)bac
{
  m_info.m_bac = bac;
}

- (BookmarkAndCategory)bac
{
  return m_info.GetBookmarkAndCategory();
}

- (NSString *)bookmarkCategory
{
  if (!_bookmarkCategory)
  {
    Framework & f = GetFramework();
    BookmarkCategory * category = f.GetBmCategory(f.LastEditedBMCategory());
    _bookmarkCategory = @(category->GetName().c_str());
  }
  return _bookmarkCategory;
}

- (NSString *)bookmarkDescription
{
  if (!_bookmarkDescription)
    _bookmarkDescription = @"";
  return _bookmarkDescription;
}

- (NSString *)bookmarkColor
{
  if (!_bookmarkColor)
  {
    Framework & f = GetFramework();
    string type = f.LastEditedBMType();
    _bookmarkColor = @(type.c_str());
  }
  return _bookmarkColor;
}

- (NSString *)bookmarkTitle
{
  if (!_bookmarkTitle)
    _bookmarkTitle = self.title;
  return _bookmarkTitle;
}

- (void)synchronize
{
  Framework & f = GetFramework();
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  if (!category)
    return;

  {
    BookmarkCategory::Guard guard(*category);
    Bookmark * bookmark = static_cast<Bookmark *>(guard.m_controller.GetUserMarkForEdit(self.bac.second));
    if (!bookmark)
      return;

    if (self.bookmarkColor)
      bookmark->SetType(self.bookmarkColor.UTF8String);

    if (self.bookmarkDescription)
    {
      string const description(self.bookmarkDescription.UTF8String);
      _isHTMLDescription = strings::IsHTML(description);
      bookmark->SetDescription(description);
    }

    if (self.bookmarkTitle)
      bookmark->SetName(self.bookmarkTitle.UTF8String);
  }

  category->SaveToKMLFile();
}

@end

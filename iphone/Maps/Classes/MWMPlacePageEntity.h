#include "Framework.h"

#include "indexer/feature_meta.hpp"

#include "storage/index.hpp"

typedef NS_ENUM(NSUInteger, MWMPlacePageCellType)
{
  MWMPlacePageCellTypePostcode = feature::Metadata::EType::FMD_COUNT,
  MWMPlacePageCellTypePhoneNumber,
  MWMPlacePageCellTypeWebsite,
  MWMPlacePageCellTypeURL,
  MWMPlacePageCellTypeEmail,
  MWMPlacePageCellTypeOperator,
  MWMPlacePageCellTypeOpenHours,
  MWMPlacePageCellTypeWiFi,
  MWMPlacePageCellTypeCoordinate,
  MWMPlacePageCellTypeBookmark,
  MWMPlacePageCellTypeEditButton,
  MWMPlacePageCellTypeAddBusinessButton,
  MWMPlacePageCellTypeAddPlaceButton,
  MWMPlacePageCellTypeReportButton,
  MWMPlacePageCellTypeCategory,
  MWMPlacePageCellTypeName,
  MWMPlacePageCellTypeAdditionalName,
  MWMPlacePageCellTypeAddAdditionalName,
  MWMPlacePageCellTypeAddAdditionalNamePlaceholder,
  MWMPlacePageCellTypeStreet,
  MWMPlacePageCellTypeBuilding,
  MWMPlacePageCellTypeZipCode,
  MWMPlacePageCellTypeBuildingLevels,
  MWMPlacePageCellTypeCuisine,
  MWMPlacePageCellTypeNote,
  MWMPlacePageCellTypeBookingMore,
  MWMPlacePageCellTypeCount
};

using MWMPlacePageCellTypeValueMap = map<MWMPlacePageCellType, string>;

@class MWMPlacePageViewManager;

@interface MWMPlacePageEntity : NSObject

@property (copy, nonatomic) NSString * title;
@property (copy, nonatomic) NSString * subtitle;
@property (copy, nonatomic) NSString * address;
@property (copy, nonatomic) NSString * bookmarkTitle;
@property (copy, nonatomic) NSString * bookmarkCategory;
@property (copy, nonatomic) NSString * bookmarkDescription;
@property (nonatomic, readonly) BOOL isHTMLDescription;
@property (copy, nonatomic) NSString * bookmarkColor;
@property (copy, nonatomic) NSString * bookingRating;
@property (copy, nonatomic) NSString * bookingPrice;
@property (copy, nonatomic) NSString * bookingOnlinePrice;

@property (nonatomic) BookmarkAndCategory bac;
@property (weak, nonatomic) MWMPlacePageViewManager * manager;

- (FeatureID const &)featureID;
- (BOOL)isMyPosition;
- (BOOL)isBookmark;
- (BOOL)isApi;
- (BOOL)isBooking;
- (ms::LatLon)latlon;
- (m2::PointD const &)mercator;
- (NSString *)apiURL;
- (NSURL *)bookingUrl;
- (string)titleForNewBookmark;

- (instancetype)initWithInfo:(place_page::Info const &)info;
- (void)synchronize;
- (void)onlinePricingWithCompletionBlock:(TMWMVoidBlock)completion failure:(TMWMVoidBlock)failure;

- (void)toggleCoordinateSystem;

- (NSString *)getCellValue:(MWMPlacePageCellType)cellType;
- (place_page::Info const &)info;
- (storage::TCountryId const &)countryId;

@end

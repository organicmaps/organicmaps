#include "Framework.h"

#include "indexer/feature_meta.hpp"

typedef NS_ENUM(NSUInteger, MWMPlacePageCellType)
{
  MWMPlacePageCellTypePostcode = feature::Metadata::EType::FMD_COUNT,
  MWMPlacePageCellTypePhoneNumber,
  MWMPlacePageCellTypeWebsite,
  MWMPlacePageCellTypeURL,
  MWMPlacePageCellTypeEmail,
  MWMPlacePageCellTypeOpenHours,
  MWMPlacePageCellTypeWiFi,
  MWMPlacePageCellTypeCoordinate,
  MWMPlacePageCellTypeBookmark,
  MWMPlacePageCellTypeEditButton,
  MWMPlacePageCellTypeAddBusinessButton,
  MWMPlacePageCellTypeReportButton,
  MWMPlacePageCellTypeCategory,
  MWMPlacePageCellTypeName,
  MWMPlacePageCellTypeStreet,
  MWMPlacePageCellTypeBuilding,
  MWMPlacePageCellTypeCuisine,
  MWMPlacePageCellTypeCount
};

using MWMPlacePageCellTypeValueMap = map<MWMPlacePageCellType, string>;

@class MWMPlacePageViewManager;

@interface MWMPlacePageEntity : NSObject

@property (copy, nonatomic) NSString * title;
@property (copy, nonatomic) NSString * category;
@property (copy, nonatomic) NSString * address;
@property (copy, nonatomic) NSString * bookmarkTitle;
@property (copy, nonatomic) NSString * bookmarkCategory;
@property (copy, nonatomic) NSString * bookmarkDescription;
@property (nonatomic, readonly) BOOL isHTMLDescription;
@property (copy, nonatomic) NSString * bookmarkColor;

@property (nonatomic) BookmarkAndCategory bac;
@property (weak, nonatomic) MWMPlacePageViewManager * manager;

- (FeatureID const &)featureID;
- (BOOL)isMyPosition;
- (BOOL)isBookmark;
- (BOOL)isApi;
- (ms::LatLon)latlon;
- (m2::PointD const &)mercator;
- (NSString *)apiURL;
- (string)titleForNewBookmark;

- (instancetype)initWithInfo:(place_page::Info const &)info;
- (void)synchronize;

- (void)toggleCoordinateSystem;

- (NSString *)getCellValue:(MWMPlacePageCellType)cellType;
- (place_page::Info const &)info;

@end

#include "Framework.h"

#include "map/user_mark.hpp"
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
  MWMPlacePageCellTypeName,
  MWMPlacePageCellTypeStreet,
  MWMPlacePageCellTypeBuilding,
  MWMPlacePageCellTypeCuisine,
  MWMPlacePageCellTypeSpacer,
  MWMPlacePageCellTypeCount
};

typedef NS_ENUM(NSUInteger, MWMPlacePageEntityType)
{
  MWMPlacePageEntityTypeRegular,
  MWMPlacePageEntityTypeBookmark,
  MWMPlacePageEntityTypeEle,
  MWMPlacePageEntityTypeHotel,
  MWMPlacePageEntityTypeAPI,
  MWMPlacePageEntityTypeMyPosition
};

using MWMPlacePageCellTypeValueMap = map<MWMPlacePageCellType, string>;

@class MWMPlacePageViewManager;

@protocol MWMPlacePageEntityProtocol <NSObject>

- (UserMark const *)userMark;

@end

@interface MWMPlacePageEntity : NSObject

@property (copy, nonatomic) NSString * title;
@property (copy, nonatomic) NSString * category;
@property (copy, nonatomic) NSString * bookmarkTitle;
@property (copy, nonatomic) NSString * bookmarkCategory;
@property (copy, nonatomic) NSString * bookmarkDescription;
@property (nonatomic, readonly) BOOL isHTMLDescription;
@property (copy, nonatomic) NSString * bookmarkColor;
@property (copy, nonatomic) NSSet<NSString *> * cuisines;

@property (nonatomic) MWMPlacePageEntityType type;

@property (nonatomic) int typeDescriptionValue;

@property (nonatomic) BookmarkAndCategory bac;
@property (weak, nonatomic) MWMPlacePageViewManager * manager;

@property (nonatomic, readonly) ms::LatLon latlon;

- (void)addBookmarkField;
- (void)removeBookmarkField;

- (instancetype)initWithDelegate:(id<MWMPlacePageEntityProtocol>)delegate;
- (void)synchronize;

- (void)toggleCoordinateSystem;

- (NSUInteger)getCellsCount;
- (MWMPlacePageCellType)getCellType:(NSUInteger)index;
- (NSString *)getCellValue:(MWMPlacePageCellType)cellType;
- (BOOL)isCellEditable:(MWMPlacePageCellType)cellType;
- (void)saveEditedCells:(MWMPlacePageCellTypeValueMap const &)cells;

@end

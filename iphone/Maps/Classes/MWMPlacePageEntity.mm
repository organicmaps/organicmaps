#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"
#include "platform/measurement_utils.hpp"

extern NSArray * const kBookmarkColorsVariant = @[
  @"placemark-red",
  @"placemark-yellow",
  @"placemark-blue",
  @"placemark-green",
  @"placemark-purple",
  @"placemark-orange",
  @"placemark-brown",
  @"placemark-pink"
];
extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";
static NSArray * const kPatternTypesArray = @[
  @(MWMPlacePageMetadataTypePostcode),
  @(MWMPlacePageMetadataTypePhoneNumber),
  @(MWMPlacePageMetadataTypeWebsite),
  @(MWMPlacePageMetadataTypeURL),
  @(MWMPlacePageMetadataTypeEmail),
  @(MWMPlacePageMetadataTypeOpenHours),
  @(MWMPlacePageMetadataTypeWiFi),
  @(MWMPlacePageMetadataTypeCoordinate)
];

using feature::Metadata;

@interface MWMPlacePageEntity ()

@property (nonatomic) NSMutableArray * metaTypes;
@property (nonatomic) NSMutableArray * metaValues;

@end

@implementation MWMPlacePageEntity

- (instancetype)initWithUserMark:(UserMark const *)mark
{
  self = [super init];
  if (self)
  {
    self.metaTypes = [NSMutableArray array];
    self.metaValues = [NSMutableArray array];
    [self configureWithUserMark:mark];
  }

  return self;
}

- (void)configureWithUserMark:(UserMark const *)mark
{
  UserMark::Type const type = mark->GetMarkType();
  self.ll = mark->GetLatLon();

  typedef UserMark::Type Type;
  switch (type)
  {
    case Type::API:
    {
      ApiMarkPoint const * apiMark = static_cast<ApiMarkPoint const *>(mark);
      [self configureForApi:apiMark];
      break;
    }
    case Type::DEBUG_MARK:
      break;
    case Type::MY_POSITION:
    {
      MyPositionMarkPoint const * myPositionMark = static_cast<MyPositionMarkPoint const *>(mark);
      [self configureForMyPosition:myPositionMark];
      break;
    }
    case Type::SEARCH:
      [self configureForSearch:static_cast<SearchMarkPoint const *>(mark)];
      break;
    case Type::POI:
      [self configureForPOI:static_cast<PoiMarkPoint const *>(mark)];
      break;
    case Type::BOOKMARK:
      [self configureForBookmark:mark];
      break;
  }
}

- (void)configureForBookmark:(UserMark const *)bookmark
{
  Framework & f = GetFramework();
  self.bac = f.FindBookmark(bookmark);
  self.type = MWMPlacePageEntityTypeBookmark;
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  BookmarkData const & data = static_cast<Bookmark const *>(bookmark)->GetData();
  m2::PointD const & point = bookmark->GetPivot();
  Metadata metadata;
  search::AddressInfo info;
  f.FindClosestPOIMetadata(point, metadata);
  f.GetAddressInfoForGlobalPoint(point, info);

  self.bookmarkTitle = @(data.GetName().c_str());
  self.bookmarkCategory = @(category->GetName().c_str());
  string const description = data.GetDescription();
  self.bookmarkDescription = @(description.c_str());
  _isHTMLDescription = strings::IsHTML(description);
  self.bookmarkColor = @(data.GetType().c_str());

  [self configureEntityWithMetadata:metadata addressInfo:info];
  [self insertBookmarkInTypes];
}

- (void)configureForSearch:(SearchMarkPoint const *)searchMark
{
//Workaround for framework bug.
//TODO: Make correct way to get search metadata.
  Metadata metadata;
  GetFramework().FindClosestPOIMetadata(searchMark->GetPivot(), metadata);
  [self configureEntityWithMetadata:metadata addressInfo:searchMark->GetInfo()];
}

- (void)configureForPOI:(PoiMarkPoint const *)poiMark
{
  [self configureEntityWithMetadata:poiMark->GetMetadata() addressInfo:poiMark->GetInfo()];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  [self.metaTypes addObject:kPatternTypesArray.lastObject];
  [self.metaValues addObject:[self coordinates]];
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{
  self.type = MWMPlacePageEntityTypeAPI;
  self.title = @(apiMark->GetName().c_str());
  self.category = @(GetFramework().GetApiDataHolder().GetAppTitle().c_str());
  [self.metaTypes addObject:kPatternTypesArray.lastObject];
  [self.metaValues addObject:[self coordinates]];
}

- (void)configureEntityWithMetadata:(Metadata const &)metadata addressInfo:(search::AddressInfo const &)info
{
  NSString * const name = @(info.GetPinName().c_str());
  self.title = name.length > 0 ? name : L(@"dropped_pin");
  self.category = @(info.FormatAddress().c_str());

  auto const presentTypes = metadata.GetPresentTypes();

  for (auto const & type : presentTypes)
  {
    switch (type)
    {
      case Metadata::FMD_CUISINE:
      {
        NSString * result = @(metadata.Get(type).c_str());
        NSString * cuisine = [NSString stringWithFormat:@"cuisine_%@", result];
        NSString * localizedResult = L(cuisine);
        NSString * currentCategory = self.category;
        if (![localizedResult isEqualToString:currentCategory])
        {
          if ([localizedResult isEqualToString:cuisine])
          {
            if (![result isEqualToString:currentCategory])
              self.category = [NSString stringWithFormat:@"%@, %@", self.category, result];
          }
          else
          {
            self.category = [NSString stringWithFormat:@"%@, %@", self.category, localizedResult];
          }
        }
        break;
      }
      case Metadata::FMD_ELE:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeEle;
        break;
      }
      case Metadata::FMD_OPERATOR:
      {
        NSString const * bank = @(metadata.Get(type).c_str());
        if (self.category.length)
          self.category = [NSString stringWithFormat:@"%@, %@", self.category, bank];
        else
          self.category = [NSString stringWithFormat:@"%@", bank];
        break;
      }
      case Metadata::FMD_STARS:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeHotel;
        break;
      }
      case Metadata::FMD_URL:
      case Metadata::FMD_WEBSITE:
      case Metadata::FMD_PHONE_NUMBER:
      case Metadata::FMD_OPEN_HOURS:
      case Metadata::FMD_EMAIL:
      case Metadata::FMD_POSTCODE:
      case Metadata::FMD_INTERNET:
      {
        NSString * v;
        if (type == Metadata::FMD_INTERNET)
          v = L(@"WiFi_available");
        else
          v = @(metadata.Get(type).c_str());

        NSNumber const * t = [self typeFromMetadata:type];
        [self.metaTypes addObject:t];
        [self.metaValues addObject:v];
        break;
      }

      default:
        break;
    }
  }

  NSUInteger swappedIndex = 0;
  for (NSNumber * pattern in kPatternTypesArray)
  {
    NSUInteger const index = [self.metaTypes indexOfObject:pattern];
    if (index == NSNotFound)
      continue;
    [self.metaTypes exchangeObjectAtIndex:index withObjectAtIndex:swappedIndex];
    [self.metaValues exchangeObjectAtIndex:index withObjectAtIndex:swappedIndex];
    swappedIndex++;
  }

  [self.metaTypes addObject:kPatternTypesArray.lastObject];
  [self.metaValues addObject:[self coordinates]];
}

- (NSArray *)metadataTypes
{
  return (NSArray *)self.metaTypes;
}

- (NSArray *)metadataValues
{
  return (NSArray *)self.metaValues;
}

- (void)enableEditing
{
  NSNumber * editType = @(MWMPlacePageMetadataTypeEditButton);
  if (![self.metaTypes containsObject:editType])
    [self.metaTypes addObject:editType];
}

- (void)insertBookmarkInTypes
{
  NSNumber * bookmarkType = @(MWMPlacePageMetadataTypeBookmark);
  if (![self.metaTypes containsObject:bookmarkType])
    [self.metaTypes addObject:bookmarkType];
}

- (void)removeBookmarkFromTypes
{
  [self.metaTypes removeObject:@(MWMPlacePageMetadataTypeBookmark)];
}

- (NSNumber *)typeFromMetadata:(uint8_t)type
{
  switch (type)
  {
    case Metadata::FMD_URL:
      return @(MWMPlacePageMetadataTypeURL);
    case Metadata::FMD_WEBSITE:
      return @(MWMPlacePageMetadataTypeWebsite);
    case Metadata::FMD_PHONE_NUMBER:
      return @(MWMPlacePageMetadataTypePhoneNumber);
    case Metadata::FMD_OPEN_HOURS:
      return @(MWMPlacePageMetadataTypeOpenHours);
    case Metadata::FMD_EMAIL:
      return @(MWMPlacePageMetadataTypeEmail);
    case Metadata::FMD_POSTCODE:
      return @(MWMPlacePageMetadataTypePostcode);
    case Metadata::FMD_INTERNET:
      return @(MWMPlacePageMetadataTypeWiFi);
    default:
      return nil;
  }
}

- (NSString *)coordinates
{
  BOOL const useDMSFormat = [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  return @((useDMSFormat ? MeasurementUtils::FormatLatLon(self.ll.lat, self.ll.lon).c_str()
                         : MeasurementUtils::FormatLatLonAsDMS(self.ll.lat, self.ll.lon, 2).c_str()));
}

#pragma mark - Bookmark editing

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

//
//  MWMPlacePageEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageEntity.h"
#import "UIKitCategories.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"
#import "UIKitCategories.h"
#include "../../../platform/measurement_utils.hpp"

extern NSArray * const kBookmarkColorsVariant = @[@"placemark-red", @"placemark-yellow", @"placemark-blue", @"placemark-green", @"placemark-purple", @"placemark-orange", @"placemark-brown", @"placemark-pink"];
extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";
static NSArray * const kPatternTypesArray = @[@(MWMPlacePageMetadataTypePostcode), @(MWMPlacePageMetadataTypePhoneNumber), @(MWMPlacePageMetadataTypeWebsite), @(MWMPlacePageMetadataTypeURL), @(MWMPlacePageMetadataTypeEmail), @(MWMPlacePageMetadataTypeOpenHours), @(MWMPlacePageMetadataTypeCoordinate)];

static NSString * const kTypesKey = @"types";
static NSString * const kValuesKey = @"values";

typedef feature::FeatureMetadata::EMetadataType TMetadataType;

@interface MWMPlacePageEntity ()

@property (nonatomic) NSDictionary * metadata;

@end

@implementation MWMPlacePageEntity

- (instancetype)initWithUserMark:(UserMark const *)mark
{
  self = [super init];
  if (self)
    [self configureWithUserMark:mark];

  return self;
}

- (void)configureWithUserMark:(UserMark const *)mark
{
  UserMark::Type const type = mark->GetMarkType();
  double x, y;
  mark->GetLatLon(x, y);
  self.point = m2::PointD(x, y);

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
    case Type::POI:
      [self configureForPOI:static_cast<SearchMarkPoint const *>(mark)];
      break;
    case Type::BOOKMARK:
      [self configureForBookmark:mark];
      break;
  }
 GetFramework().ActivateUserMark(mark);
}

- (void)configureForBookmark:(UserMark const *)bookmark
{
  Framework & f = GetFramework();
  self.bac = f.FindBookmark(bookmark);
  self.type = MWMPlacePageEntityTypeBookmark;
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  BookmarkData const & data = static_cast<Bookmark const *>(bookmark)->GetData();
  m2::PointD const & point = bookmark->GetOrg();
  feature::FeatureMetadata metadata;
  search::AddressInfo info;
  f.FindClosestPOIMetadata(point, metadata);
  f.GetAddressInfoForGlobalPoint(point, info);

  self.bookmarkTitle = [NSString stringWithUTF8String:data.GetName().c_str()];
  self.bookmarkCategory = [NSString stringWithUTF8String:category->GetName().c_str()];
  string const description = data.GetDescription();
  self.bookmarkDescription = [NSString stringWithUTF8String:description.c_str()];
  _isHTMLDescription = strings::IsHTML(description);
  self.bookmarkColor = [NSString stringWithUTF8String:data.GetType().c_str()];

  [self configureEntityWithMetadata:metadata addressInfo:info];
  [self insertBookmarkInTypes];
}

- (void)configureForPOI:(SearchMarkPoint const *)poiMark
{
  search::AddressInfo const & addressInfo = poiMark->GetInfo();
  feature::FeatureMetadata const & metadata = poiMark->GetMetadata();
  [self configureEntityWithMetadata:metadata addressInfo:addressInfo];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  NSMutableArray * types = [NSMutableArray array];
  NSMutableArray * values = [NSMutableArray array];
  [types addObject:kPatternTypesArray.lastObject];
  BOOL const isLatLonAsDMS = [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  NSString * latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String: MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()]: [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  [values addObject:latLonStr];

  self.metadata = @{kTypesKey : types, kValuesKey : values};
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{
// TODO(Vlad): Should implement this method.
}

- (void)configureEntityWithMetadata:(feature::FeatureMetadata const &)metadata addressInfo:(search::AddressInfo const &)info
{
  NSString * const name = [NSString stringWithUTF8String:info.GetPinName().c_str()];
  self.title = name.length > 0 ? name : L(@"dropped_pin");
  self.category = [NSString stringWithUTF8String:info.GetPinType().c_str()];

  vector<TMetadataType> const presentTypes = metadata.GetPresentTypes();

  NSMutableArray const * types = [NSMutableArray array];
  NSMutableArray const * values = [NSMutableArray array];

  for (auto const & type : presentTypes)
  {
    switch (type)
    {
      case TMetadataType::FMD_CUISINE:
      {
        NSString * cuisine = [NSString stringWithFormat:@"cuisine_%@", [NSString stringWithUTF8String:metadata.Get(type).c_str()]];
        self.category = [NSString stringWithFormat:@"%@, %@", self.category, L(cuisine)];
        break;
      }
      case TMetadataType::FMD_ELE:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeEle;
        break;
      }
      case TMetadataType::FMD_OPERATOR:
      {
        NSString const * bank = [NSString stringWithUTF8String:metadata.Get(type).c_str()];
        if (self.category.length)
          self.category = [NSString stringWithFormat:@"%@, %@", self.category, bank];
        else
          self.category = [NSString stringWithFormat:@"%@", bank];
        break;
      }
      case TMetadataType::FMD_STARS:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeHotel;
        break;
      }
      case TMetadataType::FMD_URL:
      case TMetadataType::FMD_WEBSITE:
      case TMetadataType::FMD_PHONE_NUMBER:
      case TMetadataType::FMD_OPEN_HOURS:
      case TMetadataType::FMD_EMAIL:
      case TMetadataType::FMD_POSTCODE:
      {
        NSString * v;
        if (type == feature::FeatureMetadata::EMetadataType::FMD_OPEN_HOURS)
          v = [self formattedOpenHoursFromString:metadata.Get(type)];
        else
          v = [NSString stringWithUTF8String:metadata.Get(type).c_str()];

        NSNumber const * t = [self typeFromMetadata:type];
        [types addObject:t];
        [values addObject:v];
        break;
      }
      case TMetadataType::FMD_TURN_LANES:
      case TMetadataType::FMD_TURN_LANES_BACKWARD:
      case TMetadataType::FMD_TURN_LANES_FORWARD:
      case TMetadataType::FMD_FAX_NUMBER:
      case TMetadataType::FMD_INTERNET:
        break;
    }
  }

  NSUInteger swappedIndex = 0;
  for (NSNumber * pattern in kPatternTypesArray)
  {
    NSUInteger const index = [types indexOfObject:pattern];
    if (index == NSNotFound)
      continue;
    [types exchangeObjectAtIndex:index withObjectAtIndex:swappedIndex];
    [values exchangeObjectAtIndex:index withObjectAtIndex:swappedIndex];
    swappedIndex++;
  }

  [types addObject:kPatternTypesArray.lastObject];
  BOOL const isLatLonAsDMS = [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  NSString * latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String:MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()] : [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String:MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()] : [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  [values addObject:latLonStr];
  
  self.metadata = @{kTypesKey : types, kValuesKey : values};
}

- (NSArray *)metadataTypes
{
  return (NSArray *)self.metadata[kTypesKey];
}

- (NSArray *)metadataValues
{
  return (NSArray *)self.metadata[kValuesKey];
}

- (void)insertBookmarkInTypes
{
  if ([self.metadataTypes containsObject:@(MWMPlacePageMetadataTypeBookmark)])
    return;
  [self.metadata[kTypesKey] insertObject:@(MWMPlacePageMetadataTypeBookmark) atIndex:self.metadataTypes.count];
}

- (void)removeBookmarkFromTypes
{
  [self.metadata[kTypesKey] removeObject:@(MWMPlacePageMetadataTypeBookmark)];
}

- (NSNumber *)typeFromMetadata:(TMetadataType)type
{
  switch (type)
  {
    case TMetadataType::FMD_URL:
      return @(MWMPlacePageMetadataTypeURL);
    case TMetadataType::FMD_WEBSITE:
      return @(MWMPlacePageMetadataTypeWebsite);
    case TMetadataType::FMD_PHONE_NUMBER:
      return @(MWMPlacePageMetadataTypePhoneNumber);
    case TMetadataType::FMD_OPEN_HOURS:
      return @(MWMPlacePageMetadataTypeOpenHours);
    case TMetadataType::FMD_EMAIL:
      return @(MWMPlacePageMetadataTypeEmail);
    case TMetadataType::FMD_POSTCODE:
      return @(MWMPlacePageMetadataTypePostcode);

    case TMetadataType::FMD_TURN_LANES:
    case TMetadataType::FMD_TURN_LANES_BACKWARD:
    case TMetadataType::FMD_TURN_LANES_FORWARD:
    case TMetadataType::FMD_FAX_NUMBER:
    case TMetadataType::FMD_INTERNET:
    case TMetadataType::FMD_STARS:
    case TMetadataType::FMD_OPERATOR:
    case TMetadataType::FMD_ELE:
    case TMetadataType::FMD_CUISINE:
      break;
  }
  return nil;
}

#pragma mark - Bookmark editing

- (NSString *)bookmarkCategory
{
  if (!_bookmarkCategory)
  {
    Framework & f = GetFramework();
    BookmarkCategory * category = f.GetBmCategory(f.LastEditedBMCategory());
    _bookmarkCategory = [NSString stringWithUTF8String:category->GetName().c_str()];
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
    _bookmarkColor = [NSString stringWithUTF8String:type.c_str()];
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

  Bookmark * bookmark = category->GetBookmark(self.bac.second);
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

  category->SaveToKMLFile();
}

#pragma mark - Open hours string formatter

- (NSString *)formattedOpenHoursFromString:(string const &)s
{
//TODO (Vlad): Not the best solution, but this function is temporary and will be replaced in future.
  NSMutableString * r = [NSMutableString stringWithUTF8String:s.c_str()];
  [r replaceOccurrencesOfString:@"," withString:@", " options:NSCaseInsensitiveSearch range:NSMakeRange(0, r.length)];
  while (YES)
  {
    NSRange const range = [r rangeOfString:@"  "];
    if (range.location == NSNotFound)
      break;
    [r replaceCharactersInRange:range withString:@" "];
  }
  [r replaceOccurrencesOfString:@"; " withString:@"\n" options:NSCaseInsensitiveSearch range:NSMakeRange(0, r.length)];
  [r replaceOccurrencesOfString:@";" withString:@"\n" options:NSCaseInsensitiveSearch range:NSMakeRange(0, r.length)];
  return r.copy;
}

@end

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

static NSArray * const kTypesArray = @[@"Coisine", @"OpenHours", @"PhoneNumber", @"FaxNumber", @"Stars", @"Operator", @"URL", @"Website", @"Internet", @"ELE", @"TurnLanes", @"TurnLanesForward", @"TurnLanesBackward", @"Email", @"Coordinate"];

extern NSArray * const kBookmarkColorsVariant = @[@"placemark-red", @"placemark-yellow", @"placemark-blue", @"placemark-green", @"placemark-purple", @"placemark-orange", @"placemark-brown", @"placemark-pink"];

extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";

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
  UserMark::Type type = mark->GetMarkType();
  double x, y;
  mark->GetLatLon(x, y);
  self.point = m2::PointD(x, y);
  switch (type)
  {
    case UserMark::Type::API:
    {
      ApiMarkPoint const * apiMark = static_cast<ApiMarkPoint const *>(mark);
      [self configureForApi:apiMark];
      break;
    }
    case UserMark::Type::SEARCH:
    {
      SearchMarkPoint const * searchMark = static_cast<SearchMarkPoint const *>(mark);
      [self configureForSearch:searchMark];
      break;
    }
    case UserMark::Type::DEBUG_MARK:
      break;

    case UserMark::Type::MY_POSITION:
    {
      MyPositionMarkPoint const * myPositionMark = static_cast<MyPositionMarkPoint const *>(mark);
      [self configureForMyPosition:myPositionMark];
      break;
    }

    case UserMark::Type::POI:
    {
      PoiMarkPoint const * poiMark = static_cast<PoiMarkPoint const *>(mark);
      [self configureForPOI:poiMark];
      break;
    }

    case UserMark::Type::BOOKMARK:
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
  BookmarkData data = static_cast<Bookmark const *>(bookmark)->GetData();
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
  NSUInteger const count = [self.metadata[@"keys"] count];
  [self.metadata[@"keys"] insertObject:@"Bookmark" atIndex:count];
}

- (void)configureForSearch:(SearchMarkPoint const *)searchMark
{
  m2::PointD const & point = searchMark->GetOrg();
  Framework & f = GetFramework();
  feature::FeatureMetadata metadata;
  search::AddressInfo info;
  f.FindClosestPOIMetadata(point, metadata);
  f.GetAddressInfoForGlobalPoint(point, info);
  [self configureEntityWithMetadata:metadata addressInfo:info];
}

- (void)configureForPOI:(PoiMarkPoint const *)poiMark
{
  search::AddressInfo const & addressInfo = poiMark->GetInfo();
  feature::FeatureMetadata const & metadata = poiMark->GetMetadata();
  [self configureEntityWithMetadata:metadata addressInfo:addressInfo];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  NSMutableArray * keys = [NSMutableArray array];
  NSMutableArray * values = [NSMutableArray array];
  [keys addObject:kTypesArray.lastObject];
  BOOL const isLatLonAsDMS = [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  NSString * latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String: MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()]: [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  [values addObject:latLonStr];

  self.metadata = @{@"keys" : keys, @"values" : values};
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{

}

- (void)configureEntityWithMetadata:(feature::FeatureMetadata const &)metadata addressInfo:(search::AddressInfo const &)info
{
  self.title = [NSString stringWithUTF8String:info.GetPinName().c_str()];
  self.category = [NSString stringWithUTF8String:info.GetPinType().c_str()];

  vector<feature::FeatureMetadata::EMetadataType> presentTypes = metadata.GetPresentTypes();

  NSMutableArray * keys = [NSMutableArray array];
  NSMutableArray * values = [NSMutableArray array];

  for (auto const & type : presentTypes)
  {
    if (type == feature::FeatureMetadata::EMetadataType::FMD_POSTCODE)
      continue;

    if (type == feature::FeatureMetadata::EMetadataType::FMD_OPERATOR)
    {
      NSString * bank = [NSString stringWithUTF8String:metadata.Get(type).c_str()];
      self.category = [NSString stringWithFormat:@"%@, %@", self.category, bank];
      continue;
    }

    if (type == feature::FeatureMetadata::EMetadataType::FMD_CUISINE)
    {
      NSString * cuisine = [NSString stringWithFormat:@"cuisine_%@", [NSString stringWithUTF8String:metadata.Get(type).c_str()]];
      self.category = [NSString stringWithFormat:@"%@, %@", self.category, L(cuisine)];
      continue;
    }
    
    if (type == feature::FeatureMetadata::EMetadataType::FMD_ELE)
    {
      self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
      if (self.type != MWMPlacePageEntityTypeBookmark)
        self.type = MWMPlacePageEntityTypeEle;
      continue;
    }

    if (type == feature::FeatureMetadata::EMetadataType::FMD_STARS)
    {
      self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
      if (self.type != MWMPlacePageEntityTypeBookmark)
        self.type = MWMPlacePageEntityTypeHotel;
      continue;
    }

    NSString * value;

    if (type == feature::FeatureMetadata::EMetadataType::FMD_OPEN_HOURS)
      value = [[NSString stringWithUTF8String:metadata.Get(type).c_str()] stringByReplacingOccurrencesOfString:@"; " withString:@";\n"];
    else
      value = [NSString stringWithUTF8String:metadata.Get(type).c_str()];

    NSString *key = [self stringFromMetadataType:type];
    [keys addObject:key];
    [values addObject:value];
  }

  [keys addObject:kTypesArray.lastObject];
  BOOL const isLatLonAsDMS = [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  NSString * latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String:MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()] : [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  latLonStr = isLatLonAsDMS ? [NSString stringWithUTF8String:MeasurementUtils::FormatLatLonAsDMS(self.point.x, self.point.y, 2).c_str()] : [NSString stringWithUTF8String: MeasurementUtils::FormatLatLon(self.point.x, self.point.y).c_str()];
  [values addObject:latLonStr];

  self.metadata = @{@"keys" : keys, @"values" : values};
}

- (NSString *)stringFromMetadataType:(feature::FeatureMetadata::EMetadataType)type
{
  return kTypesArray[type - 1];
}

#pragma mark - Bookmark editing

- (NSString *)bookmarkCategory
{
  if (_bookmarkCategory == nil)
  {
    Framework & f = GetFramework();
    BookmarkCategory * category = f.GetBmCategory(f.LastEditedBMCategory());
    return [NSString stringWithUTF8String:category->GetName().c_str()];
  }
  return _bookmarkCategory;
}

- (NSString *)bookmarkDescription
{
  if (_bookmarkDescription == nil)
    return @"";

  return _bookmarkDescription;
}

- (NSString *)bookmarkColor
{
  if (_bookmarkColor == nil)
  {
    Framework & f = GetFramework();
    string type = f.LastEditedBMType();
    return [NSString stringWithUTF8String:type.c_str()];
  }
  return _bookmarkColor;
}

- (NSString *)bookmarkTitle
{
  if (_bookmarkTitle == nil)
    return self.title;
  return _bookmarkTitle;
}

- (void)synchronize
{
  Framework & f = GetFramework();
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  Bookmark * bookmark = category->GetBookmark(self.bac.second);

  if (!bookmark)
    return;
  

  if (self.bookmarkColor)
    bookmark->SetType(self.bookmarkColor.UTF8String);

  if (self.bookmarkDescription)
  {
    string const description (self.bookmarkDescription.UTF8String);
    _isHTMLDescription = strings::IsHTML(description);
    bookmark->SetDescription(self.bookmarkDescription.UTF8String);
  }

  if (self.bookmarkTitle)
    bookmark->SetName(self.bookmarkTitle.UTF8String);

  category->SaveToKMLFile();
}

- (MWMPlacePageEntityType)type
{
  if (!_type)
    return MWMPlacePageEntityTypeRegular;

  return _type;
}

@end

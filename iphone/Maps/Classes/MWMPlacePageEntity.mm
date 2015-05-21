//
//  MWMPlacePageEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageEntity.h"
#import "UIKitCategories.h"

#import "Framework.h"

static NSArray * const kTypesArray = @[@"Coisine", @"OpenHours", @"PhoneNumber", @"FaxNumber", @"Stars", @"Operator", @"URL", @"Website", @"Internet", @"ELE", @"TurnLanes", @"TurnLanesForward", @"TurnLanesBackward", @"Email", @"Coordinate"];

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

    case UserMark::Type::BOOKMARK:

      break;

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

  }
  GetFramework().ActivateUserMark(mark);
}

- (void)configureForSearch:(SearchMarkPoint const *)searchMark
{
  search::AddressInfo const & info = searchMark->GetInfo();
  self.title = [NSString stringWithUTF8String:info.m_name.c_str()];
}

- (void)configureForPOI:(PoiMarkPoint const *)poiMark
{
  search::AddressInfo const & addressInfo = poiMark->GetInfo();
  self.title = [NSString stringWithUTF8String:addressInfo.GetPinName().c_str()];
  self.category = [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];

  m2::PointD const & point = poiMark->GetOrg();
  feature::FeatureMetadata const & metadata = poiMark->GetMetadata();
  vector<feature::FeatureMetadata::EMetadataType> presentTypes = metadata.GetPresentTypes();

  NSMutableArray * keys = [NSMutableArray array];
  NSMutableArray * values = [NSMutableArray array];

  for (auto const & type : presentTypes)
  {
    if (type == feature::FeatureMetadata::EMetadataType::FMD_OPERATOR)
      continue;

    if (type == feature::FeatureMetadata::EMetadataType::FMD_CUISINE)
    {
      self.category = [NSString stringWithFormat:@"%@, %@", self.category, L([NSString stringWithUTF8String:metadata.Get(type).c_str()])];
      continue;
    }

    NSString *value;

    if (type == feature::FeatureMetadata::EMetadataType::FMD_OPEN_HOURS)
      value = [[NSString stringWithUTF8String:metadata.Get(type).c_str()] stringByReplacingOccurrencesOfString:@"; " withString:@";\n"];
    else
      value = [NSString stringWithUTF8String:metadata.Get(type).c_str()];

    NSString *key = [self stringFromMetadataType:type];
    [keys addObject:key];
    [values addObject:value];
  }

  [keys addObject:kTypesArray.lastObject];
  [values addObject:[NSString stringWithFormat:@"%.6f, %.6f", point.y, point.x]];

  self.metadata = @{@"keys" : keys, @"values" : values};
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{

}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{

}

- (NSString *)stringFromMetadataType:(feature::FeatureMetadata::EMetadataType)type
{
  return kTypesArray[type - 1];
}

@end

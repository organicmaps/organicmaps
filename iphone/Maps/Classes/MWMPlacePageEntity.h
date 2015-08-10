//
//  MWMPlacePageEntity.h
//  Maps
//
//  Created by v.mikhaylenko on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Framework.h"

#include "map/user_mark.hpp"

typedef NS_ENUM (NSUInteger, MWMPlacePageMetadataType)
{
  MWMPlacePageMetadataTypePostcode,
  MWMPlacePageMetadataTypePhoneNumber,
  MWMPlacePageMetadataTypeWebsite,
  MWMPlacePageMetadataTypeURL,
  MWMPlacePageMetadataTypeEmail,
  MWMPlacePageMetadataTypeOpenHours,
  MWMPlacePageMetadataTypeCoordinate,
  MWMPlacePageMetadataTypeWiFi,
  MWMPlacePageMetadataTypeBookmark
};

typedef NS_ENUM (NSUInteger, MWMPlacePageEntityType)
{
  MWMPlacePageEntityTypeRegular,
  MWMPlacePageEntityTypeBookmark,
  MWMPlacePageEntityTypeEle,
  MWMPlacePageEntityTypeHotel,
  MWMPlacePageEntityTypeAPI,
  MWMPlacePageEntityTypeMyPosition
};

@class MWMPlacePageViewManager;

@interface MWMPlacePageEntity : NSObject

@property (copy, nonatomic) NSString * title;
@property (copy, nonatomic) NSString * category;
@property (copy, nonatomic) NSString * bookmarkTitle;
@property (copy, nonatomic) NSString * bookmarkCategory;
@property (copy, nonatomic) NSString * bookmarkDescription;
@property (nonatomic, readonly) BOOL isHTMLDescription;
@property (copy, nonatomic) NSString * bookmarkColor;

@property (nonatomic) MWMPlacePageEntityType type;

@property (nonatomic) int typeDescriptionValue;

@property (nonatomic) BookmarkAndCategory bac;
@property (nonatomic) m2::PointD point;
@property (weak, nonatomic) MWMPlacePageViewManager * manager;

- (NSArray *)metadataTypes;
- (NSArray *)metadataValues;
- (void)insertBookmarkInTypes;
- (void)removeBookmarkFromTypes;

- (instancetype)initWithUserMark:(UserMark const *)mark;
- (void)synchronize;

@end

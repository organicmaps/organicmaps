//
//  MWMAlertEntity.h
//  Maps
//
//  Created by v.mikhaylenko on 06.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "../../../std/vector.hpp"
#include "../../../map/country_status_display.hpp"

typedef NS_ENUM(NSUInteger, MWMAlertEntityType) {
  MWMAlertEntityTypeDownloader
};

@interface MWMAlertEntity : NSObject

@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *message;
@property (nonatomic, copy) NSString *contry;
@property (nonatomic, copy) NSString *location;
@property (nonatomic, assign) NSUInteger size;

+ (instancetype)entityWithType:(MWMAlertEntityType)type;

@end

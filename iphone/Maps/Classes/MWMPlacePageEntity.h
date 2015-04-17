//
//  MWMPlacePageEntity.h
//  Maps
//
//  Created by v.mikhaylenko on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "map/user_mark.hpp"

@interface MWMPlacePageEntity : NSObject

@property (copy, nonatomic) NSString *title;
@property (copy, nonatomic) NSString *category;
@property (copy, nonatomic) NSString *distance;
@property (copy, nonatomic) NSDictionary *metadata;

- (instancetype)initWithUserMark:(UserMark const *)mark;

@end

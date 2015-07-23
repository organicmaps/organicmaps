//
//  MWMNavigationDashboardEntity.h
//  Maps
//
//  Created by v.mikhaylenko on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "platform/location.hpp"

@interface MWMNavigationDashboardEntity : NSObject

@property (nonatomic, readonly) NSString * targetDistance;
@property (nonatomic, readonly) NSString * targetUnits;
@property (nonatomic, readonly) NSString * distanceToTurn;
@property (nonatomic, readonly) NSString * turnUnits;
@property (nonatomic, readonly) UIImage * turnImage;
@property (nonatomic, readonly) NSUInteger roundExitNumber;
@property (nonatomic, readonly) NSUInteger timeToTarget;

- (instancetype)initWithFollowingInfo:(location::FollowingInfo const &)info;
- (void)updateWithFollowingInfo:(location::FollowingInfo const &)info;

- (instancetype)init __attribute__((unavailable("init is not available")));
+ (instancetype)new __attribute__((unavailable("init is not available")));

@end

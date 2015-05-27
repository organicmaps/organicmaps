//
//  RouteState.h
//  Maps
//
//  Created by Ilya Grechuhin on 26.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <geometry/point2d.hpp>

@interface RouteState : NSObject

@property (nonatomic, readonly) BOOL hasActualRoute;
@property (nonatomic) m2::PointD endPoint;

+ (instancetype)savedState;

+ (void)save;
+ (void)remove;

@end

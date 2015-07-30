//
//  MWMWatchLocationTracker.h
//  Maps
//
//  Created by v.mikhaylenko on 09.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMWatchLocationTrackerDelegate.h"

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include "platform/location.hpp"

@interface MWMWatchLocationTracker : NSObject

+ (instancetype)sharedLocationTracker;
- (location::GpsInfo)infoFromCurrentLocation;
- (NSString *)distanceToPoint:(m2::PointD const &)point;

+ (instancetype)alloc __attribute__((unavailable("call sharedLocationTracker instead")));
- (instancetype)init __attribute__((unavailable("call sharedLocation instead")));
+ (instancetype)new __attribute__((unavailable("call sharedLocationTracker instead")));

@property (nonatomic, readonly) CLLocationManager * locationManager;
@property (weak, nonatomic) id<MWMWatchLocationTrackerDelegate> delegate;
@property (nonatomic) m2::PointD destinationPosition;
@property (nonatomic, readonly) BOOL hasLocation;
@property (nonatomic, readonly) BOOL hasDestination;
@property (nonatomic, readonly) BOOL locationServiceEnabled;
@property (nonatomic, readonly) CLLocationCoordinate2D currentCoordinate;

- (void)clearDestination;

- (BOOL)arrivedToDestination;
- (BOOL)movementStopped;

@end

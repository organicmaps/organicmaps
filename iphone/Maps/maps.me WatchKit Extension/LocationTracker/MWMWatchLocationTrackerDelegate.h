//
//  MWMWatchLocationTrackerDelegate.h
//  Maps
//
//  Created by Ilya Grechuhin on 22.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@protocol MWMWatchLocationTrackerDelegate <NSObject>

@optional
- (void)onChangeLocation:(CLLocation *)location;
- (void)locationTrackingFailedWithError:(NSError *)error;
- (void)didChangeAuthorizationStatus:(BOOL)haveAuthorization;

@end

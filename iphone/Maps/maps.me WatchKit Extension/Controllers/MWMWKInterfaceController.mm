//
//  MWMWKInterfaceController.m
//  Maps
//
//  Created by Ilya Grechuhin on 22.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMWKInterfaceController.h"
#import "MWMWatchLocationTracker.h"

extern NSString * const kNoLocationControllerIdentifier;

@implementation MWMWKInterfaceController

- (void)willActivate
{
  [super willActivate];
  [self checkLocationService];
}

- (void)checkLocationService
{
  MWMWatchLocationTracker *tracker = [MWMWatchLocationTracker sharedLocationTracker];
  _haveLocation = tracker.locationServiceEnabled;
  if (self.haveLocation)
    tracker.delegate = self;
  else
    [self pushControllerWithName:kNoLocationControllerIdentifier context:@NO];
}

- (void)pushControllerWithName:(NSString *)name context:(id)context
{
  _haveLocation = NO;
  [MWMWatchLocationTracker sharedLocationTracker].delegate = nil;
  [super pushControllerWithName:name context:context];
}

#pragma mark - MWMWatchLocationTrackerDelegate

- (void)locationTrackingFailedWithError:(NSError *)error
{
  [self pushControllerWithName:kNoLocationControllerIdentifier context:@YES];
}

- (void)didChangeAuthorizationStatus:(BOOL)haveAuthorization
{
  if (!haveAuthorization)
    [self pushControllerWithName:kNoLocationControllerIdentifier context:@NO];
}

@end

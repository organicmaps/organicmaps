//
//  MWMNoLocationInterfaceController.m
//  Maps
//
//  Created by i.grechuhin on 09.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNoLocationInterfaceController.h"
#import "MWMWatchLocationTrackerDelegate.h"
#import "MWMWatchLocationTracker.h"
#import <CoreLocation/CoreLocation.h>
#import "Macros.h"

@interface MWMNoLocationInterfaceController() <MWMWatchLocationTrackerDelegate>

@property (weak, nonatomic) IBOutlet WKInterfaceLabel * descriptionLabel;

@end

extern NSString * const kNoLocationControllerIdentifier = @"NoLocationController";

@implementation MWMNoLocationInterfaceController

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
  [MWMWatchLocationTracker sharedLocationTracker].delegate = self;
  if ([context boolValue])
    [self.descriptionLabel setText:L(@"undefined_location")];
  else
    [self.descriptionLabel setText:L(@"enable_location_services")];
}

#pragma mark - MWMWatchLocationTrackerDelegate

- (void)onChangeLocation:(CLLocation *)location
{
  [self popController];
}

- (void)didChangeAuthorizationStatus:(BOOL)haveAuthorization
{
  if (haveAuthorization)
    [self popController];
}

@end

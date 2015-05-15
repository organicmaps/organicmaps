//
//  MWMFollowController.m
//  Maps
//
//  Created by v.mikhaylenko on 10.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMFollowController.h"
#import "FrameworkUtils.h"
#import "MWMWatchLocationTracker.h"

#include "../../../platform/location.hpp"
#include "Framework.h"

@interface MWMFollowController()

@property (weak, nonatomic) IBOutlet WKInterfaceGroup *mapGroup;
@property (assign, nonatomic) CGPoint destinationPoint;

@end

static NSString * const kFinishControllerIdentifier = @"FinishController";

@implementation MWMFollowController

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
  NSValue *point = context;
  self.destinationPoint = point.CGPointValue;
}

- (void)didDeactivate
{
  [super didDeactivate];
  Framework & f = GetFramework();
  f.GetBookmarkManager().UserMarksClear(UserMarkContainer::SEARCH_MARK);
}

- (BOOL)firstPoint:(m2::PointD const &)pt1 isEqualToPoint:(m2::PointD const &)pt2
{
  static double const accuracy = 0.00001;

  if (fabs(pt2.x - pt1.x) < accuracy && fabs(pt2.y - pt1.y) < accuracy)
    return YES;

  return NO;
}

#pragma mark - Base class virtual methods

- (void)initializeFramework
{
  Framework & f = GetFramework();
  f.GetBookmarkManager().UserMarksClear(UserMarkContainer::SEARCH_MARK);
  m2::PointD pt(self.destinationPoint.x, self.destinationPoint.y);
  f.GetBookmarkManager().UserMarksAddMark(UserMarkContainer::SEARCH_MARK, pt);
  f.Invalidate();
}

- (void)prepareFramework
{
  MWMWatchLocationTracker *tracker = [MWMWatchLocationTracker sharedLocationTracker];
  CLLocation *location = [[tracker locationManager] location];
  CLLocationCoordinate2D coordinate = location.coordinate;
  m2::PointD destination (self.destinationPoint.x, self.destinationPoint.y);
  m2::PointD current (MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude));

  if ([self firstPoint:current isEqualToPoint:destination])
  {
    [self pushControllerWithName:kFinishControllerIdentifier context:nil];
    return;
  }

  location::GpsInfo info = [tracker infoFromCurrentLocation];

  Framework & f = GetFramework();
  [FrameworkUtils setWatchScreenRectWithCenterPoint:current targetPoint:destination];
  f.OnLocationUpdate(info);

  [self setTitle:[tracker distanceToPoint:destination]];
}

@end

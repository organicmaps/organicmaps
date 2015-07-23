//
//  MWMNavigationDashboard.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "TimeUtils.h"

@implementation MWMNavigationDashboard

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  self.direction.image = entity.turnImage;
  self.distanceToNextAction.text = entity.distanceToTurn;
  self.distanceToNextActionUnits.text = entity.turnUnits;
  self.distanceLeft.text = [NSString stringWithFormat:@"%@ %@", entity.targetDistance, entity.targetUnits];
  self.eta.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
}

@end

//
//  MWMNavigationDashboard.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "TimeUtils.h"

@interface MWMImageView : UIImageView

@end

@implementation MWMImageView

- (void)setCenter:(CGPoint)center
{
//TODO(Vlad): There is hack for "cut" iOS7.
  if (isIOSVersionLessThan(8))
    return;
  [super setCenter:center];
}

@end

@implementation MWMNavigationDashboard

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  self.direction.image = entity.turnImage;
  if (!entity.isPedestrian)
    self.direction.transform = CGAffineTransformIdentity;
  self.distanceToNextAction.text = entity.distanceToTurn;
  self.distanceToNextActionUnits.text = entity.turnUnits;
  self.distanceLeft.text = [NSString stringWithFormat:@"%@ %@", entity.targetDistance, entity.targetUnits];
  self.eta.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
  self.arrivalsTimeLabel.text = [NSDateFormatter localizedStringFromDate:[[NSDate date]
                                                 dateByAddingTimeInterval:entity.timeToTarget]
                                                 dateStyle:NSDateFormatterNoStyle timeStyle:NSDateFormatterShortStyle];
  self.roundRoadLabel.text = entity.roundExitNumber ? @(entity.roundExitNumber).stringValue : @"";
}

@end

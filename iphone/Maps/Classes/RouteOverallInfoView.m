//
//  RouteOverallInfoView.m
//  Maps
//
//  Created by Timur Bernikowich on 24/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import "RouteOverallInfoView.h"
#import "TimeUtils.h"
#import "UIFont+MapsMeFonts.h"
#import "UIKitCategories.h"

@implementation RouteOverallInfoView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  
  [self addSubview:self.distanceLabel];
  [self addSubview:self.metricsLabel];
  [self addSubview:self.timeLeftLabel];
  
  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  self.distanceLabel.text = info[@"targetDistance"];
  self.metricsLabel.text = [info[@"targetMetrics"] uppercaseString];
  self.timeLeftLabel.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:info[@"timeToTarget"]];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self layoutSubviews];
  }];
}

- (void)layoutSubviews
{
  [self.distanceLabel sizeToIntegralFit];
  [self.metricsLabel sizeToIntegralFit];
  [self.timeLeftLabel sizeToIntegralFit];
  
  CGFloat const betweenOffset = 2;
  
  self.distanceLabel.minX = 0;
  self.metricsLabel.minX = self.distanceLabel.minX + self.distanceLabel.width + betweenOffset;
  self.distanceLabel.minY = 0;
  self.metricsLabel.maxY = self.distanceLabel.maxY - 2;
  self.timeLeftLabel.minY = self.metricsLabel.maxY + 2;
}

- (UILabel *)metricsLabel
{
  if (!_metricsLabel)
  {
    _metricsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _metricsLabel.backgroundColor = [UIColor clearColor];
    _metricsLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:11];
    _metricsLabel.textColor = [UIColor blackColor];
  }
  return _metricsLabel;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:19];
    _distanceLabel.textColor = [UIColor blackColor];
  }
  return _distanceLabel;
}

- (UILabel *)timeLeftLabel
{
  if (!_timeLeftLabel)
  {
    _timeLeftLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _timeLeftLabel.backgroundColor = [UIColor clearColor];
    _timeLeftLabel.font = [UIFont regular14];
    _timeLeftLabel.textColor = [UIColor colorWithColorCode:@"565656"];
  }
  return _timeLeftLabel;
}

@end

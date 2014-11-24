//
//  NextTurnPhoneView.m
//  Maps
//
//  Created by Timur Bernikowich on 24/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import "NextTurnPhoneView.h"
#import "UIKitCategories.h"

@implementation NextTurnPhoneView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  
  [self addSubview:self.turnTypeView];
  [self addSubview:self.distanceLabel];
  [self addSubview:self.metricsLabel];
  
  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  self.turnTypeView.image = [self turnImageWithType:info[@"turnType"]];
  self.distanceLabel.text = info[@"targetDistance"];
  self.metricsLabel.text = [info[@"targetMetrics"] uppercaseString];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self layoutSubviews];
  }];
}

- (UIImage *)turnImageWithType:(NSString *)turnType
{
  NSString * turnTypeImageName = [NSString stringWithFormat:@"big-%@", turnType];
  return [UIImage imageNamed:turnTypeImageName];
}

- (void)layoutSubviews
{
  [self.distanceLabel sizeToIntegralFit];
  [self.metricsLabel sizeToIntegralFit];
  
  CGFloat const betweenOffset = 2;
  
  self.turnTypeView.maxX = self.width / 2.0 - 16.0;
  self.turnTypeView.midY = self.height / 2.0;
  
  self.distanceLabel.minX = self.width / 2.0 - 8;
  self.distanceLabel.midY = self.turnTypeView.midY;
  
  self.metricsLabel.minX = self.distanceLabel.maxX + betweenOffset;
  self.metricsLabel.maxY = self.distanceLabel.maxY - 7;
}

- (UIImageView *)turnTypeView
{
  if (!_turnTypeView)
  {
    _turnTypeView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 48, 48)];
    _turnTypeView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _turnTypeView;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:44];
    _distanceLabel.textColor = [UIColor colorWithColorCode:@"2291D1"];
  }
  return _distanceLabel;
}

- (UILabel *)metricsLabel
{
  if (!_metricsLabel)
  {
    _metricsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _metricsLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:10];
    _metricsLabel.textColor = [UIColor colorWithColorCode:@"2291D1"];
  }
  return _metricsLabel;
}

@end
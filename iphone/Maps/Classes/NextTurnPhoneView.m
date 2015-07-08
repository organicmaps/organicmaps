//
//  NextTurnPhoneView.m
//  Maps
//
//  Created by Timur Bernikowich on 24/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import "NextTurnPhoneView.h"
#import "UIFont+MapsMeFonts.h"
#import "UIKitCategories.h"

@implementation NextTurnPhoneView

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  
  [self addSubview:self.turnTypeView];
  [self.turnTypeView addSubview:self.turnValue];
  [self addSubview:self.distanceLabel];
  [self addSubview:self.metricsLabel];
  
  return self;
}

- (void)updateWithInfo:(NSDictionary *)info
{
  self.turnTypeView.image = [self turnImageWithType:info[@"turnType"]];
  NSNumber * turnValue = info[@"turnTypeValue"];
  self.turnValue.text = [turnValue integerValue] ? [turnValue stringValue] : @"";
  self.distanceLabel.text = info[@"turnDistance"];
  self.metricsLabel.text = [info[@"turnMetrics"] uppercaseString];
  
  [UIView animateWithDuration:0.2 animations:^{
    [self updateSubviews];
  }];
}

- (UIImage *)turnImageWithType:(NSString *)turnType
{
  NSString * turnTypeImageName = [NSString stringWithFormat:@"big-%@", turnType];
  
  if ([turnTypeImageName isEqualToString:@"big-left-1"])
    return [self flippedImageWithImage:[UIImage imageNamed:@"big-right-1"]];
  if ([turnTypeImageName isEqualToString:@"big-left-2"])
    return [self flippedImageWithImage:[UIImage imageNamed:@"big-right-2"]];
  if ([turnTypeImageName isEqualToString:@"big-left-3"])
    return [self flippedImageWithImage:[UIImage imageNamed:@"big-right-3"]];
  
  return [UIImage imageNamed:turnTypeImageName];
}

- (UIImage *)flippedImageWithImage:(UIImage *)sourceImage
{
  UIImage * flippedImage = [UIImage imageWithCGImage:sourceImage.CGImage scale:sourceImage.scale orientation:UIImageOrientationUpMirrored];
  return flippedImage;
}

- (void)setFrame:(CGRect)frame
{
  [super setFrame:frame];
  [self configureSubviews];
}

- (void)configureSubviews
{
  self.turnValue.frame = self.turnTypeView.bounds;
  
  self.turnTypeView.maxX = self.width / 2.0 - 16.0;
  self.turnTypeView.midY = self.height / 2.0 - 2;
  
  [self updateSubviews];
}

- (void)updateSubviews
{
  [self.distanceLabel sizeToIntegralFit];
  [self.metricsLabel sizeToIntegralFit];
  
  CGFloat const betweenOffset = 2;
  
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

- (UILabel *)turnValue
{
  if (!_turnValue)
  {
    _turnValue = [[UILabel alloc] initWithFrame:CGRectZero];
    _turnValue.textAlignment = NSTextAlignmentCenter;
    _turnValue.backgroundColor = [UIColor clearColor];
    _turnValue.font = [UIFont fontWithName:@"HelveticaNeue" size:12];
    _turnValue.textColor = [UIColor blackColor];
  }
  return _turnValue;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:44];
    _distanceLabel.textColor = [UIColor colorWithColorCode:@"2291D1"];
  }
  return _distanceLabel;
}

- (UILabel *)metricsLabel
{
  if (!_metricsLabel)
  {
    _metricsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _metricsLabel.backgroundColor = [UIColor clearColor];
    _metricsLabel.font = [UIFont regular14];
    _metricsLabel.textColor = [UIColor colorWithColorCode:@"2291D1"];
  }
  return _metricsLabel;
}

@end
//
//  MWMLocationButtonStatusLabel.m
//  Maps
//
//  Created by Ilya Grechuhin on 29.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLocationButtonStatusLabel.h"
#import "MWMMapViewControlsCommon.h"
#import "UIKitCategories.h"

@implementation MWMLocationButtonStatusLabel

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (!self)
    return nil;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)layoutSubviews
{
  static CGFloat const maxLabelWidthPercentOfScreen = 0.52;
  static CGFloat const hSpacing = 4.0;
  static CGFloat const vSpacing = 1.0;

  [super layoutSubviews];
  self.width = maxLabelWidthPercentOfScreen * self.superview.width;
  [self sizeToFit];
  self.width = self.width + 2.0 * hSpacing;
  self.height = self.height + 2.0 * vSpacing;
  self.midX = self.superview.midX;
  self.maxY = self.superview.height - 2.0 * kViewControlsOffsetToBounds;
}

- (void)show
{
  self.hidden = NO;
  self.alpha = 0.0;
  [UIView animateWithDuration:framesDuration(1) animations:^
  {
    self.alpha = 1.0;
  }
  completion:^(BOOL finished)
  {
    [self performSelector:@selector(hide) withObject:self afterDelay:framesDuration(30)];
  }];
}

- (void)hide
{
  [UIView animateWithDuration:framesDuration(1) animations:^
  {
    self.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    self.hidden = YES;
  }];
}

@end

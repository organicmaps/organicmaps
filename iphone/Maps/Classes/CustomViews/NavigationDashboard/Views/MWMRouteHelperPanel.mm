//
//  MWMRouteHelperPanel.m
//  Maps
//
//  Created by v.mikhaylenko on 26.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "MWMRouteHelperPanel.h"
#import "UIKitCategories.h"

static CGFloat const kHeight = 40.;

@implementation MWMRouteHelperPanel

- (void)setHidden:(BOOL)hidden
{
  if (IPAD)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ self.alpha = hidden ? 0. : 1.; }];
    return;
  }
  [super setHidden:hidden];
  if (hidden)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ self.alpha = 0.; }];
  }
  else
  {
    CGFloat const offsetFactor = 40.;
    CGFloat const scaleFactor = 0.4;
    auto const scale = CGAffineTransformMakeScale(scaleFactor, scaleFactor);
    self.transform = scale;
    self.minY += offsetFactor;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.transform = CGAffineTransformIdentity;
      self.alpha = 1.;
      self.minY -= offsetFactor;
    }];
  }
}

- (void)setTopBound:(CGFloat)topBound
{
  [self layoutIfNeeded];
  _topBound = topBound;
  self.minY = topBound;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
}

- (void)layoutSubviews
{
  self.height = self.defaultHeight;
  self.minY = self.topBound;
  if (IPAD)
    self.minX = 0.;
  else
    self.center = {self.parentView.center.x, self.center.y};
  [super layoutSubviews];
}

- (CGFloat)defaultHeight
{
  return kHeight;
}

@end

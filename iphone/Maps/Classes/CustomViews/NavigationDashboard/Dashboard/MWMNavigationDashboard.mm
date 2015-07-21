//
//  MWMNavigationDashboard.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationDashboard.h"

@interface MWMNavigationDashboard ()

@end

@implementation MWMNavigationDashboard

- (void)addToView:(UIView *)superview
{
  [superview addSubview:self];
  self.frame = self.defaultFrame;
  [self setInitialPosition];
  [self moveIn];
}

- (void)remove
{
  [UIView animateWithDuration:0.2 animations:^
  {
    [self setInitialPosition];
  }
  completion:^(BOOL finished)
  {
    [self removeFromSuperview];
  }];
}

- (void)setInitialPosition
{
  self.maxY = 0.0;
}

- (void)moveIn
{
  [UIView animateWithDuration:0.2 animations:^{ self.origin = CGPointZero; }];
}

- (void)layoutSubviews
{
  self.frame = self.defaultFrame;
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  return CGRectMake(0.0, self.topOffset, self.superview.width, 92.0);
}

@end

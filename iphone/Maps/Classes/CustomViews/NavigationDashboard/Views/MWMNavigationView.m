//
//  MWMNavigationView.m
//  Maps
//
//  Created by Ilya Grechuhin on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationView.h"

static CGFloat const kStatusbarHeight = 20.0;

@interface MWMNavigationView ()

@property (nonatomic) BOOL isVisible;
@property (nonatomic) CGFloat defaultHeight;

@property (nonatomic) UIView * statusbarBackground;
@property (weak, nonatomic) IBOutlet UIView * contentView;

@end

@implementation MWMNavigationView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.statusbarBackground = [[UIView alloc] initWithFrame:CGRectZero];
  self.statusbarBackground.backgroundColor = self.contentView.backgroundColor;
  self.defaultHeight = self.height;
  self.topBound = 0.0;
}

- (void)addToView:(UIView *)superview
{
  [superview insertSubview:self atIndex:0];
  self.frame = self.defaultFrame;
  dispatch_async(dispatch_get_main_queue(), ^{ self.isVisible = YES; });
}

- (void)remove
{
  self.isVisible = NO;
}

- (void)layoutSubviews
{
  [UIView animateWithDuration:0.2 animations:^
  {
    if (!CGRectEqualToRect(self.frame, self.defaultFrame))
    {
      self.frame = self.defaultFrame;
      [self layoutStatusbar];
    }
  }
  completion:^(BOOL finished)
  {
    if (!self.isVisible)
      [self removeFromSuperview];
  }];
  [super layoutSubviews];
}

- (void)layoutStatusbar
{
  if (self.topBound <= kStatusbarHeight)
  {
    if (![self.statusbarBackground.superview isEqual:self])
      [self addSubview:self.statusbarBackground];
    self.statusbarBackground.frame = CGRectMake(0.0, -kStatusbarHeight, self.width, kStatusbarHeight);
  }
  else
  {
    [self.statusbarBackground removeFromSuperview];
  }
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  return CGRectMake(0.0, self.topBound - (self.isVisible ? 0.0 : self.defaultHeight), self.superview.width, self.defaultHeight);
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = MAX(topBound, kStatusbarHeight);
  [self layoutSubviews];
}

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  [self layoutSubviews];
}

- (CGFloat)visibleHeight
{
  CGFloat height = self.contentView.height;
  if ([self.statusbarBackground.superview isEqual:self])
    height += self.statusbarBackground.height;
  return height;
}

@end

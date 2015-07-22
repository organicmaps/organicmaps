//
//  MWMNavigationView.m
//  Maps
//
//  Created by Ilya Grechuhin on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationView.h"

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
  self.isVisible = YES;
}

- (void)remove
{
  self.isVisible = NO;
}

- (void)layoutSubviews
{
  [UIView animateWithDuration:0.2 animations:^
  {
    self.frame = self.defaultFrame;
    [self layoutStatusbar];
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
  CGRect const statusBarFrame = UIApplication.sharedApplication.statusBarFrame;
  if (self.topBound <= statusBarFrame.size.height)
  {
    if (![self.statusbarBackground.superview isEqual:self])
      [self addSubview:self.statusbarBackground];
    self.statusbarBackground.frame = statusBarFrame;
    self.statusbarBackground.maxY = 0.0;
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
  _topBound = MAX(topBound, UIApplication.sharedApplication.statusBarFrame.size.height);
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

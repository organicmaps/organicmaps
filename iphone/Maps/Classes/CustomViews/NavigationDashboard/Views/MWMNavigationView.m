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

@end

@implementation MWMNavigationView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.defaultHeight = self.height;
}

- (void)addToView:(UIView *)superview
{
  [superview addSubview:self];
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
   }
                   completion:^(BOOL finished)
   {
     if (!self.isVisible)
       [self removeFromSuperview];
   }];
  [super layoutSubviews];
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  return CGRectMake(0.0, self.topOffset - (self.isVisible ? 0.0 : self.defaultHeight), self.superview.width, self.defaultHeight);
}

- (void)setTopOffset:(CGFloat)topOffset
{
  _topOffset = topOffset;
  [self layoutSubviews];
}

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  [self layoutSubviews];
}

@end

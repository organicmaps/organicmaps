//
//  MWMRoutePreview.m
//  Maps
//
//  Created by Ilya Grechuhin on 21.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMRoutePreview.h"
#import "UIKitCategories.h"

@interface MWMRoutePreview ()

@property (nonatomic) CGFloat goButtonHiddenOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonVerticalOffset;

@property (nonatomic) BOOL shown;

@end

@implementation MWMRoutePreview

- (void)awakeFromNib
{
  self.goButtonHiddenOffset = self.goButtonVerticalOffset.constant;
}

- (void)addToView:(UIView *)superview
{
  self.frame = self.defaultFrame;
  [superview addSubview:self];
  self.shown = YES;
}

- (void)remove
{
  self.shown = NO;
}

- (void)layoutSubviews
{
  [UIView animateWithDuration:0.2 animations:^
  {
    self.frame = self.defaultFrame;
  }
  completion:^(BOOL finished)
  {
    if (!self.shown)
      [self removeFromSuperview];
  }];
  [super layoutSubviews];
}

- (IBAction)routeTypePressed:(UIButton *)sender
{
  self.pedestrian.selected = self.vehicle.selected = NO;
  sender.selected = YES;
}

#pragma mark - Properties

- (CGRect)defaultFrame
{
  return CGRectMake(0.0, self.topOffset - (self.shown ? 0.0 : self.height), self.superview.width, self.height);
}

- (void)setTopOffset:(CGFloat)topOffset
{
  _topOffset = topOffset;
  [self layoutSubviews];
}

- (void)setShowGoButton:(BOOL)showGoButton
{
  _showGoButton = showGoButton;
  [self layoutIfNeeded];
  self.goButtonVerticalOffset.constant = showGoButton ? 0.0 : self.goButtonHiddenOffset;
  [UIView animateWithDuration:0.2 animations:^{ [self layoutIfNeeded]; }];
}

- (void)setShown:(BOOL)shown
{
  _shown = shown;
  [self layoutSubviews];
}

@end

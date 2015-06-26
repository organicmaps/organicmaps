//
//  MWMZoomButtonsView.m
//  Maps
//
//  Created by Ilya Grechuhin on 12.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMZoomButtonsView.h"
#import "MWMMapViewControlsCommon.h"
#import "UIKitCategories.h"

static CGFloat const kZoomViewOffsetToTopBound = 12.0;
static CGFloat const kZoomViewOffsetToBottomBound = 40.0;
static CGFloat const kZoomViewOffsetToFrameBound = 294.0;
static CGFloat const kZoomViewHideBoundPercent = 0.4;

@interface MWMZoomButtonsView()

@property (nonatomic) CGRect defaultBounds;

@end

@implementation MWMZoomButtonsView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.defaultBounds = self.bounds;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.topBound = 0.0;
    self.bottomBound = 0.0;
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.bounds = self.defaultBounds;
  [self layoutXPosition:self.hidden];
  [self layoutYPosition];
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (hidden)
    self.minX = self.superview.width;
  else
    self.maxX = self.superview.width - kViewControlsOffsetToBounds;
}

- (void)layoutYPosition
{
  self.maxY = self.superview.height - kZoomViewOffsetToFrameBound;
  if (self.bottomBound > 0.0)
    self.maxY = MIN(self.maxY, self.bottomBound - kZoomViewOffsetToBottomBound);

  self.minY = MAX(self.minY, self.topBound + kZoomViewOffsetToTopBound);
}

- (void)moveAnimated
{
  if (self.hidden)
    return;
  [UIView animateWithDuration:framesDuration(kMenuViewMoveFramesCount) animations:^
  {
    [self layoutYPosition];
  }];
}

- (void)fadeAnimatedIn:(BOOL)show
{
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
  {
    self.alpha = show ? 1.0 : 0.0;
  }];
}

#pragma mark - Properties

- (void)setHidden:(BOOL)hidden animated:(BOOL)animated
{
  if (animated) {
    if (self.hidden == hidden)
      return;
    if (!hidden)
      self.hidden = NO;
    [self layoutXPosition:!hidden];
    [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
    {
      [self layoutXPosition:hidden];
    }
    completion:^(BOOL finished)
    {
      if (hidden)
        self.hidden = YES;
    }];
  }
  else
  {
    self.hidden = hidden;
  }
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = topBound;
  [self moveAnimated];
}

- (void)setBottomBound:(CGFloat)bottomBound
{
  CGFloat const hideBound = kZoomViewHideBoundPercent * self.superview.height;
  BOOL const isHidden = _bottomBound < hideBound;
  BOOL const willHide = bottomBound < hideBound;
  _bottomBound = bottomBound;
  
  if (willHide)
  {
    if (!isHidden)
      [self fadeAnimatedIn:NO];
  }
  else
  {
    [self moveAnimated];
    if (isHidden)
      [self fadeAnimatedIn:YES];
  }
}

@end

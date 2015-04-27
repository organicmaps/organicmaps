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
static CGFloat const kZoomViewOffsetToBottomBound = 294.0;

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
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.bounds = self.defaultBounds;
  [self layoutXPosition:self.hidden];
  self.maxY = self.superview.height - kZoomViewOffsetToBottomBound;
  self.minY = MAX(self.minY, kZoomViewOffsetToTopBound);
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (hidden)
    self.minX = self.superview.width;
  else
    self.maxX = self.superview.width - kViewControlsOffsetToBounds;
}

#pragma mark - Properties

- (void)setHidden:(BOOL)hidden
{
  if (!hidden)
    super.hidden = NO;
  [self layoutXPosition:!hidden];
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
  {
    [self layoutXPosition:hidden];
  }
  completion:^(BOOL finished)
  {
    if (hidden)
      super.hidden = YES;
  }];
}

@end

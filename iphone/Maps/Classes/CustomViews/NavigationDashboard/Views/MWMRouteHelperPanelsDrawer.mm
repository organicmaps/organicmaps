//
//  MWMRouteHelperPanelsDrawer.m
//  Maps
//
//  Created by v.mikhaylenko on 08.09.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLanesPanel.h"
#import "MWMNextTurnPanel.h"
#import "MWMRouteHelperPanelsDrawer.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"

static CGFloat const kTopOffsetValuePortrait = 160.;
static CGFloat const kTopOffsetValueLandscape = 116.;
static CGFloat const kBottomOffsetValuePortrait = 208.;
static CGFloat const kBottomOffsetValueLandscape = 164.;

@interface MWMRouteHelperPanelsDrawer ()

@property (nonatomic) UIView * divider;

@end

@implementation MWMRouteHelperPanelsDrawer

- (instancetype)initWithView:(UIView *)view
{
  self = [super init];
  if (self)
    _parentView = view;
  return self;
}

- (void)invalidateTopBounds:(vector<MWMRouteHelperPanel *> const &)panels forOrientation:(UIInterfaceOrientation)orientation
{
  if (IPAD || panels.empty())
    return;
  if (UIInterfaceOrientationIsPortrait(orientation))
    panels.front().topBound = kTopOffsetValuePortrait;
  else
    panels.front().topBound = kTopOffsetValueLandscape;
}

- (void)drawPanels:(vector<MWMRouteHelperPanel *> const &)panels
{
  if (panels.empty())
    return;
  if (IPAD)
    [self drawForiPad:panels];
  else
    [self drawForiPhone:panels];
}

- (void)drawForiPad:(vector<MWMRouteHelperPanel *> const &)panels
{
  if (panels.empty())
  {
    [self removeDivider];
    return;
  }
  if (panels.size() == 1)
  {
    panels.front().topBound = self.parentView.height;
    [self removeDivider];
    return;
  }
  if (panels.size() == 2)
  {
    MWMRouteHelperPanel * first;
    MWMRouteHelperPanel * second;
    for (auto p : panels)
    {
      if ([p isKindOfClass:[MWMLanesPanel class]])
        first = p;
      else
        second = p;
    }
    first.topBound = self.parentView.height;
    second.topBound = first.maxY;
    [self drawDivider:first];
    return;
  }
  NSAssert(false, @"Incorrect vector size");
}

- (void)drawForiPhone:(vector<MWMRouteHelperPanel *> const &)panels
{
  if (panels.empty())
    return;

  CGSize const s = self.parentView.size;
  BOOL const isPortrait = s.height > s.width;
  if (panels.size() == 1)
  {
    panels.front().topBound = isPortrait ? kTopOffsetValuePortrait : kTopOffsetValueLandscape;
    return;
  }
  if (panels.size() == 2)
  {
    MWMRouteHelperPanel * first;
    MWMRouteHelperPanel * second;
    for (auto p : panels)
    {
      if ([p isKindOfClass:[MWMLanesPanel class]])
        first = p;
      else
        second = p;
    }
    first.topBound = isPortrait ? kTopOffsetValuePortrait : kTopOffsetValuePortrait;
    second.topBound = isPortrait ? kBottomOffsetValuePortrait : kBottomOffsetValueLandscape;
    return;
  }
  NSAssert(false, @"Incorrect vector size");
}

- (void)drawDivider:(UIView *)src
{
  if ([src.subviews containsObject:self.divider])
    return;
  self.divider.minY = src.height;
  [src addSubview:self.divider];
}

- (void)removeDivider
{
  if (self.divider.superview)
    [self.divider removeFromSuperview];
}

- (UIView *)divider
{
  if (!_divider)
  {
    _divider = [[UIView alloc] initWithFrame:{{}, {360., 1.}}];
    _divider.backgroundColor = UIColor.whiteSecondaryText;
  }
  return _divider;
}

@end

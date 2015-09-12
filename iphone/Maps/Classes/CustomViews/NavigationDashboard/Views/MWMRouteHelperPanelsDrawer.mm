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

- (void)invalidateTopBounds:(NSArray *)panels forOrientation:(UIInterfaceOrientation)orientation
{
  if (IPAD || !panels.count)
    return;
  [(MWMRouteHelperPanel *)panels.firstObject setTopBound:UIInterfaceOrientationIsPortrait(orientation) ? kTopOffsetValuePortrait : kTopOffsetValueLandscape];
}

- (void)drawPanels:(NSArray *)panels
{
  if (IPAD)
    [self drawForiPad:panels];
  else
    [self drawForiPhone:panels];
}

- (void)drawForiPad:(NSArray *)panels
{
  switch (panels.count)
  {
    case 0:
      [self removeDivider];
      return;
    case 1:
      [(MWMRouteHelperPanel *)panels.firstObject setTopBound:self.parentView.height];
      [self removeDivider];
      return;
    case 2:
    {
      MWMRouteHelperPanel * first;
      MWMRouteHelperPanel * second;
      for (MWMRouteHelperPanel * p in panels)
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
    default:
      NSAssert(false, @"Incorrect vector size");
      return;
  }
}

- (void)drawForiPhone:(NSArray *)panels
{
  CGSize const s = self.parentView.size;
  BOOL const isPortrait = s.height > s.width;

  switch (panels.count)
  {
    case 0:
      return;
    case 1:
      [(MWMRouteHelperPanel *)panels.firstObject setTopBound:isPortrait ? kTopOffsetValuePortrait : kTopOffsetValueLandscape ];
      return;
    case 2:
    {
      MWMRouteHelperPanel * first;
      MWMRouteHelperPanel * second;
      for (MWMRouteHelperPanel * p in panels)
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
    default:
      NSAssert(false, @"Incorrect vector size");
      return;
  }
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

#import "MWMLanesPanel.h"
#import "MWMNextTurnPanel.h"
#import "MWMRouteHelperPanelsDrawer.h"
#import "UIColor+MapsMeColor.h"

static CGFloat const kOffsetBetweenPanels = 8.;

@interface MWMRouteHelperPanelsDrawer ()

@property (nonatomic) UIView * divider;
@property (weak, nonatomic, readwrite) UIView * topView;

@end

@implementation MWMRouteHelperPanelsDrawer

- (instancetype)initWithTopView:(UIView *)view
{
  self = [super init];
  if (self)
    self.topView = view;
  return self;
}

- (void)invalidateTopBounds:(NSArray *)panels topView:(UIView *)view
{
  if (IPAD || !panels.count)
    return;
  self.topView = view;
  dispatch_async(dispatch_get_main_queue(), ^
  {
    [self.topView layoutIfNeeded];
    [(MWMRouteHelperPanel *)panels.firstObject setTopBound:self.topView.maxY + kOffsetBetweenPanels];
    [self drawPanels:panels];
  });
}

- (void)drawPanels:(NSArray *)panels
{
  switch (panels.count)
  {
    case 0:
      [self removeDivider];
      return;
    case 1:
      [(MWMRouteHelperPanel *)panels.firstObject setTopBound:self.firstPanelTopOffset];
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
      first.topBound = self.firstPanelTopOffset;
      second.topBound = first.maxY + (IPAD ? 0. : kOffsetBetweenPanels);
      if (IPAD)
        [self drawDivider:first];
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

- (CGFloat)firstPanelTopOffset
{
  return IPAD ? self.topView.height : self.topView.maxY + kOffsetBetweenPanels;
}

@end

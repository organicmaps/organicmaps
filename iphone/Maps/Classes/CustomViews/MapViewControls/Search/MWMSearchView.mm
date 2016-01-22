#import "Common.h"
#import "MWMSearchTabButtonsView.h"
#import "MWMSearchView.h"

static CGFloat const kWidthForiPad = 320.0;

@interface MWMSearchView ()

@property (weak, nonatomic) IBOutlet UIView * searchBar;

@property (weak, nonatomic) IBOutlet UIView * infoWrapper;

@property (weak, nonatomic) IBOutlet UIView * tabBar;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tabBarTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tabBarHeight;

@property (nonatomic) CGFloat correctMinX;

@end

@implementation MWMSearchView

- (void)awakeFromNib
{
  self.autoresizingMask = UIViewAutoresizingFlexibleHeight;
  self.correctMinX = -kWidthForiPad;
  CALayer * sl = self.layer;
  CALayer * sbl = self.searchBar.layer;
  CALayer * tbl = self.tabBar.layer;
  CGFloat const scale = UIScreen.mainScreen.scale;
  sl.shouldRasterize = sbl.shouldRasterize = tbl.shouldRasterize = YES;
  sl.rasterizationScale = sbl.rasterizationScale = tbl.rasterizationScale = scale;
}

- (void)refresh
{
  [self.searchBar refresh];
  [self.infoWrapper refresh];
}

- (void)setFrame:(CGRect)frame
{
  BOOL const equal = CGRectEqualToRect(super.frame, frame);
  super.frame = frame;
  if (!equal && self.superview && self.isVisible && (IPAD || self.compact))
    [self.delegate searchFrameUpdated:frame];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (IPAD)
  {
    self.frame = {{self.correctMinX, 0.0}, {kWidthForiPad, self.superview.height}};
  }
  else
  {
    self.frame = self.superview.bounds;
    if (self.compact)
      self.height = self.searchBar.minY + self.searchBar.height;
  }
  if (self.tabBarIsVisible)
    self.tabBar.hidden = NO;
  CGFloat const tabBarHeight = self.height > self.width ? 64.0 : 44.0;
  self.tabBarHeight.constant = tabBarHeight;
  self.tabBarTopOffset.constant = self.tabBarIsVisible ? 0.0 : -tabBarHeight;
  self.searchBar.layer.shadowRadius = self.tabBarIsVisible ? 0.0 : 2.0;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    [self.tabBar.subviews enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL *stop)
    {
      [btn setNeedsLayout];
    }];
    if (!self.tabBarIsVisible)
      self.tabBar.hidden = YES;
    [self.tabBar layoutIfNeeded];
  }];
  [super layoutSubviews];
}

#pragma mark - Properties

- (void)setIsVisible:(BOOL)isVisible
{
  if (_isVisible == isVisible)
    return;
  _isVisible = isVisible;
  self.minY = 0.0;
  self.height = self.superview.height;
  if (IPAD)
  {
    if (isVisible)
      self.hidden = NO;
    self.correctMinX = self.minX = isVisible ? -kWidthForiPad : 0.0;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.correctMinX = self.minX = isVisible ? 0.0: -kWidthForiPad;
    }
    completion:^(BOOL finished)
    {
      if (!isVisible)
      {
        self.hidden = YES;
        [self removeFromSuperview];
      }
    }];
  }
  else
  {
    self.hidden = !isVisible;
    if (!isVisible)
      [self removeFromSuperview];
  }
  [self setNeedsLayout];
}

- (void)setTabBarIsVisible:(BOOL)tabBarIsVisible
{
  if (_tabBarIsVisible == tabBarIsVisible)
    return;
  _tabBarIsVisible = tabBarIsVisible;
  [self setNeedsLayout];
}

- (void)setCompact:(BOOL)compact
{
  if (IPAD)
    return;
  _compact = compact;
  if (!compact)
    self.infoWrapper.hidden = NO;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.infoWrapper.alpha = compact ? 0.0 : 1.0;
  }
  completion:^(BOOL finished)
  {
    if (compact)
      self.infoWrapper.hidden = YES;
    self.autoresizingMask = compact ? UIViewAutoresizingFlexibleWidth : UIViewAutoresizingFlexibleHeight;
    [self setNeedsLayout];
  }];
}

@end

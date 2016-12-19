#import "MWMSideButtonsView.h"
#import "MWMCommon.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"

#include "base/math.hpp"

namespace
{
CGFloat const kLocationButtonSpacingMax = 52;
CGFloat const kLocationButtonSpacingMin = 8;
CGFloat const kButtonsTopOffset = 82;
CGFloat const kButtonsBottomOffset = 6;
}  // namespace

@interface MWMSideButtonsView ()

@property(weak, nonatomic) IBOutlet MWMButton * zoomIn;
@property(weak, nonatomic) IBOutlet MWMButton * zoomOut;
@property(weak, nonatomic) IBOutlet MWMButton * location;

@end

@implementation MWMSideButtonsView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)layoutSubviews
{
  CGFloat spacing = self.bottomBound - self.topBound - self.zoomOut.maxY - self.location.height;
  spacing = my::clamp(spacing, kLocationButtonSpacingMin, kLocationButtonSpacingMax);

  self.location.minY = self.zoomOut.maxY + spacing;
  self.bounds = {{}, {self.zoomOut.width, self.location.maxY}};
  if (self.zoomHidden)
    self.height = self.location.height;
  self.location.maxY = self.height;

  [self layoutXPosition:self.hidden];
  [self layoutYPosition];
  [super layoutSubviews];
}

- (void)layoutXPosition:(BOOL)hidden
{
  if ([UIApplication sharedApplication].userInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft)
  {
    if (hidden)
      self.maxX = 0;
    else
      self.minX = kViewControlsOffsetToBounds;
  }
  else
  {
    if (hidden)
      self.minX = self.superview.width;
    else
      self.maxX = self.superview.width - kViewControlsOffsetToBounds;
  }
}

- (void)layoutYPosition
{
  CGFloat const centerShift = (self.height - self.zoomIn.midY - self.zoomOut.midY) / 2;
  self.midY = centerShift + self.superview.height / 2;
  if (self.maxY > self.bottomBound)
    self.maxY = self.bottomBound;
}
- (void)fadeZoomButtonsShow:(BOOL)show
{
  CGFloat const alpha = show ? 1.0 : 0.0;
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount)
                   animations:^{
                     self.zoomIn.alpha = alpha;
                     self.zoomOut.alpha = alpha;
                   }];
}

- (void)fadeLocationButtonShow:(BOOL)show
{
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount)
                   animations:^{
                     self.location.alpha = show ? 1.0 : 0.0;
                   }];
}

- (void)animate
{
  runAsyncOnMainQueue(^{
    [self layoutYPosition];

    CGFloat const spaceLeft = self.bottomBound - self.topBound -
                              (equalScreenDimensions(self.topBound, 0.0) ? statusBarHeight() : 0.0);
    BOOL const isZoomHidden = self.zoomIn.alpha == 0.0;
    BOOL const willZoomHide = (self.location.maxY > spaceLeft);
    if (willZoomHide)
    {
      if (!isZoomHidden)
        [self fadeZoomButtonsShow:NO];
    }
    else
    {
      if (isZoomHidden)
        [self fadeZoomButtonsShow:YES];
    }
    BOOL const isLocationHidden = self.location.alpha == 0.0;
    BOOL const willLocationHide = (self.location.height > spaceLeft);
    if (willLocationHide)
    {
      if (!isLocationHidden)
        [self fadeLocationButtonShow:NO];
    }
    else
    {
      if (isLocationHidden)
        [self fadeLocationButtonShow:YES];
    }
  });
}

#pragma mark - Properties

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  CGFloat const minX = zoomHidden ? self.width + kViewControlsOffsetToBounds : 0.0;
  self.zoomIn.minX = minX;
  self.zoomOut.minX = minX;
  [self setNeedsLayout];
}

- (void)setHidden:(BOOL)hidden animated:(BOOL)animated
{
  if (animated)
  {
    if (self.hidden == hidden)
      return;
    if (!hidden)
      self.hidden = NO;
    [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount)
        animations:^{
          [self layoutXPosition:hidden];
        }
        completion:^(BOOL finished) {
          if (hidden)
            self.hidden = YES;
        }];
  }
  else
  {
    self.hidden = hidden;
  }
}

@synthesize topBound = _topBound;

- (void)setTopBound:(CGFloat)topBound
{
  if (equalScreenDimensions(_topBound, topBound))
    return;
  _topBound = topBound;
  [self animate];
}

- (CGFloat)topBound { return MAX(kButtonsTopOffset, _topBound); }
@synthesize bottomBound = _bottomBound;

- (void)setBottomBound:(CGFloat)bottomBound
{
  if (equalScreenDimensions(_bottomBound, bottomBound))
    return;
  _bottomBound = bottomBound;
  [self animate];
}

- (CGFloat)bottomBound
{
  if (!self.superview)
    return _bottomBound;
  CGFloat const spaceOccupiedByMenu =
      self.superview.height - [MWMBottomMenuViewController controller].mainStateHeight;
  return MIN(spaceOccupiedByMenu, _bottomBound) - kButtonsBottomOffset;
}

@end

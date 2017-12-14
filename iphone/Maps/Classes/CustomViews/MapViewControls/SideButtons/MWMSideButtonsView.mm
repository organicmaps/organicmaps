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
CGFloat const kButtonsTopOffset = 6;
CGFloat const kButtonsBottomOffset = 6;
}  // namespace

@interface MWMSideButtonsView ()

@property(weak, nonatomic) IBOutlet MWMButton * zoomIn;
@property(weak, nonatomic) IBOutlet MWMButton * zoomOut;
@property(weak, nonatomic) IBOutlet MWMButton * location;

@property(nonatomic) CGRect availableArea;

@end

@implementation MWMSideButtonsView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)layoutSubviews
{
  CGFloat spacing = self.availableHeight - self.zoomOut.maxY - self.location.height;
  spacing = my::clamp(spacing, kLocationButtonSpacingMin, kLocationButtonSpacingMax);

  self.location.minY = self.zoomOut.maxY + spacing;
  self.bounds = {{}, {self.zoomOut.width, self.location.maxY}};
  if (self.zoomHidden)
    self.height = self.location.height;
  self.location.maxY = self.height;

  [self layoutXPosition:self.hidden];
  [self animate];
  [super layoutSubviews];
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (UIApplication.sharedApplication.userInterfaceLayoutDirection ==
      UIUserInterfaceLayoutDirectionRightToLeft)
  {
    if (hidden)
      self.maxX = 0;
    else
      self.minX = self.availableArea.origin.x + kViewControlsOffsetToBounds;
  }
  else
  {
    if (hidden)
      self.minX = self.superview.width;
    else
      self.maxX = self.availableArea.origin.x + self.availableArea.size.width - kViewControlsOffsetToBounds;
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
  dispatch_async(dispatch_get_main_queue(), ^{
    [self layoutYPosition];

    auto const spaceLeft = self.availableHeight;
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
    [self layoutIfNeeded];
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

- (void)updateAvailableArea:(CGRect)frame
{
  if (CGRectEqualToRect(self.availableArea, frame))
    return;
  self.availableArea = frame;
  [self setNeedsLayout];
}

- (CGFloat)availableHeight
{
  return self.availableArea.size.height - kButtonsTopOffset - kButtonsBottomOffset;
}

- (CGFloat)topBound { return self.availableArea.origin.y + kButtonsTopOffset; }
- (CGFloat)bottomBound
{
  auto const area = self.availableArea;
  return area.origin.y + area.size.height - kButtonsBottomOffset;
}

@end

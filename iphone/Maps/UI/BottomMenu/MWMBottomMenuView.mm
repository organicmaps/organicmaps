#import "MWMBottomMenuView.h"
#import "MWMAvailableAreaAffectDirection.h"
#import "MWMButton.h"
#import "MWMCommon.h"

namespace
{
CGFloat constexpr kAdditionalHeight = 64;
CGFloat constexpr kDefaultMainButtonsHeight = 48;
}  // namespace

@interface MWMBottomMenuView ()

@property(nonatomic) CGFloat layoutDuration;
@property(nonatomic) CGRect availableArea;
@property(weak, nonatomic) IBOutlet MWMButton * menuButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * additionalButtonsHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * mainButtonsHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * separatorHeight;
@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;
@property(weak, nonatomic) IBOutlet UIView * downloadBadge;
@property(weak, nonatomic) IBOutlet UIView * extraBottomView;

@end

@implementation MWMBottomMenuView

- (void)didMoveToSuperview
{
  [super didMoveToSuperview];
  self.minY = self.superview.height;
  self.width = self.superview.width;
}

- (void)layoutSubviews
{
  if (self.layoutDuration > 0.0)
  {
    CGFloat const duration = self.layoutDuration;
    self.layoutDuration = 0.0;
    if (self.state != MWMBottomMenuStateHidden)
      self.hidden = NO;
    [self setNeedsLayout];
    [UIView animateWithDuration:duration
        animations:^{
          [self updateAppearance];
          [self layoutIfNeeded];
        }
        completion:^(BOOL finished) {
          if (self.state == MWMBottomMenuStateHidden)
            self.hidden = YES;
        }];
  }
  [super layoutSubviews];
}

- (void)updateAppearance
{
  [self updateAlphaAndColor];
  [self updateGeometry];
}

- (void)updateAlphaAndColor
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: break;
  case MWMBottomMenuStateInactive:
    self.extraBottomView.backgroundColor = [UIColor menuBackground];
    self.downloadBadge.alpha = [self isCompact] ? 0.0 : 1.0;
    self.additionalButtons.alpha = 0.0;
    break;
  case MWMBottomMenuStateActive:
    self.extraBottomView.backgroundColor = [UIColor white];
    self.downloadBadge.alpha = 0.0;
    self.additionalButtons.alpha = 1.0;
    break;
  }
}

- (void)updateGeometry
{
  auto const availableArea = self.availableArea;
  if (CGRectEqualToRect(availableArea, CGRectZero))
    return;
  self.separatorHeight.constant = 0.0;
  self.additionalButtonsHeight.constant = 0.0;
  self.mainButtonsHeight.constant = kDefaultMainButtonsHeight;
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: self.minY = self.superview.height; return;
  case MWMBottomMenuStateInactive: break;
  case MWMBottomMenuStateActive:
  {
    self.separatorHeight.constant = 1.0;
    BOOL const isLandscape = self.width > self.layoutThreshold;
    if (isLandscape)
    {
      self.additionalButtonsHeight.constant = kAdditionalHeight;
    }
    else
    {
      NSUInteger const additionalButtonsCount = [self.additionalButtons numberOfItemsInSection:0];
      CGFloat const buttonHeight = 52.0;
      self.additionalButtonsHeight.constant = additionalButtonsCount * buttonHeight;
    }
  }
  break;
  }
  auto const mainHeight = self.mainButtonsHeight.constant;
  auto const separatorHeight = self.separatorHeight.constant;
  auto const additionalHeight = self.additionalButtonsHeight.constant;
  auto const height = mainHeight + separatorHeight + additionalHeight;
  self.frame = {{availableArea.origin.x, availableArea.size.height - height},
                {availableArea.size.width, height}};
}

- (void)morphMenuButtonTemplate:(NSString *)morphTemplate direct:(BOOL)direct
{
  UIButton * btn = self.menuButton;
  NSUInteger const morphImagesCount = 6;
  NSUInteger const startValue = direct ? 1 : morphImagesCount;
  NSUInteger const endValue = direct ? morphImagesCount + 1 : 0;
  NSInteger const stepValue = direct ? 1 : -1;
  NSMutableArray * morphImages = [NSMutableArray arrayWithCapacity:morphImagesCount];
  for (NSUInteger i = startValue, j = 0; i != endValue; i += stepValue, j++)
  {
    UIImage * image =
        [UIImage imageNamed:[NSString stringWithFormat:@"%@%@_%@", morphTemplate, @(i).stringValue,
                                                       [UIColor isNightMode] ? @"dark" : @"light"]];
    morphImages[j] = image;
  }
  btn.imageView.animationImages = morphImages;
  btn.imageView.animationRepeatCount = 1;
  btn.imageView.image = morphImages.lastObject;
  [btn.imageView startAnimating];
  [self refreshMenuButtonState];
}

- (void)refreshMenuButtonState
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.menuButton.imageView.isAnimating)
    {
      [self refreshMenuButtonState];
    }
    else
    {
      NSString * name = nil;
      switch (self.state)
      {
      case MWMBottomMenuStateHidden: name = @"ic_menu"; break;
      case MWMBottomMenuStateInactive:
        name = [self isCompact] ? @"ic_menu_left" : @"ic_menu";
        break;
      case MWMBottomMenuStateActive: name = @"ic_menu_down"; break;
      }
      [self.menuButton setImage:[UIImage imageNamed:name] forState:UIControlStateNormal];
    }
  });
}

- (void)refreshLayout
{
  self.layoutDuration = kDefaultAnimationDuration;
  [self setNeedsLayout];
}

#pragma mark - Properties

- (void)setState:(MWMBottomMenuState)state
{
  if (_state == state)
    return;
  BOOL const isActive = (state == MWMBottomMenuStateActive);
  self.additionalButtons.hidden = !isActive;
  if (_state == MWMBottomMenuStateActive || isActive)
    [self morphMenuButtonTemplate:@"ic_menu_" direct:isActive];
  _state = state;
  [self refreshLayout];
}

- (void)updateAvailableArea:(CGRect)frame
{
  if (CGRectEqualToRect(self.availableArea, frame))
    return;
  BOOL const wasCompact = [self isCompact];
  self.availableArea = frame;
  BOOL const isCompact = [self isCompact];
  if (wasCompact || isCompact)
    [self morphMenuButtonTemplate:@"ic_menu_rotate_" direct:isCompact];
  [self refreshLayout];
}

- (BOOL)isCompact { return self.availableArea.origin.x > 0; }
#pragma mark - AvailableArea / PlacePageArea

- (MWMAvailableAreaAffectDirections)placePageAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsBottom : MWMAvailableAreaAffectDirectionsNone;
}

#pragma mark - AvailableArea / WidgetsArea

- (MWMAvailableAreaAffectDirections)widgetsAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsBottom;
}

#pragma mark - AvailableArea / SideButtonsArea

- (MWMAvailableAreaAffectDirections)sideButtonsAreaAffectDirections
{
  return MWMAvailableAreaAffectDirectionsBottom;
}

@end

#import "MWMBottomMenuView.h"
#import "Common.h"
#import "EAGLView.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MapsAppDelegate.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIView+RuntimeAttributes.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"

namespace
{
CGFloat constexpr kAdditionalHeight = 64;
CGFloat constexpr kDefaultMainButtonsHeight = 48;
CGFloat constexpr kDefaultMenuButtonWidth = 60;
CGFloat constexpr kRoutingAdditionalButtonsOffsetCompact = 0;
CGFloat constexpr kRoutingAdditionalButtonsOffsetRegular = 48;
CGFloat constexpr kRoutingMainButtonsHeightCompact = 52;
CGFloat constexpr kRoutingMainButtonsHeightRegular = 56;
CGFloat constexpr kRoutingMainButtonsHeightLandscape = 40;
CGFloat constexpr kRoutingMenuButtonWidthCompact = 40;
CGFloat constexpr kRoutingMenuButtonWidthRegular = 56;
CGFloat constexpr kSpeedDistanceOffsetCompact = 0;
CGFloat constexpr kSpeedDistanceOffsetRegular = 16;
CGFloat constexpr kSpeedDistanceWidthCompact = 72;
CGFloat constexpr kSpeedDistanceWidthLandscape = 128;
CGFloat constexpr kSpeedDistanceWidthRegular = 88;
CGFloat constexpr kPageControlTopOffsetRegular = 0;
CGFloat constexpr kPageControlTopOffsetLandscape = -8;
CGFloat constexpr kPageControlScaleLandscape = 0.7;
CGFloat constexpr kThresholdCompact = 321;
CGFloat constexpr kThresholdRegular = 415;
CGFloat constexpr kTimeWidthCompact = 112;
CGFloat constexpr kTimeWidthRegular = 128;
}  // namespace

@interface MWMBottomMenuView ()

@property(weak, nonatomic) IBOutlet UIView * mainButtons;
@property(weak, nonatomic) IBOutlet UIView * separator;
@property(weak, nonatomic) IBOutlet UICollectionView * additionalButtons;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * mainButtonsHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * additionalButtonsHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * separatorHeight;

@property(weak, nonatomic) IBOutlet UIView * downloadBadge;

@property(weak, nonatomic) IBOutlet MWMButton * p2pButton;
@property(weak, nonatomic) IBOutlet MWMButton * searchButton;
@property(weak, nonatomic) IBOutlet MWMButton * bookmarksButton;
@property(weak, nonatomic) IBOutlet MWMButton * menuButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * menuButtonWidth;

@property(weak, nonatomic) IBOutlet UIView * routingView;
@property(weak, nonatomic) IBOutlet UIButton * goButton;
@property(weak, nonatomic) IBOutlet UIButton * toggleInfoButton;
@property(weak, nonatomic) IBOutlet UIView * speedView;
@property(weak, nonatomic) IBOutlet UIView * timeView;
@property(weak, nonatomic) IBOutlet UIView * distanceView;
@property(weak, nonatomic) IBOutlet UIView * progressView;
@property(weak, nonatomic) IBOutlet UIView * routingAdditionalView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * routingAdditionalViewHeight;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint)
    NSArray * routingAdditionalButtonsOffset;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * speedDistanceOffset;
@property(weak, nonatomic) IBOutlet UILabel * speedLabel;
@property(weak, nonatomic) IBOutlet UILabel * timeLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * speedWithLegendLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceWithLegendLabel;

@property(weak, nonatomic) IBOutlet UIPageControl * pageControl;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * pageControlTopOffset;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * speedDistanceWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * timeWidth;

@property(nonatomic) CGFloat layoutDuration;

@property(weak, nonatomic) IBOutlet MWMBottomMenuViewController * owner;

@end

@implementation MWMBottomMenuView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.additionalButtons.hidden = YES;
  self.downloadBadge.hidden = YES;
  self.goButton.hidden = YES;
  self.toggleInfoButton.hidden = YES;
  self.speedView.hidden = YES;
  self.timeView.hidden = YES;
  self.distanceView.hidden = YES;
  self.progressView.hidden = YES;
  self.routingView.hidden = YES;
  self.routingAdditionalView.hidden = YES;
  self.restoreState = MWMBottomMenuStateInactive;
  [self.goButton setBackgroundColor:[UIColor linkBlue] forState:UIControlStateNormal];
  [self.goButton setBackgroundColor:[UIColor linkBlueHighlighted]
                           forState:UIControlStateHighlighted];
}

- (void)layoutSubviews
{
  [self refreshOnOrientationChange];
  if (self.layoutDuration > 0.0)
  {
    CGFloat const duration = self.layoutDuration;
    self.layoutDuration = 0.0;
    [self layoutIfNeeded];
    [UIView animateWithDuration:duration
                     animations:^{
                       [self layoutGeometry];
                       [self layoutIfNeeded];
                     }];
  }
  else
  {
    [self layoutGeometry];
  }
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        [self updateAlphaAndColor];
      }
      completion:^(BOOL finished) {
        [self updateVisibility];
      }];
  ((EAGLView *)self.superview).widgetsManager.bottomBound = self.mainButtonsHeight.constant;
  [self updateFonts];
  [super layoutSubviews];
}

- (void)updateAlphaAndColor
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: break;
  case MWMBottomMenuStateInactive:
    self.backgroundColor = [UIColor menuBackground];
    self.bookmarksButton.alpha = 1.0;
    self.downloadBadge.alpha = 1.0;
    self.routingView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    self.p2pButton.alpha = 1.0;
    self.searchButton.alpha = 1.0;
    break;
  case MWMBottomMenuStateActive:
    self.backgroundColor = [UIColor white];
    self.bookmarksButton.alpha = 1.0;
    self.downloadBadge.alpha = 0.0;
    self.routingView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    self.p2pButton.alpha = 1.0;
    self.searchButton.alpha = 1.0;
    break;
  case MWMBottomMenuStateCompact:
    self.backgroundColor = [UIColor menuBackground];
    if (!IPAD)
    {
      self.bookmarksButton.alpha = 0.0;
      self.p2pButton.alpha = 0.0;
      self.searchButton.alpha = 0.0;
    }
    self.downloadBadge.alpha = 0.0;
    self.routingView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    break;
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo:
    self.backgroundColor = [UIColor white];
    self.downloadBadge.alpha = 1.0;
    self.bookmarksButton.alpha = 0.0;
    self.routingView.alpha = 1.0;
    self.goButton.alpha = 1.0;
    self.speedView.alpha = 0.0;
    self.timeView.alpha = 0.0;
    self.distanceView.alpha = 0.0;
    self.progressView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    self.p2pButton.alpha = 0.0;
    self.searchButton.alpha = 0.0;
    break;
  case MWMBottomMenuStateRouting:
  case MWMBottomMenuStateRoutingExpanded:
    self.backgroundColor = [UIColor white];
    self.downloadBadge.alpha = 0.0;
    self.bookmarksButton.alpha = 0.0;
    self.routingView.alpha = 1.0;
    self.goButton.alpha = 0.0;
    self.speedView.alpha = 1.0;
    self.timeView.alpha = 1.0;
    self.distanceView.alpha = 1.0;
    self.progressView.alpha = 1.0;
    self.routingAdditionalView.alpha = 1.0;
    self.p2pButton.alpha = 0.0;
    self.searchButton.alpha = 0.0;
    break;
  }
}

- (void)updateFonts
{
  if (self.state != MWMBottomMenuStateRouting && self.state != MWMBottomMenuStateRoutingExpanded)
    return;
  UIFont * numberFont =
      (IPAD || self.width > kThresholdRegular) ? [UIFont bold24] : [UIFont bold22];
  UIFont * legendFont = [UIFont bold12];
  self.speedLabel.font = numberFont;
  self.timeLabel.font = numberFont;
  self.distanceLabel.font = numberFont;
  self.speedLegendLabel.font = legendFont;
  self.distanceLegendLabel.font = legendFont;
}

- (void)updateVisibility
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: break;
  case MWMBottomMenuStateInactive:
    self.additionalButtons.hidden = YES;
    self.routingView.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateActive:
    self.downloadBadge.hidden = YES;
    self.routingView.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateCompact:
    if (!IPAD)
    {
      self.bookmarksButton.hidden = YES;
      self.p2pButton.hidden = YES;
      self.searchButton.hidden = YES;
    }
    self.downloadBadge.hidden = YES;
    self.routingView.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo:
    self.bookmarksButton.hidden = YES;
    self.p2pButton.hidden = YES;
    self.searchButton.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateRouting:
  case MWMBottomMenuStateRoutingExpanded:
    self.bookmarksButton.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = NO;
    self.p2pButton.hidden = YES;
    self.searchButton.hidden = YES;
    break;
  }
}

- (void)layoutGeometry
{
  self.separatorHeight.constant = 0.0;
  self.additionalButtonsHeight.constant = 0.0;
  self.menuButtonWidth.constant = kDefaultMenuButtonWidth;
  self.mainButtonsHeight.constant = kDefaultMainButtonsHeight;
  self.routingAdditionalViewHeight.constant = 0.0;
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: self.minY = self.superview.height; return;
  case MWMBottomMenuStateInactive:
  case MWMBottomMenuStatePlanning:
  case MWMBottomMenuStateGo: break;
  case MWMBottomMenuStateCompact:
    if (self.restoreState == MWMBottomMenuStateRouting && !IPAD &&
        UIDeviceOrientationIsLandscape([UIDevice currentDevice].orientation))
      self.mainButtonsHeight.constant = kRoutingMainButtonsHeightLandscape;
    break;
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
  case MWMBottomMenuStateRouting: [self layoutRoutingGeometry]; break;
  case MWMBottomMenuStateRoutingExpanded:
    self.routingAdditionalViewHeight.constant = kAdditionalHeight;
    [self layoutRoutingGeometry];
    break;
  }
  CGFloat const width = MIN(self.superview.width - self.leftBound, self.superview.width);
  CGFloat const additionalHeight =
      MAX(self.additionalButtonsHeight.constant, self.routingAdditionalViewHeight.constant);
  CGFloat const height =
      self.mainButtonsHeight.constant + self.separatorHeight.constant + additionalHeight;
  self.frame = {{self.superview.width - width, self.superview.height - height}, {width, height}};
}

- (void)layoutRoutingGeometry
{
  auto layoutAdditionalButtonsOffset = ^(CGFloat offset) {
    for (NSLayoutConstraint * constraint in self.routingAdditionalButtonsOffset)
      constraint.constant = offset;
  };
  auto layoutSpeedDistanceOffset = ^(CGFloat offset) {
    for (NSLayoutConstraint * constraint in self.speedDistanceOffset)
      constraint.constant = offset;
  };
  self.pageControlTopOffset.constant = kPageControlTopOffsetRegular;
  self.pageControl.transform = CGAffineTransformIdentity;
  self.speedLabel.hidden = NO;
  self.distanceLabel.hidden = NO;
  self.speedLegendLabel.hidden = NO;
  self.distanceLegendLabel.hidden = NO;
  self.speedWithLegendLabel.hidden = YES;
  self.distanceWithLegendLabel.hidden = YES;
  if (IPAD)
  {
    self.speedDistanceWidth.constant = kSpeedDistanceWidthRegular;
    self.timeWidth.constant = kTimeWidthRegular;
    self.mainButtonsHeight.constant = kRoutingMainButtonsHeightRegular;
    self.menuButtonWidth.constant = kDefaultMenuButtonWidth;
    layoutAdditionalButtonsOffset(kRoutingAdditionalButtonsOffsetRegular);
    layoutSpeedDistanceOffset(kSpeedDistanceOffsetRegular);
  }
  else
  {
    self.timeWidth.constant = kTimeWidthRegular;
    self.mainButtonsHeight.constant = kRoutingMainButtonsHeightCompact;
    self.menuButtonWidth.constant = kRoutingMenuButtonWidthCompact;
    layoutAdditionalButtonsOffset(kRoutingAdditionalButtonsOffsetCompact);
    layoutSpeedDistanceOffset(kSpeedDistanceOffsetCompact);
    if (self.width <= kThresholdCompact)
    {
      self.speedDistanceWidth.constant = kSpeedDistanceWidthCompact;
      self.timeWidth.constant = kTimeWidthCompact;
    }
    else if (self.width <= kThresholdRegular)
    {
      self.speedDistanceWidth.constant = kSpeedDistanceWidthRegular;
    }
    else
    {
      self.pageControlTopOffset.constant = kPageControlTopOffsetLandscape;
      self.pageControl.transform =
          CGAffineTransformMakeScale(kPageControlScaleLandscape, kPageControlScaleLandscape);
      self.speedDistanceWidth.constant = kSpeedDistanceWidthLandscape;
      self.mainButtonsHeight.constant = kRoutingMainButtonsHeightLandscape;
      self.menuButtonWidth.constant = kRoutingMenuButtonWidthRegular;
      layoutAdditionalButtonsOffset(kRoutingAdditionalButtonsOffsetRegular);
      self.speedLabel.hidden = YES;
      self.distanceLabel.hidden = YES;
      self.speedLegendLabel.hidden = YES;
      self.distanceLegendLabel.hidden = YES;
      self.speedWithLegendLabel.hidden = NO;
      self.distanceWithLegendLabel.hidden = NO;
    }
  }
}

- (void)updateMenuButtonFromState:(MWMBottomMenuState)fromState toState:(MWMBottomMenuState)toState
{
  if (fromState == MWMBottomMenuStateActive || toState == MWMBottomMenuStateActive)
    [self morphMenuButtonTemplate:@"ic_menu_" toState:toState];
  else if (fromState == MWMBottomMenuStateCompact || toState == MWMBottomMenuStateCompact)
    [self morphMenuButtonTemplate:@"ic_menu_rotate_" toState:toState];
  [self refreshMenuButtonState];
}

- (void)morphMenuButtonTemplate:(NSString *)morphTemplate toState:(MWMBottomMenuState)toState
{
  BOOL const direct = toState == MWMBottomMenuStateActive || toState == MWMBottomMenuStateCompact;
  UIButton * btn = self.menuButton;
  NSUInteger const morphImagesCount = 6;
  NSUInteger const startValue = direct ? 1 : morphImagesCount;
  NSUInteger const endValue = direct ? morphImagesCount + 1 : 0;
  NSInteger const stepValue = direct ? 1 : -1;
  NSMutableArray * morphImages = [NSMutableArray arrayWithCapacity:morphImagesCount];
  for (NSUInteger i = startValue, j = 0; i != endValue; i += stepValue, j++)
    morphImages[j] =
        [UIImage imageNamed:[NSString stringWithFormat:@"%@%@_%@", morphTemplate, @(i).stringValue,
                                                       [UIColor isNightMode] ? @"dark" : @"light"]];
  btn.imageView.animationImages = morphImages;
  btn.imageView.animationRepeatCount = 1;
  btn.imageView.image = morphImages.lastObject;
  [btn.imageView startAnimating];
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
      UIButton * btn = self.menuButton;
      NSString * name = nil;
      switch (self.state)
      {
      case MWMBottomMenuStateHidden:
      case MWMBottomMenuStateInactive:
      case MWMBottomMenuStatePlanning:
      case MWMBottomMenuStateGo: name = @"ic_menu"; break;
      case MWMBottomMenuStateActive:
      case MWMBottomMenuStateRouting:
      case MWMBottomMenuStateRoutingExpanded: name = @"ic_menu_down"; break;
      case MWMBottomMenuStateCompact: name = @"ic_menu_left"; break;
      }
      UIImage * image = [UIImage imageNamed:name];
      if (isIOS7)
        image = [image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
      [btn setImage:image forState:UIControlStateNormal];
      if (self.state == MWMBottomMenuStateInactive)
      {
        btn.transform = CGAffineTransformIdentity;
      }
      else
      {
        BOOL const rotate = (self.state == MWMBottomMenuStateRouting);
        [UIView animateWithDuration:kDefaultAnimationDuration
                         animations:^{
                           btn.transform = rotate ? CGAffineTransformMakeRotation(M_PI)
                                                  : CGAffineTransformIdentity;
                         }];
      }
    }
  });
}

- (void)refreshOnOrientationChange
{
  [self refreshButtonsColor];
  if (IPAD || self.state != MWMBottomMenuStateCompact)
    return;
  BOOL const isPortrait = self.superview.width < self.superview.height;
  if (isPortrait)
    self.owner.leftBound = 0.0;
}

- (void)refreshLayout
{
  self.layoutDuration = kDefaultAnimationDuration;
  [self setNeedsLayout];
  [self refreshButtonsColor];
  if (self.state == MWMBottomMenuStateInactive)
    [self updateBadge];
}

- (void)refreshButtonsColor
{
  if (!isIOS7)
    return;
  auto const coloring = self.p2pButton.coloring;
  self.p2pButton.coloring = coloring;
  self.bookmarksButton.coloring = coloring;
  self.searchButton.coloring = self.searchButton.coloring;
}

- (void)updateBadge
{
  if (self.state == MWMBottomMenuStateRouting || self.state == MWMBottomMenuStateRoutingExpanded)
  {
    self.downloadBadge.hidden = YES;
    return;
  }
  auto & s = GetFramework().Storage();
  storage::Storage::UpdateInfo updateInfo{};
  s.GetUpdateInfo(s.GetRootId(), updateInfo);
  self.downloadBadge.hidden =
      (updateInfo.m_numberOfMwmFilesToUpdate == 0) && !platform::migrate::NeedMigrate();
}

#pragma mark - Properties

- (void)setFrame:(CGRect)frame
{
  frame.size.width = MAX(self.menuButtonWidth.constant, frame.size.width);
  super.frame = frame;
}

- (void)setState:(MWMBottomMenuState)state
{
  if (_state == state)
    return;
  [self refreshLayout];
  BOOL updateMenuButton = YES;
  switch (state)
  {
  case MWMBottomMenuStateHidden: updateMenuButton = NO; break;
  case MWMBottomMenuStateInactive:
  {
    if (MapsAppDelegate.theApp.routingPlaneMode == MWMRoutingPlaneModeNone)
      _leftBound = 0.0;
    [self updateBadge];
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = NO;
    self.layoutDuration =
        (_state == MWMBottomMenuStateCompact && !IPAD) ? 0.0 : kDefaultAnimationDuration;
    break;
  }
  case MWMBottomMenuStateActive:
    self.restoreState = _state;
    [self updateMenuButtonFromState:_state toState:state];
    self.additionalButtons.hidden = NO;
    self.bookmarksButton.hidden = NO;
    self.p2pButton.hidden = NO;
    self.searchButton.hidden = NO;
    break;
  case MWMBottomMenuStateCompact:
    if (_state == MWMBottomMenuStateGo)
      self.restoreState = _state;
    self.layoutDuration = IPAD ? kDefaultAnimationDuration : 0.0;
    break;
  case MWMBottomMenuStatePlanning:
    self.goButton.enabled = NO;
    self.goButton.hidden = NO;
    self.toggleInfoButton.hidden = YES;
    self.speedView.hidden = YES;
    self.timeView.hidden = YES;
    self.distanceView.hidden = YES;
    self.progressView.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateGo:
    self.goButton.enabled = YES;
    self.goButton.hidden = NO;
    self.toggleInfoButton.hidden = YES;
    self.speedView.hidden = YES;
    self.timeView.hidden = YES;
    self.distanceView.hidden = YES;
    self.progressView.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateRouting:
    self.goButton.hidden = YES;
    self.toggleInfoButton.hidden = NO;
    self.speedView.hidden = NO;
    self.timeView.hidden = NO;
    self.distanceView.hidden = NO;
    self.progressView.hidden = NO;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateRoutingExpanded:
    self.goButton.hidden = YES;
    self.toggleInfoButton.hidden = NO;
    self.speedView.hidden = NO;
    self.timeView.hidden = NO;
    self.distanceView.hidden = NO;
    self.progressView.hidden = NO;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = NO;
    break;
  }
  if (updateMenuButton)
    [self updateMenuButtonFromState:_state toState:state];
  _state = state;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  self.state = _leftBound > 1.0 ? MWMBottomMenuStateCompact : self.restoreState;
  [self setNeedsLayout];
}

- (void)setSearchIsActive:(BOOL)searchIsActive
{
  _searchIsActive = self.searchButton.selected = searchIsActive;
}

@end

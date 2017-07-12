#import "MWMBottomMenuView.h"
#import "EAGLView.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMRouter.h"
#import "MWMSideButtons.h"
#import "MapsAppDelegate.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIImageView+Coloring.h"
#import "UIView+RuntimeAttributes.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"

namespace
{
CGFloat constexpr kAdditionalHeight = 64;
CGFloat constexpr kDefaultMainButtonsHeight = 48;
CGFloat constexpr kBicyclePlanningMainButtonsHeightLandscape = 62;
CGFloat constexpr kBicyclePlanningMainButtonsHeightRegular = 94;
CGFloat constexpr kTaxiPreviewMainButtonHeight = 68.;
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
CGFloat constexpr kGoButtonWidthLandscape = 128;
CGFloat constexpr kGoButtonWidthRegular = 80;
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
@property(weak, nonatomic) IBOutlet MWMTaxiCollectionView * taxiCollectionView;

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
@property(weak, nonatomic) IBOutlet UILabel * estimateLabel;
@property(weak, nonatomic) IBOutlet UIView * heightProfileContainer;
@property(weak, nonatomic) IBOutlet UIView * taxiContainer;
@property(weak, nonatomic) IBOutlet UIImageView * heightProfileImage;
@property(weak, nonatomic) IBOutlet UIView * heightProfileElevation;
@property(weak, nonatomic) IBOutlet UIImageView * elevationImage;
@property(weak, nonatomic) IBOutlet UILabel * elevationHeight;

@property(weak, nonatomic) IBOutlet UIPageControl * pageControl;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * pageControlTopOffset;

@property(weak, nonatomic) IBOutlet NSLayoutConstraint * speedDistanceWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * timeWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * estimateLabelTopOffset;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint)
    NSArray * heightProfileContainerVerticalOrientation;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * routingViewManuButtonPffset;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * mainButtonConstraintsLeftToRight;

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
  self.estimateLabel.hidden = YES;
  self.heightProfileContainer.hidden = YES;
  self.heightProfileElevation.hidden = YES;
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
  self.elevationImage.mwm_coloring = MWMImageColoringBlue;

  if (isInterfaceRightToLeft())
  {
    for (NSLayoutConstraint * constraint in self.mainButtonConstraintsLeftToRight)
      constraint.priority = UILayoutPriorityFittingSizeLevel;
  }
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
  [self noftifyBottomBoundChange];
  [self updateFonts];
  [self updateHeightProfile];
  [super layoutSubviews];
}

- (void)noftifyBottomBoundChange
{
  if (self.state == MWMBottomMenuStateHidden)
    return;
  CGFloat const height = self.superview.height - self.mainButtonsHeight.constant;
  [MWMMapWidgets widgetsManager].bottomBound = height;
  [MWMSideButtons buttons].bottomBound = height;
}

- (void)updateAlphaAndColor
{
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: break;
  case MWMBottomMenuStateInactive:
    self.backgroundColor = [UIColor menuBackground];
    self.menuButton.alpha = 1.0;
    self.bookmarksButton.alpha = 1.0;
    self.downloadBadge.alpha = 1.0;
    self.routingView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    self.p2pButton.alpha = 1.0;
    self.searchButton.alpha = 1.0;
    break;
  case MWMBottomMenuStateActive:
    self.backgroundColor = [UIColor white];
    self.menuButton.alpha = 1.0;
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
    self.menuButton.alpha = 1.0;
    self.downloadBadge.alpha = 0.0;
    self.routingView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    break;
  case MWMBottomMenuStateGo:
    self.backgroundColor = [UIColor white];
    self.menuButton.alpha = 0.0;
    self.downloadBadge.alpha = 1.0;
    self.bookmarksButton.alpha = 0.0;
    self.routingView.alpha = 1.0;
    self.goButton.alpha = 1.0;
    self.estimateLabel.alpha = 1.0;
    self.heightProfileContainer.alpha = 1.0;
    self.speedView.alpha = 0.0;
    self.timeView.alpha = 0.0;
    self.distanceView.alpha = 0.0;
    self.progressView.alpha = 0.0;
    self.routingAdditionalView.alpha = 0.0;
    self.p2pButton.alpha = 0.0;
    self.searchButton.alpha = 0.0;
    break;
  case MWMBottomMenuStateRoutingError:
    self.backgroundColor = [UIColor white];
    self.menuButton.alpha = 0.0;
    self.downloadBadge.alpha = 1.0;
    self.bookmarksButton.alpha = 0.0;
    self.routingView.alpha = 1.0;
    self.goButton.alpha = 0.0;
    self.estimateLabel.alpha = 1.0;
    self.heightProfileContainer.alpha = 0.0;
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
    self.menuButton.alpha = 1.0;
    self.downloadBadge.alpha = 0.0;
    self.bookmarksButton.alpha = 0.0;
    self.routingView.alpha = 1.0;
    self.goButton.alpha = 0.0;
    self.estimateLabel.alpha = 0.0;
    self.heightProfileContainer.alpha = 0.0;
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

- (void)updateHeightProfile
{
  if (self.heightProfileContainer.hidden || ![MWMRouter hasRouteAltitude])
    return;
  dispatch_async(dispatch_get_main_queue(), ^{
    [MWMRouter routeAltitudeImageForSize:self.heightProfileImage.frame.size
                              completion:^(UIImage * image, NSString * altitudeElevation) {
                                self.heightProfileImage.image = image;
                                self.elevationHeight.text = altitudeElevation;
                              }];
  });
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
    self.taxiContainer.hidden = YES;
    self.estimateLabel.hidden = YES;
    break;
  case MWMBottomMenuStateActive:
    self.downloadBadge.hidden = YES;
    self.routingView.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    self.taxiContainer.hidden = YES;
    self.estimateLabel.hidden = YES;
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
    self.taxiContainer.hidden = YES;
    self.estimateLabel.hidden = YES;
    break;
  case MWMBottomMenuStateGo:
  {
    self.downloadBadge.hidden = YES;
    self.menuButton.hidden = YES;
    self.bookmarksButton.hidden = YES;
    self.p2pButton.hidden = YES;
    self.searchButton.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    BOOL const isNeedToShowTaxi = [MWMRouter isTaxi] || IPAD;
    self.estimateLabel.hidden = isNeedToShowTaxi;
    self.taxiContainer.hidden = !isNeedToShowTaxi;
    break;
  }
  case MWMBottomMenuStateRoutingError:
    self.downloadBadge.hidden = YES;
    self.menuButton.hidden = YES;
    self.bookmarksButton.hidden = YES;
    self.p2pButton.hidden = YES;
    self.searchButton.hidden = YES;
    self.routingAdditionalView.hidden = YES;
    self.estimateLabel.hidden = NO;
    self.taxiContainer.hidden = YES;
    break;
  case MWMBottomMenuStateRouting:
  case MWMBottomMenuStateRoutingExpanded:
    self.downloadBadge.hidden = YES;
    self.bookmarksButton.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = NO;
    self.p2pButton.hidden = YES;
    self.searchButton.hidden = YES;
    self.taxiContainer.hidden = YES;
    break;
  }
}

- (void)layoutGeometry
{
  self.separatorHeight.constant = 0.0;
  self.additionalButtonsHeight.constant = 0.0;
  self.menuButtonWidth.constant = kDefaultMenuButtonWidth;
  self.mainButtonsHeight.constant = kDefaultMainButtonsHeight;
  self.routingViewManuButtonPffset.priority = UILayoutPriorityDefaultHigh;
  self.routingAdditionalViewHeight.constant = 0.0;
  switch (self.state)
  {
  case MWMBottomMenuStateHidden: self.minY = self.superview.height; return;
  case MWMBottomMenuStateInactive: break;
  case MWMBottomMenuStateGo:
  {
    [[MWMNavigationDashboardManager manager] updateStartButtonTitle:self.goButton];
    [self layoutGoGeometry];
    if ([MWMRouter hasRouteAltitude])
    {
      BOOL const isLandscape = self.width > self.layoutThreshold;
      if (isLandscape)
      {
        self.mainButtonsHeight.constant = kBicyclePlanningMainButtonsHeightLandscape;
        self.estimateLabelTopOffset.priority = UILayoutPriorityDefaultHigh;
      }
      else
      {
        self.estimateLabelTopOffset.priority = UILayoutPriorityDefaultHigh;
        for (NSLayoutConstraint * constraint in self.heightProfileContainerVerticalOrientation)
          constraint.priority = UILayoutPriorityDefaultHigh;
        self.mainButtonsHeight.constant = kBicyclePlanningMainButtonsHeightRegular;
      }
    }
    else if ([MWMRouter isTaxi])
    {
      self.mainButtonsHeight.constant = kTaxiPreviewMainButtonHeight;
    }
    break;
  }
  case MWMBottomMenuStateRoutingError:
    self.mainButtonsHeight.constant = kDefaultMainButtonsHeight;
    break;
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

- (void)layoutGoGeometry
{
  BOOL const isLandscape = self.width > self.layoutThreshold;
  self.estimateLabelTopOffset.priority = UILayoutPriorityDefaultLow;
  self.routingViewManuButtonPffset.priority = UILayoutPriorityDefaultLow;
  for (NSLayoutConstraint * constraint in self.heightProfileContainerVerticalOrientation)
    constraint.priority = UILayoutPriorityDefaultLow;
  self.goButtonWidth.constant = isLandscape ? kGoButtonWidthLandscape : kGoButtonWidthRegular;
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
      case MWMBottomMenuStateRoutingError:
      case MWMBottomMenuStateGo: name = @"ic_menu"; break;
      case MWMBottomMenuStateActive:
      case MWMBottomMenuStateRouting:
      case MWMBottomMenuStateRoutingExpanded: name = @"ic_menu_down"; break;
      case MWMBottomMenuStateCompact: name = @"ic_menu_left"; break;
      }
      UIImage * image = [UIImage imageNamed:name];
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
  if (self.state == MWMBottomMenuStateInactive)
    [self updateBadge];
}

- (void)updateBadge
{
  auto state = self.state;
  if (state == MWMBottomMenuStateRouting || state == MWMBottomMenuStateRoutingExpanded ||
      state == MWMBottomMenuStateGo)
  {
    self.downloadBadge.hidden = YES;
    return;
  }
  auto & s = GetFramework().GetStorage();
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
    if (![MWMRouter isRoutingActive])
      _leftBound = 0.0;
    self.p2pButton.hidden = self.searchButton.hidden = self.bookmarksButton.hidden = NO;
    self.menuButton.hidden = NO;
    self.layoutDuration =
        (_state == MWMBottomMenuStateCompact && !IPAD) ? 0.0 : kDefaultAnimationDuration;
    break;
  }
  case MWMBottomMenuStateActive:
    self.restoreState = _state;
    [self updateMenuButtonFromState:_state toState:state];
    self.menuButton.hidden = NO;
    self.additionalButtons.hidden = NO;
    self.bookmarksButton.hidden = NO;
    self.p2pButton.hidden = NO;
    self.searchButton.hidden = NO;
    break;
  case MWMBottomMenuStateCompact:
    if (_state == MWMBottomMenuStateGo)
      self.restoreState = _state;
    self.layoutDuration = IPAD ? kDefaultAnimationDuration : 0.0;
    self.menuButton.hidden = NO;
    break;
  case MWMBottomMenuStateGo:
  {
    self.goButton.enabled = YES;
    self.goButton.hidden = NO;
    self.estimateLabel.hidden = NO;
    BOOL const hasAltitude = [MWMRouter hasRouteAltitude];
    self.heightProfileContainer.hidden = !hasAltitude;
    self.heightProfileElevation.hidden = !hasAltitude;
    self.toggleInfoButton.hidden = YES;
    self.speedView.hidden = YES;
    self.timeView.hidden = YES;
    self.distanceView.hidden = YES;
    self.progressView.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  }
  case MWMBottomMenuStateRoutingError:
    self.goButton.hidden = YES;
    self.estimateLabel.hidden = NO;
    self.heightProfileContainer.hidden = YES;
    self.heightProfileElevation.hidden = YES;
    self.toggleInfoButton.hidden = YES;
    self.speedView.hidden = YES;
    self.timeView.hidden = YES;
    self.distanceView.hidden = YES;
    self.progressView.hidden = YES;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateRouting:
    self.menuButton.hidden = NO;
    self.goButton.hidden = YES;
    self.estimateLabel.hidden = YES;
    self.heightProfileContainer.hidden = YES;
    self.heightProfileElevation.hidden = YES;
    self.toggleInfoButton.hidden = NO;
    self.speedView.hidden = NO;
    self.timeView.hidden = NO;
    self.distanceView.hidden = NO;
    self.progressView.hidden = NO;
    self.routingView.hidden = NO;
    self.routingAdditionalView.hidden = YES;
    break;
  case MWMBottomMenuStateRoutingExpanded:
    self.menuButton.hidden = NO;
    self.goButton.hidden = YES;
    self.estimateLabel.hidden = YES;
    self.heightProfileContainer.hidden = YES;
    self.heightProfileElevation.hidden = YES;
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
  [self updateBadge];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  if ([MWMNavigationDashboardManager manager].state != MWMNavigationDashboardStatePrepare)
    self.state = _leftBound > 1.0 ? MWMBottomMenuStateCompact : self.restoreState;
  [self setNeedsLayout];
}

- (void)setSearchIsActive:(BOOL)searchIsActive
{
  _searchIsActive = self.searchButton.selected = searchIsActive;
}

#pragma mark - VisibleArea / PlacePageArea

- (MWMAvailableAreaAffectDirections)placePageAreaAffectDirections
{
  return IPAD ? MWMAvailableAreaAffectDirectionsBottom : MWMAvailableAreaAffectDirectionsNone;
}

@end

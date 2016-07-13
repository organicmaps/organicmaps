#import "MWMNavigationInfoView.h"
#import "Common.h"
#import "MWMButton.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMRouter.h"
#import "UIFont+MapsMeFonts.h"
#import "UIImageView+Coloring.h"

#include "geometry/angles.hpp"

namespace
{
CGFloat constexpr kTurnsiPhoneWidth = 96;
CGFloat constexpr kTurnsiPadWidth = 140;

CGFloat constexpr kSearchMainButtonBottomOffsetPortrait = 174;
CGFloat constexpr kSearchMainButtonBottomOffsetLandscape = 48;

CGFloat constexpr kSearchButtonsViewHeightPortrait = 200;
CGFloat constexpr kSearchButtonsViewWidthPortrait = 200;
CGFloat constexpr kSearchButtonsViewHeightLandscape = 56;
CGFloat constexpr kSearchButtonsViewWidthLandscape = 286;
CGFloat constexpr kSearchButtonsSideSize = 44;

NSTimeInterval constexpr kCollapseSearchTimeout = 5.0;

enum class SearchState
{
  Expanded,
  CollapsedNormal,
  CollapsedSearch,
  CollapsedGas,
  CollapsedParking,
  CollapsedFood,
  CollapsedShop,
  CollapsedATM
};

map<SearchState, string> const kSearchStateButtonImageNames{
    {SearchState::Expanded, "ic_routing_search"},
    {SearchState::CollapsedNormal, "ic_routing_search"},
    {SearchState::CollapsedSearch, "ic_routing_search_off"},
    {SearchState::CollapsedGas, "ic_routing_fuel_off"},
    {SearchState::CollapsedParking, "ic_routing_parking_off"},
    {SearchState::CollapsedFood, "ic_routing_food_off"},
    {SearchState::CollapsedShop, "ic_routing_shop_off"},
    {SearchState::CollapsedATM, "ic_routing_atm_off"}};

BOOL defaultOrientation()
{
  return IPAD || UIDeviceOrientationIsPortrait([UIDevice currentDevice].orientation);
}
}  // namespace

@interface MWMNavigationInfoView ()<MWMLocationObserver>

@property(weak, nonatomic) IBOutlet UIView * streetNameView;
@property(weak, nonatomic) IBOutlet UILabel * streetNameLabel;
@property(weak, nonatomic) IBOutlet UIView * turnsView;
@property(weak, nonatomic) IBOutlet UIImageView * nextTurnImageView;
@property(weak, nonatomic) IBOutlet UILabel * distanceToNextTurnLabel;
@property(weak, nonatomic) IBOutlet UIView * secondTurnView;
@property(weak, nonatomic) IBOutlet UIImageView * secondTurnImageView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * turnsWidth;

@property(weak, nonatomic) IBOutlet UIView * searchView;
@property(weak, nonatomic) IBOutlet UIView * searchButtonsView;
@property(weak, nonatomic) IBOutlet MWMButton * searchMainButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchButtonsViewHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchButtonsViewWidth;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchMainButtonBottomOffset;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * searchLandscapeConstraints;
@property(nonatomic) IBOutletCollection(UIButton) NSArray * searchButtons;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * searchButtonsSideSize;
@property(weak, nonatomic) IBOutlet MWMButton * searchButtonGas;
@property(weak, nonatomic) IBOutlet MWMButton * searchButtonParking;
@property(weak, nonatomic) IBOutlet MWMButton * searchButtonFood;
@property(weak, nonatomic) IBOutlet MWMButton * searchButtonShop;
@property(weak, nonatomic) IBOutlet MWMButton * searchButtonATM;

@property(nonatomic) SearchState searchState;
@property(nonatomic) BOOL isVisible;

@property(weak, nonatomic) MWMNavigationDashboardEntity * navigationInfo;

@end

@implementation MWMNavigationInfoView

- (void)addToView:(UIView *)superview
{
  self.isVisible = YES;
  [self setSearchState:SearchState::CollapsedNormal animated:NO];
  if (IPAD)
  {
    self.turnsWidth.constant = kTurnsiPadWidth;
    self.distanceToNextTurnLabel.font = [UIFont bold36];
  }
  else
  {
    self.turnsWidth.constant = kTurnsiPhoneWidth;
    self.distanceToNextTurnLabel.font = [UIFont bold24];
  }
  NSAssert(superview != nil, @"Superview can't be nil");
  if ([superview.subviews containsObject:self])
    return;
  [superview insertSubview:self atIndex:0];
}

- (void)remove { self.isVisible = NO; }
- (void)layoutSubviews
{
  if (!CGRectEqualToRect(self.frame, self.defaultFrame))
  {
    self.frame = self.defaultFrame;
    [self setNeedsLayout];
  }

  if (!self.isVisible)
    [self removeFromSuperview];
  [super layoutSubviews];
}

- (CGFloat)visibleHeight { return self.streetNameView.maxY; }
#pragma mark - Search

- (IBAction)searchMainButtonTouchUpInside
{
  switch (self.searchState)
  {
  case SearchState::Expanded:
    [self setSearchState:SearchState::CollapsedSearch animated:YES];
    break;
  case SearchState::CollapsedNormal:
    [self setSearchState:SearchState::Expanded animated:YES];
    break;
  case SearchState::CollapsedSearch:
  case SearchState::CollapsedGas:
  case SearchState::CollapsedParking:
  case SearchState::CollapsedFood:
  case SearchState::CollapsedShop:
  case SearchState::CollapsedATM:
    [self setSearchState:SearchState::CollapsedNormal animated:YES];
    break;
  }
}

- (IBAction)searchButtonTouchUpInside:(MWMButton *)sender
{
  if (sender == self.searchButtonGas)
    [self setSearchState:SearchState::CollapsedGas animated:YES];
  else if (sender == self.searchButtonParking)
    [self setSearchState:SearchState::CollapsedParking animated:YES];
  else if (sender == self.searchButtonFood)
    [self setSearchState:SearchState::CollapsedFood animated:YES];
  else if (sender == self.searchButtonShop)
    [self setSearchState:SearchState::CollapsedShop animated:YES];
  else if (sender == self.searchButtonATM)
    [self setSearchState:SearchState::CollapsedATM animated:YES];
}

- (void)collapseSearchOnTimer { [self setSearchState:SearchState::CollapsedNormal animated:YES]; }
#pragma mark - MWMNavigationDashboardInfoProtocol

- (void)updateNavigationInfo:(MWMNavigationDashboardEntity *)info
{
  self.navigationInfo = info;
  if (info.streetName.length != 0)
  {
    self.streetNameView.hidden = NO;
    self.streetNameLabel.text = info.streetName;
  }
  else
  {
    self.streetNameView.hidden = YES;
  }
  if (info.turnImage)
  {
    self.turnsView.hidden = NO;
    self.nextTurnImageView.image = info.turnImage;
    if (isIOS7)
      [self.nextTurnImageView makeImageAlwaysTemplate];
    self.nextTurnImageView.mwm_coloring = MWMImageColoringWhite;
    self.distanceToNextTurnLabel.text =
        [NSString stringWithFormat:@"%@%@", info.distanceToTurn, info.turnUnits];
    if (info.nextTurnImage)
    {
      self.secondTurnView.hidden = NO;
      self.secondTurnImageView.image = info.nextTurnImage;
      if (isIOS7)
        [self.secondTurnImageView makeImageAlwaysTemplate];
      self.secondTurnImageView.mwm_coloring = MWMImageColoringBlack;
    }
    else
    {
      self.secondTurnView.hidden = YES;
    }
  }
  else
  {
    self.turnsView.hidden = YES;
  }
  self.hidden = self.streetNameView.hidden && self.turnsView.hidden;
}

- (void)layoutSearch
{
  BOOL const defaultView = defaultOrientation();
  self.searchMainButtonBottomOffset.constant =
      defaultView ? kSearchMainButtonBottomOffsetPortrait : kSearchMainButtonBottomOffsetLandscape;
  CGFloat alpha = 1;
  CGFloat searchButtonsSideSize = kSearchButtonsSideSize;
  if (self.searchState == SearchState::Expanded)
  {
    self.searchButtonsViewWidth.constant =
        defaultView ? kSearchButtonsViewWidthPortrait : kSearchButtonsViewWidthLandscape;
    self.searchButtonsViewHeight.constant =
        defaultView ? kSearchButtonsViewHeightPortrait : kSearchButtonsViewHeightLandscape;
  }
  else
  {
    self.searchButtonsViewWidth.constant = 0;
    self.searchButtonsViewHeight.constant = 0;
    alpha = 0;
    searchButtonsSideSize = 0;
  }
  for (UIButton * searchButton in self.searchButtons)
    searchButton.alpha = alpha;
  UILayoutPriority const priority =
      defaultView ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  for (NSLayoutConstraint * constraint in self.searchLandscapeConstraints)
    constraint.priority = priority;
  for (NSLayoutConstraint * constraint in self.searchButtonsSideSize)
    constraint.constant = searchButtonsSideSize;
}

#pragma mark - MWMLocationObserver

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;

  CGFloat const angle =
      ang::AngleTo(lastLocation.mercator,
                   location_helpers::ToMercator(self.navigationInfo.pedestrianDirectionPosition)) +
      info.m_bearing;
  self.nextTurnImageView.transform = CGAffineTransformMakeRotation(M_PI_2 - angle);
}

#pragma mark - SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == SearchState::Expanded)
    return;
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == SearchState::Expanded)
    return;
  [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == SearchState::Expanded)
    [self setSearchState:SearchState::CollapsedNormal animated:YES];
  else
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == SearchState::Expanded)
    [self setSearchState:SearchState::CollapsedNormal animated:YES];
  else
    [super touchesCancelled:touches withEvent:event];
}

#pragma mark - Properties

- (void)setFrame:(CGRect)frame
{
  if (CGRectEqualToRect(self.frame, frame))
    return;
  super.frame = frame;
  [self layoutIfNeeded];
  [self layoutSearch];
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     self.searchButtonsView.layer.cornerRadius =
                         (defaultOrientation() ? kSearchButtonsViewHeightPortrait
                                               : kSearchButtonsViewHeightLandscape) /
                         2;
                     [self layoutIfNeeded];
                   }];
}

- (void)setSearchState:(SearchState)searchState animated:(BOOL)animated
{
  self.searchState = searchState;
  auto block = ^{
    [self layoutSearch];
    [self layoutIfNeeded];
  };
  if (animated)
  {
    [self layoutIfNeeded];
    [UIView animateWithDuration:kDefaultAnimationDuration animations:block];
  }
  else
  {
    block();
  }
  SEL const collapseSelector = @selector(collapseSearchOnTimer);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:collapseSelector object:self];
  if (self.searchState == SearchState::Expanded)
  {
    [self.superview bringSubviewToFront:self];
    [self performSelector:collapseSelector withObject:self afterDelay:kCollapseSearchTimeout];
  }
  else
  {
    [self.superview sendSubviewToBack:self];
  }
}

- (void)setSearchState:(SearchState)searchState
{
  _searchState = searchState;
  self.searchMainButton.imageName = @(kSearchStateButtonImageNames.at(searchState).c_str());
}

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = isVisible;
  [self setNeedsLayout];
  if (isVisible && [MWMRouter router].type == routing::RouterType::Pedestrian)
    [MWMLocationManager addObserver:self];
  else
    [MWMLocationManager removeObserver:self];
}

- (CGRect)defaultFrame
{
  return CGRectMake(self.leftBound, 0.0, self.superview.width - self.leftBound,
                    self.superview.height);
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  [self setNeedsLayout];
}

- (CGFloat)extraCompassBottomOffset
{
  return defaultOrientation() || self.searchView.hidden ? 0 : kSearchButtonsViewHeightLandscape;
}

@end

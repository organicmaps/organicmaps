#import "MWMNavigationInfoView.h"
#import "CLLocation+Mercator.h"
#import "MWMButton.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMSearch.h"
#import "MapViewController.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"

#include "geometry/angles.hpp"

namespace
{
CGFloat constexpr kTurnsiPhoneWidth = 96;
CGFloat constexpr kTurnsiPadWidth = 140;

CGFloat constexpr kSearchButtonsViewHeightPortrait = 200;
CGFloat constexpr kSearchButtonsViewWidthPortrait = 200;
CGFloat constexpr kSearchButtonsViewHeightLandscape = 56;
CGFloat constexpr kSearchButtonsViewWidthLandscape = 286;
CGFloat constexpr kSearchButtonsSideSize = 44;
CGFloat constexpr kSearchButtonsBottomOffset = 6;
CGFloat constexpr kBaseTurnsTopOffset = 28;
CGFloat constexpr kShiftedTurnsTopOffset = 8;

NSTimeInterval constexpr kCollapseSearchTimeout = 5.0;

std::map<NavigationSearchState, NSString *> const kSearchStateButtonImageNames{
  {NavigationSearchStateMaximized, @"ic_routing_search"},
  {NavigationSearchStateMinimizedNormal, @"ic_routing_search"},
  {NavigationSearchStateMinimizedSearch, @"ic_routing_search_off"},
  {NavigationSearchStateMinimizedGas, @"ic_routing_fuel_off"},
  {NavigationSearchStateMinimizedParking, @"ic_routing_parking_off"},
  {NavigationSearchStateMinimizedEat, @"ic_routing_eat_off"},
  {NavigationSearchStateMinimizedFood, @"ic_routing_food_off"},
  {NavigationSearchStateMinimizedATM, @"ic_routing_atm_off"}};

std::map<NavigationSearchState, NSString *> const kSearchButtonRequest{
  {NavigationSearchStateMinimizedGas, L(@"category_fuel")},
  {NavigationSearchStateMinimizedParking, L(@"category_parking")},
  {NavigationSearchStateMinimizedEat, L(@"category_eat")},
  {NavigationSearchStateMinimizedFood, L(@"category_food")},
  {NavigationSearchStateMinimizedATM, L(@"category_atm")}};

BOOL defaultOrientation(CGSize const &size) {
  CGSize const &mapViewSize = [MapViewController sharedController].view.frame.size;
  CGFloat const minWidth = MIN(mapViewSize.width, mapViewSize.height);
  return IPAD || (size.height > size.width && size.width >= minWidth);
}
}  // namespace

@interface MWMNavigationInfoView () <MWMLocationObserver>

@property(weak, nonatomic) IBOutlet UIView *streetNameView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *streetNameTopOffsetConstraint;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *streetNameViewHideOffset;
@property(weak, nonatomic) IBOutlet UILabel *streetNameLabel;
@property(weak, nonatomic) IBOutlet UIView *turnsView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *turnsViewHideOffset;
@property(weak, nonatomic) IBOutlet UIImageView *nextTurnImageView;
@property(weak, nonatomic) IBOutlet UILabel *roundTurnLabel;
@property(weak, nonatomic) IBOutlet UILabel *distanceToNextTurnLabel;
@property(weak, nonatomic) IBOutlet UIView *secondTurnView;
@property(weak, nonatomic) IBOutlet UIImageView *secondTurnImageView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *turnsWidth;

@property(weak, nonatomic) IBOutlet UIView *searchButtonsView;
@property(weak, nonatomic) IBOutlet MWMButton *searchMainButton;
@property(weak, nonatomic) IBOutlet MWMButton *bookmarksButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *searchButtonsViewHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *searchButtonsViewWidth;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray *searchLandscapeConstraints;
@property(nonatomic) IBOutletCollection(UIButton) NSArray *searchButtons;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *searchButtonsSideSize;
@property(weak, nonatomic) IBOutlet MWMButton *searchGasButton;
@property(weak, nonatomic) IBOutlet MWMButton *searchParkingButton;
@property(weak, nonatomic) IBOutlet MWMButton *searchEatButton;
@property(weak, nonatomic) IBOutlet MWMButton *searchFoodButton;
@property(weak, nonatomic) IBOutlet MWMButton *searchATMButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *turnsTopOffset;

@property(weak, nonatomic) IBOutlet MWMNavigationAddPointToastView *toastView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *toastViewHideOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *searchMainButtonBottomConstraint;

@property(nonatomic, readwrite) NavigationSearchState searchState;
@property(nonatomic) BOOL isVisible;

@property(weak, nonatomic) MWMNavigationDashboardEntity *navigationInfo;

@property(nonatomic) BOOL hasLocation;

@property(nonatomic) NSLayoutConstraint *topConstraint;
@property(nonatomic) NSLayoutConstraint *leadingConstraint;
@property(nonatomic) NSLayoutConstraint *trailingConstraint;
@property(nonatomic) NSLayoutConstraint *heightConstraint;

@end

@implementation MWMNavigationInfoView

- (void)setMapSearch {
  [self setSearchState:NavigationSearchStateMinimizedSearch animated:YES];
}

- (void)updateToastView {
  // -S-F-L -> Start
  // -S-F+L -> Finish
  // -S+F-L -> Start
  // -S+F+L -> Start + Use
  // +S-F-L -> Finish
  // +S-F+L -> Finish
  // +S+F-L -> Hide
  // +S+F+L -> Hide

  BOOL const hasStart = ([MWMRouter startPoint] != nil);
  BOOL const hasFinish = ([MWMRouter finishPoint] != nil);
  self.hasLocation = ([MWMLocationManager lastLocation] != nil);

  if (hasStart && hasFinish) {
    [self setToastViewHidden:YES];
    return;
  }

  [self setToastViewHidden:NO];

  auto toastView = self.toastView;

  if (hasStart) {
    [toastView configWithIsStart:NO withLocationButton:NO];
    return;
  }

  if (hasFinish) {
    [toastView configWithIsStart:YES withLocationButton:self.hasLocation];
    return;
  }

  if (self.hasLocation)
    [toastView configWithIsStart:NO withLocationButton:NO];
  else
    [toastView configWithIsStart:YES withLocationButton:NO];
}

- (void)updateSideButtonsAvailableArea:(CGRect)frame animated:(BOOL)animated {
  CGFloat const height = frame.size.height;
  if (height == 0)
    return;

  CGFloat const screenHeight = [UIScreen mainScreen].bounds.size.height;
  CGFloat const bottomOffset = screenHeight - height + kSearchButtonsBottomOffset;
  self.searchMainButtonBottomConstraint.constant = bottomOffset;
  [UIView animateWithDuration:animated ? kDefaultAnimationDuration : 0
                        delay:0
                      options:UIViewAnimationOptionBeginFromCurrentState
                   animations:^{
    [self layoutIfNeeded];
  }
                   completion:nil];

  CGFloat const topSafeAreaInset = UIApplication.sharedApplication.delegate.window.safeAreaInsets.top;
  BOOL isOutOfBounds = height < topSafeAreaInset + kSearchButtonsSideSize + kSearchButtonsViewHeightPortrait;
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
    self.searchMainButton.alpha = isOutOfBounds ? 0.0 : 1.0;
    self.bookmarksButton.alpha = isOutOfBounds ? 0.0 : 1.0;
  }];
}

- (SearchOnMapManager *)searchManager {
  return [MapViewController sharedController].searchManager;
}

- (IBAction)openSearch {
  BOOL const isStart = self.toastView.isStart;

  [self.searchManager setRoutingTooltip:
    isStart ? SearchOnMapRoutingTooltipSearchStart : SearchOnMapRoutingTooltipSearchFinish ];
  [self.searchManager startSearchingWithIsRouting:YES];
}

- (IBAction)addLocationRoutePoint {
  NSAssert(![MWMRouter startPoint], @"Action button is active while start point is available");
  NSAssert([MWMLocationManager lastLocation], @"Action button is active while my location is not available");

  [MWMRouter buildFromPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                             intermediateIndex:0]
                 bestRouter:NO];
}

- (IBAction)ttsButtonAction {
  [self.delegate ttsButtonDidTap];
}

- (IBAction)settingsButtonAction {
  [self.delegate settingsButtonDidTap];
}

- (IBAction)stopRoutingButtonAction {
  [self.delegate stopRoutingButtonDidTap];
}

#pragma mark - Search

- (IBAction)searchMainButtonTouchUpInside {
  switch (self.searchState) {
    case NavigationSearchStateMaximized:
      [self.searchManager startSearchingWithIsRouting:YES];
      [self setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
      break;
    case NavigationSearchStateMinimizedNormal:
      if (self.state == MWMNavigationInfoViewStatePrepare) {
        [self.searchManager startSearchingWithIsRouting:YES];
      } else {
        [self setSearchState:NavigationSearchStateMaximized animated:YES];
      }
      break;
    case NavigationSearchStateMinimizedSearch:
    case NavigationSearchStateMinimizedGas:
    case NavigationSearchStateMinimizedParking:
    case NavigationSearchStateMinimizedEat:
    case NavigationSearchStateMinimizedFood:
    case NavigationSearchStateMinimizedATM:
      [MWMSearch clear];
      [self.searchManager hide];
      [self setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
      break;
  }
}

- (IBAction)searchButtonTouchUpInside:(MWMButton *)sender {
  auto const body = ^(NavigationSearchState state) {
    NSString * text = [kSearchButtonRequest.at(state) stringByAppendingString:@" "];
    NSString * locale = [[AppInfo sharedInfo] languageId];
    // Category request from navigation search wheel.
    SearchQuery * query = [[SearchQuery alloc] init:text locale:locale source:SearchTextSourceCategory];
    [MWMSearch searchQuery:query];
    [self setSearchState:state animated:YES];
  };

  if (sender == self.searchGasButton)
    body(NavigationSearchStateMinimizedGas);
  else if (sender == self.searchParkingButton)
    body(NavigationSearchStateMinimizedParking);
  else if (sender == self.searchEatButton)
    body(NavigationSearchStateMinimizedEat);
  else if (sender == self.searchFoodButton)
    body(NavigationSearchStateMinimizedFood);
  else if (sender == self.searchATMButton)
    body(NavigationSearchStateMinimizedATM);
}

- (IBAction)bookmarksButtonTouchUpInside {
  [[MapViewController sharedController].bookmarksCoordinator open];
}

- (void)collapseSearchOnTimer {
  [self setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
}

- (void)layoutSearch {
  BOOL const defaultView = defaultOrientation(self.availableArea.size);
  CGFloat alpha = 0;
  CGFloat searchButtonsSideSize = 0;
  self.searchButtonsViewWidth.constant = 0;
  self.searchButtonsViewHeight.constant = 0;
  if (self.searchState == NavigationSearchStateMaximized) {
    alpha = 1;
    searchButtonsSideSize = kSearchButtonsSideSize;
    self.searchButtonsViewWidth.constant =
      defaultView ? kSearchButtonsViewWidthPortrait : kSearchButtonsViewWidthLandscape;
    self.searchButtonsViewHeight.constant =
      defaultView ? kSearchButtonsViewHeightPortrait : kSearchButtonsViewHeightLandscape;
  }
  for (UIButton *searchButton in self.searchButtons)
    searchButton.alpha = alpha;
  UILayoutPriority const priority = (defaultView ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh);
  for (NSLayoutConstraint *constraint in self.searchLandscapeConstraints)
    constraint.priority = priority;
  self.searchButtonsSideSize.constant = searchButtonsSideSize;
}

#pragma mark - MWMNavigationDashboardManager

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)info {
  self.navigationInfo = info;
  if (self.state != MWMNavigationInfoViewStateNavigation)
    return;
  if (info.streetName.length != 0) {
    [self setStreetNameVisible:YES];
    self.streetNameLabel.text = info.streetName;
  } else {
    [self setStreetNameVisible:NO];
  }
  if (info.turnImage) {
    [self setTurnsViewVisible:YES];
    self.nextTurnImageView.image = info.turnImage;

    if (info.roundExitNumber == 0) {
      self.roundTurnLabel.hidden = YES;
    } else {
      self.roundTurnLabel.hidden = NO;
      self.roundTurnLabel.text = @(info.roundExitNumber).stringValue;
    }

    NSDictionary *turnNumberAttributes =
      @{NSForegroundColorAttributeName: [UIColor white], NSFontAttributeName: IPAD ? [UIFont bold36] : [UIFont bold28]};
    NSDictionary *turnLegendAttributes =
      @{NSForegroundColorAttributeName: [UIColor white], NSFontAttributeName: IPAD ? [UIFont bold24] : [UIFont bold16]};

    NSMutableAttributedString *distance = [[NSMutableAttributedString alloc] initWithString:info.distanceToTurn
                                                                                 attributes:turnNumberAttributes];
    [distance appendAttributedString:[[NSAttributedString alloc]
                                       initWithString:[NSString stringWithFormat:@" %@", info.turnUnits]
                                           attributes:turnLegendAttributes]];

    self.distanceToNextTurnLabel.attributedText = distance;
    if (info.nextTurnImage) {
      self.secondTurnView.hidden = NO;
      self.secondTurnImageView.image = info.nextTurnImage;
    } else {
      self.secondTurnView.hidden = YES;
    }
  } else {
    [self setTurnsViewVisible:NO];
  }
  [self setNeedsLayout];
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(CLLocation *)location {
  BOOL const hasLocation = ([MWMLocationManager lastLocation] != nil);
  if (self.hasLocation != hasLocation)
    [self updateToastView];
}

#pragma mark - SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  if (self.searchState == NavigationSearchStateMaximized)
    return;
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  if (self.searchState == NavigationSearchStateMaximized)
    return;
  [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  if (self.searchState == NavigationSearchStateMaximized)
    [self setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
  else
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
  if (self.searchState == NavigationSearchStateMaximized)
    [self setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
  else
    [super touchesCancelled:touches withEvent:event];
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  UIView *hitView = [super hitTest:point withEvent:event];
  if (hitView == self) {
    return nil;
  }
  return hitView;
}

- (void)configLayout {
  UIView *ov = self.superview;
  self.translatesAutoresizingMaskIntoConstraints = NO;

  self.topConstraint = [self.topAnchor constraintEqualToAnchor:ov.topAnchor];
  self.leadingConstraint = [self.leadingAnchor constraintEqualToAnchor:ov.leadingAnchor];
  self.trailingConstraint = [self.trailingAnchor constraintEqualToAnchor:ov.trailingAnchor];
  self.heightConstraint = [self.heightAnchor constraintEqualToConstant:ov.frame.size.height];
  self.streetNameTopOffsetConstraint.constant = self.additionalStreetNameTopOffset;
  [NSLayoutConstraint activateConstraints:@[
    self.topConstraint,
    self.leadingConstraint,
    self.trailingConstraint,
    self.heightConstraint
  ]];
  [self refreshLayout];
}

// Additional spacing for devices with a small top safe area (such as SE or when the device is in landscape mode).
- (CGFloat)additionalStreetNameTopOffset {
  return MapsAppDelegate.theApp.window.safeAreaInsets.top <= 20 ? 10 : 0;;
}

- (void)refreshLayout {
  dispatch_async(dispatch_get_main_queue(), ^{
    if (UIDeviceOrientationIsLandscape([UIDevice currentDevice].orientation))
      self.streetNameLabel.numberOfLines = 1;
    else
      self.streetNameLabel.numberOfLines = 2;

    auto const availableArea = self.availableArea;
    [self animateConstraintsWithAnimations:^{
      self.topConstraint.constant = availableArea.origin.y;
      self.leadingConstraint.constant = availableArea.origin.x + kViewControlsOffsetToBounds;
      self.trailingConstraint.constant = availableArea.origin.x - kViewControlsOffsetToBounds;
      self.heightConstraint.constant = availableArea.size.height;

      [self layoutSearch];
      self.turnsTopOffset.constant = availableArea.origin.y > 0 ? kShiftedTurnsTopOffset : kBaseTurnsTopOffset;
      self.searchButtonsView.layer.cornerRadius =
        (defaultOrientation(availableArea.size) ? kSearchButtonsViewHeightPortrait
                                                : kSearchButtonsViewHeightLandscape) /
        2;
      if (@available(iOS 13.0, *)) {
        self.searchButtonsView.layer.cornerCurve = kCACornerCurveContinuous;
      }
      self.streetNameTopOffsetConstraint.constant = self.additionalStreetNameTopOffset;
    }];
  });
}

#pragma mark - Properties

- (void)setAvailableArea:(CGRect)availableArea {
  if (CGRectEqualToRect(_availableArea, availableArea))
    return;
  _availableArea = availableArea;
  [self refreshLayout];
}

- (void)setSearchState:(NavigationSearchState)searchState animated:(BOOL)animated {
  self.searchState = searchState;
  auto block = ^{
    [self layoutSearch];
    [self layoutIfNeeded];
  };
  if (animated) {
    [self layoutIfNeeded];
    [UIView animateWithDuration:kDefaultAnimationDuration animations:block];
  } else {
    block();
  }
  SEL const collapseSelector = @selector(collapseSearchOnTimer);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:collapseSelector object:self];
  if (self.searchState == NavigationSearchStateMaximized) {
    [self.superview bringSubviewToFront:self];
    [self performSelector:collapseSelector withObject:self afterDelay:kCollapseSearchTimeout];
  } else {
    [self.superview sendSubviewToBack:self];
  }
}

- (void)setSearchState:(NavigationSearchState)searchState {
  _searchState = searchState;
  self.searchMainButton.imageName = kSearchStateButtonImageNames.at(searchState);
}

- (void)setState:(MWMNavigationInfoViewState)state {
  if (_state == state)
    return;
  _state = state;
  switch (state) {
    case MWMNavigationInfoViewStateHidden:
      self.isVisible = NO;
      [self setToastViewHidden:YES];
      [MWMLocationManager removeObserver:self];
      break;
    case MWMNavigationInfoViewStateNavigation:
      self.isVisible = YES;
      if ([MWMRouter type] == MWMRouterTypePedestrian)
        [MWMLocationManager addObserver:self];
      else
        [MWMLocationManager removeObserver:self];
      break;
    case MWMNavigationInfoViewStatePrepare:
      self.isVisible = YES;
      [self setStreetNameVisible:NO];
      [self setTurnsViewVisible:NO];
      [MWMLocationManager addObserver:self];
      break;
  }
}

- (void)setStreetNameVisible:(BOOL)isVisible {
  self.streetNameView.hidden = !isVisible;
  self.streetNameViewHideOffset.priority = isVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

- (void)setTurnsViewVisible:(BOOL)isVisible {
  self.turnsView.hidden = !isVisible;
  self.turnsViewHideOffset.priority = isVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

- (void)setIsVisible:(BOOL)isVisible {
  if (_isVisible == isVisible)
    return;
  _isVisible = isVisible;
  [self setNeedsLayout];
  if (isVisible) {
    [self setSearchState:NavigationSearchStateMinimizedNormal animated:NO];
    self.turnsWidth.constant = IPAD ? kTurnsiPadWidth : kTurnsiPhoneWidth;
    UIView *sv = self.ownerView;
    NSAssert(sv != nil, @"Superview can't be nil");
    if (![sv.subviews containsObject:self]) {
      [sv insertSubview:self atIndex:0];
      [self configLayout];
    }
  }
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
    self.alpha = isVisible ? 1 : 0;
    [self layoutIfNeeded];
  }
                   completion:^(BOOL finished) {
    if (!isVisible)
      self.hidden = isVisible;
  }];
}

- (void)setToastViewHidden:(BOOL)hidden {
  if (!hidden)
    self.toastView.hidden = NO;
  [self setNeedsLayout];
  self.toastViewHideOffset.priority = (hidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow);
  [UIView animateWithDuration:kDefaultAnimationDuration
    animations:^{
      [self layoutIfNeeded];
    }
    completion:^(BOOL finished) {
      if (hidden)
        self.toastView.hidden = YES;
    }];
}

// MARK: Update Theme
- (void)applyTheme {
  [self setSearchState:_searchState];
}
@end

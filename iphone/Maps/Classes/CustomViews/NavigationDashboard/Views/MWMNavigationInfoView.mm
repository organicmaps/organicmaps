#import "MWMNavigationInfoView.h"
#import "CLLocation+Mercator.h"
#import "MWMButton.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMSearch.h"
#import "MWMSearchManager.h"
#import "MapViewController.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"

#include "geometry/angles.hpp"

namespace
{
CGFloat const kTurnsiPhoneWidth = 96;
CGFloat const kTurnsiPadWidth = 140;

CGFloat const kSearchButtonsViewHeightPortrait = 200;
CGFloat const kSearchButtonsViewWidthPortrait = 200;
CGFloat const kSearchButtonsViewHeightLandscape = 56;
CGFloat const kSearchButtonsViewWidthLandscape = 286;
CGFloat const kSearchButtonsSideSize = 44;
CGFloat const kBaseTurnsTopOffset = 28;
CGFloat const kShiftedTurnsTopOffset = 8;

NSTimeInterval constexpr kCollapseSearchTimeout = 5.0;

map<NavigationSearchState, NSString *> const kSearchStateButtonImageNames{
    {NavigationSearchState::Maximized, @"ic_routing_search"},
    {NavigationSearchState::MinimizedNormal, @"ic_routing_search"},
    {NavigationSearchState::MinimizedSearch, @"ic_routing_search_off"},
    {NavigationSearchState::MinimizedGas, @"ic_routing_fuel_off"},
    {NavigationSearchState::MinimizedParking, @"ic_routing_parking_off"},
    {NavigationSearchState::MinimizedFood, @"ic_routing_food_off"},
    {NavigationSearchState::MinimizedShop, @"ic_routing_shop_off"},
    {NavigationSearchState::MinimizedATM, @"ic_routing_atm_off"}};

map<NavigationSearchState, NSString *> const kSearchButtonRequest{
    {NavigationSearchState::MinimizedGas, L(@"fuel")},
    {NavigationSearchState::MinimizedParking, L(@"parking")},
    {NavigationSearchState::MinimizedFood, L(@"food")},
    {NavigationSearchState::MinimizedShop, L(@"shop")},
    {NavigationSearchState::MinimizedATM, L(@"atm")}};

BOOL defaultOrientation(CGSize const & size)
{
  CGSize const & mapViewSize = [MapViewController controller].view.frame.size;
  CGFloat const minWidth = MIN(mapViewSize.width, mapViewSize.height);
  return IPAD || (size.height > size.width && size.width >= minWidth);
}
}  // namespace

@interface MWMNavigationInfoView ()<MWMLocationObserver>

@property(weak, nonatomic) IBOutlet UIView * streetNameView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * streetNameViewHideOffset;
@property(weak, nonatomic) IBOutlet UILabel * streetNameLabel;
@property(weak, nonatomic) IBOutlet UIView * turnsView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * turnsViewHideOffset;
@property(weak, nonatomic) IBOutlet UIImageView * nextTurnImageView;
@property(weak, nonatomic) IBOutlet UILabel * roundTurnLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceToNextTurnLabel;
@property(weak, nonatomic) IBOutlet UIView * secondTurnView;
@property(weak, nonatomic) IBOutlet UIImageView * secondTurnImageView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * turnsWidth;

@property(weak, nonatomic) IBOutlet UIView * searchButtonsView;
@property(weak, nonatomic) IBOutlet MWMButton * searchMainButton;
@property(weak, nonatomic) IBOutlet MWMButton * bookmarksButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchButtonsViewHeight;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchButtonsViewWidth;
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * searchLandscapeConstraints;
@property(nonatomic) IBOutletCollection(UIButton) NSArray * searchButtons;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * searchButtonsSideSize;
@property(weak, nonatomic) IBOutlet MWMButton * searchGasButton;
@property(weak, nonatomic) IBOutlet MWMButton * searchParkingButton;
@property(weak, nonatomic) IBOutlet MWMButton * searchFoodButton;
@property(weak, nonatomic) IBOutlet MWMButton * searchShopButton;
@property(weak, nonatomic) IBOutlet MWMButton * searchATMButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * turnsTopOffset;

@property(weak, nonatomic) IBOutlet MWMNavigationAddPointToastView * toastView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * toastViewHideOffset;

@property(nonatomic, readwrite) NavigationSearchState searchState;
@property(nonatomic) BOOL isVisible;

@property(weak, nonatomic) MWMNavigationDashboardEntity * navigationInfo;

@property(nonatomic) BOOL hasLocation;

@property(nonatomic) CGRect availableArea;

@end

@implementation MWMNavigationInfoView

- (void)setMapSearch { [self setSearchState:NavigationSearchState::MinimizedSearch animated:YES]; }
- (void)updateToastView
{
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

  if (hasStart && hasFinish)
  {
    [self setToastViewHidden:YES];
    return;
  }

  [self setToastViewHidden:NO];

  auto toastView = self.toastView;

  if (hasStart)
  {
    [toastView configWithIsStart:NO withLocationButton:NO];
    return;
  }

  if (hasFinish)
  {
    [toastView configWithIsStart:YES withLocationButton:self.hasLocation];
    return;
  }

  if (self.hasLocation)
    [toastView configWithIsStart:NO withLocationButton:NO];
  else
    [toastView configWithIsStart:YES withLocationButton:NO];
}

- (IBAction)openSearch
{
  BOOL const isStart = self.toastView.isStart;
  auto const type = isStart ? kStatRoutingPointTypeStart : kStatRoutingPointTypeFinish;
  [Statistics logEvent:kStatRoutingTooltipClicked withParameters:@{kStatRoutingPointType : type}];
  auto searchManager = [MWMSearchManager manager];

  searchManager.routingTooltipSearch = isStart ? MWMSearchManagerRoutingTooltipSearchStart
                                               : MWMSearchManagerRoutingTooltipSearchFinish;
  searchManager.state = MWMSearchManagerStateDefault;
}

- (IBAction)addLocationRoutePoint
{
  NSAssert(![MWMRouter startPoint], @"Action button is active while start point is available");
  NSAssert([MWMLocationManager lastLocation],
           @"Action button is active while my location is not available");

  [MWMRouter
      buildFromPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                      intermediateIndex:0]
          bestRouter:NO];
}

#pragma mark - Search

- (IBAction)searchMainButtonTouchUpInside
{
  switch (self.searchState)
  {
  case NavigationSearchState::Maximized:
    [MWMSearchManager manager].state = MWMSearchManagerStateDefault;
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
    [Statistics logEvent:kStatRoutingSearchClicked
          withParameters:@{kStatMode: kStatRoutingModeOnRoute}];
    break;
  case NavigationSearchState::MinimizedNormal:
    if (self.state == MWMNavigationInfoViewStatePrepare)
    {
      [MWMSearchManager manager].state = MWMSearchManagerStateDefault;
      [Statistics logEvent:kStatRoutingSearchClicked
            withParameters:@{kStatMode: kStatRoutingModePlanning}];
    }
    else
    {
      [self setSearchState:NavigationSearchState::Maximized animated:YES];
    }
    break;
  case NavigationSearchState::MinimizedSearch:
  case NavigationSearchState::MinimizedGas:
  case NavigationSearchState::MinimizedParking:
  case NavigationSearchState::MinimizedFood:
  case NavigationSearchState::MinimizedShop:
  case NavigationSearchState::MinimizedATM:
    [MWMSearch clear];
    [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
    break;
  }
}

- (IBAction)searchButtonTouchUpInside:(MWMButton *)sender
{
  auto const body = ^(NavigationSearchState state) {
    [MWMSearch setSearchOnMap:YES];
    NSString * query = [kSearchButtonRequest.at(state) stringByAppendingString:@" "];
    NSString * locale = [[AppInfo sharedInfo] languageId];
    [MWMSearch searchQuery:query forInputLocale:locale];
    [self setSearchState:state animated:YES];
  };

  if (sender == self.searchGasButton)
    body(NavigationSearchState::MinimizedGas);
  else if (sender == self.searchParkingButton)
    body(NavigationSearchState::MinimizedParking);
  else if (sender == self.searchFoodButton)
    body(NavigationSearchState::MinimizedFood);
  else if (sender == self.searchShopButton)
    body(NavigationSearchState::MinimizedShop);
  else if (sender == self.searchATMButton)
    body(NavigationSearchState::MinimizedATM);
}

- (IBAction)bookmarksButtonTouchUpInside
{
  BOOL const isOnRoute = (self.state == MWMNavigationInfoViewStateNavigation);
  [Statistics logEvent:kStatRoutingBookmarksClicked
        withParameters:@{
          kStatMode: (isOnRoute ? kStatRoutingModeOnRoute : kStatRoutingModePlanning)
        }];
  [[MapViewController controller] openBookmarks];
}

- (void)collapseSearchOnTimer
{
  [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
}

- (void)layoutSearch
{
  BOOL const defaultView = defaultOrientation(self.frame.size);
  CGFloat alpha = 0;
  CGFloat searchButtonsSideSize = 0;
  self.searchButtonsViewWidth.constant = 0;
  self.searchButtonsViewHeight.constant = 0;
  if (self.searchState == NavigationSearchState::Maximized)
  {
    alpha = 1;
    searchButtonsSideSize = kSearchButtonsSideSize;
    self.searchButtonsViewWidth.constant =
        defaultView ? kSearchButtonsViewWidthPortrait : kSearchButtonsViewWidthLandscape;
    self.searchButtonsViewHeight.constant =
        defaultView ? kSearchButtonsViewHeightPortrait : kSearchButtonsViewHeightLandscape;
  }
  for (UIButton * searchButton in self.searchButtons)
    searchButton.alpha = alpha;
  UILayoutPriority const priority =
      (defaultView ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh);
  for (NSLayoutConstraint * constraint in self.searchLandscapeConstraints)
    constraint.priority = priority;
  self.searchButtonsSideSize.constant = searchButtonsSideSize;
}

#pragma mark - MWMNavigationDashboardManager

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)info
{
  self.navigationInfo = info;
  BOOL const isPedestrianRouting = [MWMRouter type] == MWMRouterTypePedestrian;
  if (self.state != MWMNavigationInfoViewStateNavigation)
    return;
  if (!isPedestrianRouting && info.streetName.length != 0)
  {
    [self setStreetNameVisible:YES];
    self.streetNameLabel.text = info.streetName;
  }
  else
  {
    [self setStreetNameVisible:NO];
  }
  if (info.turnImage)
  {
    [self setTurnsViewVisible:YES];
    self.nextTurnImageView.image = info.turnImage;
    self.nextTurnImageView.mwm_coloring = MWMImageColoringWhite;

    if (info.roundExitNumber == 0)
    {
      self.roundTurnLabel.hidden = YES;
    }
    else
    {
      self.roundTurnLabel.hidden = NO;
      self.roundTurnLabel.text = @(info.roundExitNumber).stringValue;
    }

    NSDictionary * turnNumberAttributes = @{
      NSForegroundColorAttributeName : [UIColor white],
      NSFontAttributeName : IPAD ? [UIFont bold36] : [UIFont bold28]
    };
    NSDictionary * turnLegendAttributes = @{
      NSForegroundColorAttributeName : [UIColor white],
      NSFontAttributeName : IPAD ? [UIFont bold24] : [UIFont bold16]
    };

    NSMutableAttributedString * distance =
        [[NSMutableAttributedString alloc] initWithString:info.distanceToTurn
                                               attributes:turnNumberAttributes];
    [distance
        appendAttributedString:[[NSAttributedString alloc]
                                   initWithString:[NSString stringWithFormat:@" %@", info.turnUnits]
                                       attributes:turnLegendAttributes]];

    self.distanceToNextTurnLabel.attributedText = distance;
    if (!isPedestrianRouting && info.nextTurnImage)
    {
      self.secondTurnView.hidden = NO;
      self.secondTurnImageView.image = info.nextTurnImage;
      self.secondTurnImageView.mwm_coloring = MWMImageColoringBlack;
    }
    else
    {
      self.secondTurnView.hidden = YES;
    }
  }
  else
  {
    [self setTurnsViewVisible:NO];
  }
  [self setNeedsLayout];
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)gpsInfo
{
  BOOL const hasLocation = ([MWMLocationManager lastLocation] != nil);
  if (self.hasLocation != hasLocation)
    [self updateToastView];
}

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  auto transform = CATransform3DIdentity;
  auto lastLocation = [MWMLocationManager lastLocation];
  if (lastLocation && self.state == MWMNavigationInfoViewStateNavigation &&
      [MWMRouter type] == MWMRouterTypePedestrian)
  {
    auto const angle = ang::AngleTo(lastLocation.mercator,
                                    self.navigationInfo.pedestrianDirectionPosition.mercator);
    transform = CATransform3DMakeRotation(M_PI_2 - angle - info.m_bearing, 0, 0, 1);
  }
  self.nextTurnImageView.layer.transform = transform;
}

#pragma mark - SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == NavigationSearchState::Maximized)
    return;
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == NavigationSearchState::Maximized)
    return;
  [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == NavigationSearchState::Maximized)
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
  else
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (self.searchState == NavigationSearchState::Maximized)
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
  else
    [super touchesCancelled:touches withEvent:event];
}

#pragma mark - Properties

- (void)setFrame:(CGRect)frame
{
  super.frame = frame;
  [self setNeedsLayout];
  [self layoutSearch];
  self.turnsTopOffset.constant = self.minY > 0 ? kShiftedTurnsTopOffset : kBaseTurnsTopOffset;
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        self.searchButtonsView.layer.cornerRadius =
            (defaultOrientation(self.frame.size) ? kSearchButtonsViewHeightPortrait
                                                 : kSearchButtonsViewHeightLandscape) /
            2;
        [self layoutIfNeeded];
      }];
}

- (void)setSearchState:(NavigationSearchState)searchState animated:(BOOL)animated
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
  if (self.searchState == NavigationSearchState::Maximized)
  {
    [self.superview bringSubviewToFront:self];
    [self performSelector:collapseSelector withObject:self afterDelay:kCollapseSearchTimeout];
  }
  else
  {
    [self.superview sendSubviewToBack:self];
  }
}

- (void)setSearchState:(NavigationSearchState)searchState
{
  _searchState = searchState;
  self.searchMainButton.imageName = kSearchStateButtonImageNames.at(searchState);
}

- (void)setState:(MWMNavigationInfoViewState)state
{
  if (_state == state)
    return;
  _state = state;
  switch (state)
  {
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

- (void)setStreetNameVisible:(BOOL)isVisible
{
  self.streetNameView.hidden = !isVisible;
  self.streetNameViewHideOffset.priority =
      isVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

- (void)setTurnsViewVisible:(BOOL)isVisible
{
  self.turnsView.hidden = !isVisible;
  self.turnsViewHideOffset.priority =
      isVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

- (void)setIsVisible:(BOOL)isVisible
{
  if (_isVisible == isVisible)
    return;
  _isVisible = isVisible;
  [self setNeedsLayout];
  if (isVisible)
  {
    self.bookmarksButton.imageName = @"ic_routing_bookmark";
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:NO];
    self.turnsWidth.constant = IPAD ? kTurnsiPadWidth : kTurnsiPhoneWidth;
    UIView * sv = self.ownerView;
    NSAssert(sv != nil, @"Superview can't be nil");
    if ([sv.subviews containsObject:self])
      return;
    [sv insertSubview:self atIndex:0];
  }
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        [self layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        if (!isVisible)
          [self removeFromSuperview];
      }];
}

- (void)setToastViewHidden:(BOOL)hidden
{
  if (!hidden)
    self.toastView.hidden = NO;
  [self setNeedsLayout];
  self.toastViewHideOffset.priority =
      (hidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow);
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        [self layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        if (hidden)
          self.toastView.hidden = YES;
      }];
}

@end

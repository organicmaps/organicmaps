#import "MWMNavigationInfoView.h"
#import "AppInfo.h"
#import "CLLocation+Mercator.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsManager.h"
#import "MWMMapWidgets.h"
#import "MWMRouter.h"
#import "MWMSearch.h"
#import "MapViewController.h"
#import "Statistics.h"
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
@property(nonatomic) IBOutletCollection(NSLayoutConstraint) NSArray * searchButtonsSideSize;
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

@end

@implementation MWMNavigationInfoView

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (!CGRectEqualToRect(self.frame, self.defaultFrame))
  {
    self.frame = self.defaultFrame;
    [[MWMMapViewControlsManager manager] navigationDashBoardDidUpdate];
    [self setNeedsLayout];
  }
}

- (CGFloat)leftHeight { return self.turnsView.maxY; }
- (CGFloat)rightHeight { return self.streetNameView.hidden ? 0 : self.streetNameView.maxY; }
- (CGFloat)bottom { return self.toastView.minY; }
- (CGFloat)left
{
  auto sv = self.superview;
  BOOL const isLandscape = sv.width > sv.height;
  return isLandscape ? self.searchMainButton.maxX : 0;
}

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
    [toastView configWithText:L(@"routing_add_finish_point") withLocationButton:NO];
    return;
  }

  if (hasFinish)
  {
    [toastView configWithText:L(@"routing_add_start_point") withLocationButton:self.hasLocation];
    return;
  }

  if (self.hasLocation)
    [toastView configWithText:L(@"routing_add_finish_point") withLocationButton:NO];
  else
    [toastView configWithText:L(@"routing_add_start_point") withLocationButton:NO];
}

- (IBAction)openSearch
{
  BOOL const isStart = ([MWMRouter startPoint] == nil);
  auto const type = isStart ? kStatRoutingPointTypeStart : kStatRoutingPointTypeFinish;
  [Statistics logEvent:kStatRoutingTooltipClicked withParameters:@{kStatRoutingPointType : type}];
  [MWMMapViewControlsManager manager].searchHidden = NO;
}

- (IBAction)addLocationRoutePoint
{
  NSAssert(![MWMRouter startPoint], @"Action button is active while start point is available");
  NSAssert([MWMLocationManager lastLocation],
           @"Action button is active while my location is not available");

  [Statistics logEvent:kStatRoutingAddPoint
        withParameters:@{
          kStatRoutingPointType : kStatRoutingPointTypeStart,
          kStatRoutingPointValue : kStatRoutingPointValueMyPosition,
          kStatRoutingPointMethod : kStatRoutingPointMethodPlanning,
          kStatRoutingMode : kStatRoutingModePlanning
        }];
  [MWMRouter
      buildFromPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart]
          bestRouter:NO];
}

#pragma mark - Search

- (IBAction)searchMainButtonTouchUpInside
{
  BOOL const isOnRoute = (self.state == MWMNavigationInfoViewStateNavigation);
  [Statistics logEvent:kStatRoutingSearchClicked
        withParameters:@{
          kStatRoutingMode : (isOnRoute ? kStatRoutingModeOnRoute : kStatRoutingModePlanning)
        }];
  switch (self.searchState)
  {
  case NavigationSearchState::Maximized:
    [MWMMapViewControlsManager manager].searchHidden = NO;
    [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
    break;
  case NavigationSearchState::MinimizedNormal:
    if (self.state == MWMNavigationInfoViewStatePrepare)
    {
      [MWMMapViewControlsManager manager].searchHidden = NO;
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
    [MWMMapViewControlsManager manager].searchHidden = YES;
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
          kStatRoutingMode : (isOnRoute ? kStatRoutingModeOnRoute : kStatRoutingModePlanning)
        }];
  [[MapViewController controller] openBookmarks];
}

- (void)collapseSearchOnTimer
{
  [self setSearchState:NavigationSearchState::MinimizedNormal animated:YES];
}

#pragma mark - MWMNavigationDashboardInfoProtocol

- (void)updateNavigationInfo:(MWMNavigationDashboardEntity *)info
{
  self.navigationInfo = info;
  if (self.state != MWMNavigationInfoViewStateNavigation)
    return;
  if (info.streetName.length != 0)
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
    if (info.nextTurnImage)
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

- (void)layoutSearch
{
  BOOL const defaultView = defaultOrientation(self.frame.size);
  CGFloat alpha = 1;
  CGFloat searchButtonsSideSize = kSearchButtonsSideSize;
  if (self.searchState == NavigationSearchState::Maximized)
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

- (void)onLocationUpdate:(location::GpsInfo const &)gpsInfo
{
  BOOL const hasLocation = ([MWMLocationManager lastLocation] != nil);
  if (self.hasLocation != hasLocation)
    [self updateToastView];
}

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;

  CGFloat const angle =
      ang::AngleTo(lastLocation.mercator,
                   location_helpers::ToMercator(self.navigationInfo.pedestrianDirectionPosition)) +
      info.m_bearing;
  self.nextTurnImageView.layer.transform = CATransform3DMakeRotation(M_PI_2 - angle, 0, 0, 1);
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
  CGSize const & oldSize = self.frame.size;
  CGSize const & size = frame.size;
  if (CGRectEqualToRect(self.frame, frame) || (equalScreenDimensions(oldSize.width, size.width) &&
                                               equalScreenDimensions(oldSize.height, size.height)))
    return;
  super.frame = frame;
  [self layoutIfNeeded];
  [self layoutSearch];
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        self.searchButtonsView.layer.cornerRadius =
            (defaultOrientation(self.frame.size) ? kSearchButtonsViewHeightPortrait
                                                 : kSearchButtonsViewHeightLandscape) /
            2;
        [self layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        [[MWMMapWidgets widgetsManager] layoutWidgets];
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
  _state = state;
  switch (state)
  {
  case MWMNavigationInfoViewStateHidden: self.isVisible = NO; break;
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
  [self setNeedsLayout];
  if (_isVisible == isVisible)
    return;
  _isVisible = isVisible;
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
  else
  {
    [self removeFromSuperview];
  }
}

- (CGRect)defaultFrame
{
  return {{self.leftBound, self.topBound},
          {self.superview.width - self.leftBound, self.superview.height - self.topBound}};
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = MAX(topBound, 0.0);
  self.turnsTopOffset.constant = topBound > 0 ? kShiftedTurnsTopOffset : kBaseTurnsTopOffset;
  [self setNeedsLayout];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = MAX(leftBound, 0.0);
  [self setNeedsLayout];
}

- (void)setToastViewHidden:(BOOL)hidden
{
  if (!hidden)
    self.toastView.hidden = NO;
  [self setNeedsLayout];
  self.toastViewHideOffset.priority =
      hidden ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow;
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     [self layoutIfNeeded];
                   } completion:^(BOOL finished) {
                     if (hidden)
                       self.toastView.hidden = YES;
                   }];
}

@end

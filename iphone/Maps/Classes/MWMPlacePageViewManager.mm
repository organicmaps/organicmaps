//
//  MWMPlacePageViewManager.m
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMBasePlacePageView.h"
#import "MWMiPadPlacePage.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMPlacePageViewManager.h"
#import "ShareActionSheet.h"
#import "UIKitCategories.h"
#import "MWMDirectionView.h"
#import "MWMPlacePageNavigationBar.h"

#include "Framework.h"

typedef NS_ENUM(NSUInteger, MWMPlacePageManagerState)
{
  MWMPlacePageManagerStateClosed,
  MWMPlacePageManagerStateOpen
};

@interface MWMPlacePageViewManager () <LocationObserver>
{
  unique_ptr<UserMarkCopy> m_userMark;
}

@property (weak, nonatomic) UIViewController<MWMPlacePageViewManagerDelegate> * ownerViewController;
@property (nonatomic, readwrite) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePage * placePage;
@property (nonatomic) MWMPlacePageManagerState state;
@property (nonatomic) ShareActionSheet * actionSheet;
@property (nonatomic) MWMDirectionView * directionView;

@property (weak, nonatomic) id<MWMPlacePageViewManagerDelegate> delegate;

@end

@implementation MWMPlacePageViewManager

- (instancetype)initWithViewController:(UIViewController *)viewController delegate:(id<MWMPlacePageViewManagerDelegate>) delegate
{
  self = [super init];
  if (self)
  {
    self.ownerViewController = viewController;
    self.delegate = delegate;
    self.state = MWMPlacePageManagerStateClosed;
  }
  return self;
}

- (void)dismissPlacePage
{
  self.state = MWMPlacePageManagerStateClosed;
  [self.placePage dismiss];
  [[MapsAppDelegate theApp].m_locationManager stop:self];
  GetFramework().GetBalloonManager().RemovePin();
  m_userMark = nullptr;
  self.entity = nil;
  self.placePage = nil;
}

- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark
{
  NSAssert(userMark, @"userMark cannot be nil");
  m_userMark = std::move(userMark);
  [[MapsAppDelegate theApp].m_locationManager start:self];
  self.entity = [[MWMPlacePageEntity alloc] initWithUserMark:m_userMark->GetUserMark()];
  self.state = MWMPlacePageManagerStateOpen;
  if (IPAD)
    [self setPlacePageForiPad];
  else
    [self setPlacePageForiPhoneWithOrientation:self.ownerViewController.interfaceOrientation];
  [self configPlacePage];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  if (!self.placePage)
    return;

  if (IPAD)
  {
    self.placePage.parentViewHeight = self.ownerViewController.view.width;
    [(MWMiPadPlacePage *)self.placePage updatePlacePageLayout];
  }
  else
  {
    [self.placePage dismiss];
    [self setPlacePageForiPhoneWithOrientation:orientation];
    [self configPlacePage];
  }
}

- (void)configPlacePage
{
  if (self.entity.type == MWMPlacePageEntityTypeMyPosition)
  {
    BOOL hasSpeed;
    self.entity.category = [[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:hasSpeed];
  }
  self.placePage.topBound = self.topBound;
  self.placePage.parentViewHeight = self.ownerViewController.view.height;
  [self.placePage configure];
  [self refreshPlacePage];
}

- (void)refreshPlacePage
{
  [self.placePage show];
  [self updateDistance];
}

- (void)setPlacePageForiPad
{
  [self.placePage dismiss];
  self.placePage = [[MWMiPadPlacePage alloc] initWithManager:self];
}

- (void)updateMyPositionSpeedAndAltitude
{
  if (self.entity.type != MWMPlacePageEntityTypeMyPosition)
    return;
  BOOL hasSpeed = NO;
  [self.placePage updateMyPositionStatus:[[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:hasSpeed]];
}

- (void)setPlacePageForiPhoneWithOrientation:(UIInterfaceOrientation)orientation
{
  if (self.state == MWMPlacePageManagerStateClosed)
    return;

  switch (orientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
      if (![self.placePage isKindOfClass:[MWMiPhoneLandscapePlacePage class]])
        self.placePage = [[MWMiPhoneLandscapePlacePage alloc] initWithManager:self];
      break;

    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
      if (![self.placePage isKindOfClass:[MWMiPhonePortraitPlacePage class]])
        self.placePage = [[MWMiPhonePortraitPlacePage alloc] initWithManager:self];
      break;

    case UIInterfaceOrientationUnknown:
      break;
  }
}

- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller
{
  if (controller)
    [self.ownerViewController addChildViewController:controller];
  [self.delegate addPlacePageViews:views];
}

- (void)buildRoute
{
  GetFramework().BuildRoute(m_userMark->GetUserMark()->GetOrg(), 0 /* timeoutSec */);
}

- (void)stopBuildingRoute
{
  [self.placePage stopBuildingRoute];
}

- (void)share
{
  MWMPlacePageEntity * entity = self.entity;
  ShareInfo * info = [[ShareInfo alloc] initWithText:entity.title lat:entity.point.x lon:entity.point.y myPosition:NO];

  self.actionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self.ownerViewController];
  UIView * parentView = self.ownerViewController.view;
  CGRect rect = CGRectMake(parentView.midX, parentView.height - 40.0, 0.0, 0.0);
  [self.actionSheet showFromRect:rect];
}

- (void)addBookmark
{
  Framework & f = GetFramework();
  BookmarkData data = BookmarkData(self.entity.title.UTF8String, f.LastEditedBMType());
  size_t const categoryIndex = f.LastEditedBMCategory();
  size_t const bookmarkIndex = f.GetBookmarkManager().AddBookmark(categoryIndex, m_userMark->GetUserMark()->GetOrg(), data);
  self.entity.bac = make_pair(categoryIndex, bookmarkIndex);
  self.entity.type = MWMPlacePageEntityTypeBookmark;
  BookmarkCategory const * category = f.GetBmCategory(categoryIndex);
  Bookmark const * bookmark = category->GetBookmark(bookmarkIndex);
  m_userMark.reset(new UserMarkCopy(bookmark, false));
  f.ActivateUserMark(bookmark);
  f.Invalidate();
}

- (void)removeBookmark
{
  Framework & f = GetFramework();
  BookmarkCategory * bookmarkCategory = f.GetBookmarkManager().GetBmCategory(self.entity.bac.first);
  UserMark const * bookmark = bookmarkCategory->GetBookmark(self.entity.bac.second);
  BookmarkAndCategory const bookmarkAndCategory = f.FindBookmark(bookmark);
  self.entity.type = MWMPlacePageEntityTypeRegular;
  PoiMarkPoint const * poi = f.GetAddressMark(bookmark->GetOrg());
  m_userMark.reset(new UserMarkCopy(poi, false));
  f.ActivateUserMark(poi);
  if (bookmarkCategory)
  {
    bookmarkCategory->DeleteBookmark(bookmarkAndCategory.second);
    bookmarkCategory->SaveToKMLFile();
  }
  f.Invalidate();
}

- (void)reloadBookmark
{
  [self.entity synchronize];
  [self.placePage reloadBookmark];
}

- (void)dragPlacePage:(CGPoint)point
{
  [self.delegate dragPlacePage:point];
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error %i in %@", errorCode, [[self class] className]);
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  [self updateDistance];
  [self updateMyPositionSpeedAndAltitude];
}

- (void)updateDistance
{
  NSString * distance = [self distance];
  self.directionView.distanceLabel.text = distance;
  [self.placePage setDistance:distance];
}

- (NSString *)distance
{
  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location || !m_userMark)
    return @"";

  double azimut = -1;
  double north = -1;
  [[MapsAppDelegate theApp].m_locationManager getNorthRad:north];
  string distance;
  CLLocationCoordinate2D const coord = location.coordinate;
  GetFramework().GetDistanceAndAzimut(m_userMark->GetUserMark()->GetOrg(), coord.latitude, coord.longitude, north, distance, azimut);
  return [NSString stringWithUTF8String:distance.c_str()];
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location || !m_userMark)
    return;

  CLLocationCoordinate2D const coord = location.coordinate;
  CGFloat angle = ang::AngleTo(MercatorBounds::FromLatLon(coord.latitude, coord.longitude), m_userMark->GetUserMark()->GetOrg()) + info.m_bearing;
  CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI_2 - angle);
  [self.placePage setDirectionArrowTransform:transform];
  [self.directionView setDirectionArrowTransform:transform];
}

- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type
{
  MWMDirectionView * directionView = self.directionView;
  directionView.titleLabel.text = title;
  directionView.typeLabel.text = type;
  [self.ownerViewController.view addSubview:directionView];
  [directionView setNeedsLayout];
  [self.delegate updateStatusBarStyle];
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate disableStandby];
  [self updateDistance];
}

- (void)hideDirectionView
{
  [self.directionView removeFromSuperview];
  [self.delegate updateStatusBarStyle];
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate enableStandby];
}

#pragma mark - Properties

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] initWithManager:self];
  return _directionView;
}

- (BOOL)isDirectionViewShown
{
  return self.directionView.superview != nil;
}

- (void)setTopBound:(CGFloat)bound
{
  _topBound = bound;
  self.placePage.topBound = bound;
}

@end

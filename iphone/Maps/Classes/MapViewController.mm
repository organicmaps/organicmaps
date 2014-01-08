#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "BookmarksRootVC.h"
#import "PlacePageVC.h"
#import "PlacePreviewViewController.h"
#import "MWMApi.h"
#import "UIKitCategories.h"
#import "SettingsViewController.h"
#import "UIViewController+Navigation.h"
#import "Config.h"
#import "ShareActionSheet.h"
#import "MPInterstitialAdController.h"
#import "MPInterstitialViewController.h"
#import "MPAdView.h"
#import "Reachability.h"
#import "AppInfo.h"

#import "../Settings/SettingsManager.h"
#import "../../Common/CustomAlertView.h"

#include "Framework.h"
#include "RenderContext.hpp"

#include "../../../anim/controller.hpp"
#include "../../../gui/controller.hpp"
#include "../../../platform/platform.hpp"
#include "../Statistics/Statistics.h"
#include "../../../map/dialog_settings.hpp"
#include "../../../platform/settings.hpp"

#define FACEBOOK_ALERT_VIEW 1
#define APPSTORE_ALERT_VIEW 2
#define ITUNES_URL @"itms-apps://itunes.apple.com/app/id%lld"
#define FACEBOOK_URL @"http://www.facebook.com/MapsWithMe"
#define FACEBOOK_SCHEME @"fb://profile/111923085594432"

const long long PRO_IDL = 510623322L;
const long long LITE_IDL = 431183278L;

@interface MapViewController () <SideToolbarDelegate, MPInterstitialAdControllerDelegate, MPAdViewDelegate>

@property (nonatomic) UIView * fadeView;
@property (nonatomic) LocationButton * locationButton;
@property (nonatomic, strong) UINavigationBar * apiNavigationBar;
@property ShareActionSheet * shareActionSheet;
@property MPInterstitialAdController * interstitialAd;
@property MPAdView * topBannerAd;

@end


@implementation MapViewController

#pragma mark - LocationManager Callbacks

- (void)onLocationError:(location::TLocationError)errorCode
{
  GetFramework().OnLocationError(errorCode);

  switch (errorCode)
  {
    case location::EDenied:
    {
      UIAlertView * alert = [[CustomAlertView alloc] initWithTitle:nil
                                                           message:NSLocalizedString(@"location_is_disabled_long_text", @"Location services are disabled by user alert - message")
                                                          delegate:nil
                                                 cancelButtonTitle:NSLocalizedString(@"ok", @"Location Services are disabled by user alert - close alert button")
                                                 otherButtonTitles:nil];
      [alert show];
      [[MapsAppDelegate theApp].m_locationManager stop:self];
    }
    break;

    case location::ENotSupported:
    {
      UIAlertView * alert = [[CustomAlertView alloc] initWithTitle:nil
                                                           message:NSLocalizedString(@"device_doesnot_support_location_services", @"Location Services are not available on the device alert - message")
                                                          delegate:nil
                                                 cancelButtonTitle:NSLocalizedString(@"ok", @"Location Services are not available on the device alert - close alert button")
                                                 otherButtonTitles:nil];
      [alert show];
      [[MapsAppDelegate theApp].m_locationManager stop:self];
    }
    break;

    default:
      break;
  }
  [self.locationButton setImage:[UIImage imageNamed:@"LocationDefault"] forState:UIControlStateSelected];
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // TODO: Remove this hack for location changing bug
  if (self.navigationController.visibleViewController == self)
  {
    Framework & f = GetFramework();

    if (f.GetLocationState()->IsFirstPosition())
      [self.locationButton setImage:[UIImage imageNamed:@"LocationSelected"] forState:UIControlStateSelected];

    f.OnLocationUpdate(info);

    [self showPopover];

    [[Statistics instance] logLatitude:info.m_latitude
                             longitude:info.m_longitude
                    horizontalAccuracy:info.m_horizontalAccuracy
                      verticalAccuracy:info.m_verticalAccuracy];
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  // TODO: Remove this hack for orientation changing bug
  if (self.navigationController.visibleViewController == self)
    GetFramework().OnCompassUpdate(info);
}

- (void)onCompassStatusChanged:(int)newStatus
{
  Framework & f = GetFramework();
  shared_ptr<location::State> ls = f.GetLocationState();

  if (newStatus == location::ECompassFollow)
  {
    [self.locationButton setImage:[UIImage imageNamed:@"LocationFollow"] forState:UIControlStateSelected];
  }
  else
  {
    if (ls->HasPosition())
      [self.locationButton setImage:[UIImage imageNamed:@"LocationSelected"] forState:UIControlStateSelected];
    else
      [self.locationButton setImage:[UIImage imageNamed:@"LocationDefault"] forState:UIControlStateSelected];

    self.locationButton.selected = YES;
  }
}

#pragma mark - Map Navigation

- (void)positionBallonClickedLat:(double)lat lon:(double)lon
{
  CGPoint const p = CGPointMake(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
  PlacePreviewViewController * preview = [[PlacePreviewViewController alloc] initWithPoint:p];
  m_popoverPos = p;
  [self pushViewController:preview];
}

- (void)poiBalloonClicked:(m2::PointD const &)pt info:(search::AddressInfo const &)info
{
  PlacePreviewViewController * preview = [[PlacePreviewViewController alloc] initWith:info point:CGPointMake(pt.x, pt.y)];
  m_popoverPos = CGPointMake(pt.x, pt.y);
  [self pushViewController:preview];
}

- (void)apiBalloonClicked:(url_scheme::ApiPoint const &)apiPoint
{
  if (GetFramework().GoBackOnBalloonClick() && [MWMApi canOpenApiUrl:apiPoint])
  {
    [MWMApi openAppWithPoint:apiPoint];
    return;
  }
  PlacePreviewViewController * apiPreview = [[PlacePreviewViewController alloc] initWithApiPoint:apiPoint];
  m_popoverPos = CGPointMake(MercatorBounds::LonToX(apiPoint.m_lon), MercatorBounds::LatToY(apiPoint.m_lat));
  [self pushViewController:apiPreview];
}

- (void)bookmarkBalloonClicked:(BookmarkAndCategory const &)bmAndCat
{
  PlacePageVC * vc = [[PlacePageVC alloc] initWithBookmark:bmAndCat];
  Bookmark const * bm = GetFramework().GetBmCategory(bmAndCat.first)->GetBookmark(bmAndCat.second);
  m_popoverPos = CGPointMake(bm->GetOrg().x, bm->GetOrg().y);
  [self pushViewController:vc];
}

- (void)onMyPositionClicked:(id)sender
{
  Framework & f = GetFramework();
  shared_ptr<location::State> ls = f.GetLocationState();

  if (!ls->HasPosition())
  {
    if (!ls->IsFirstPosition())
    {
      self.locationButton.selected = YES;
      [self.locationButton setImage:[UIImage imageNamed:@"LocationSearch"] forState:UIControlStateSelected];
      [self.locationButton setSearching];

      ls->OnStartLocation();

      [[MapsAppDelegate theApp] disableStandby];
      [[MapsAppDelegate theApp].m_locationManager start:self];

      return;
    }
  }
  else
  {
    if (!ls->IsCentered())
    {
      ls->AnimateToPositionAndEnqueueLocationProcessMode(location::ELocationCenterOnly);
      self.locationButton.selected = YES;
      return;
    }
    else
      if (GetPlatform().HasRotation())
      {
        if (ls->HasCompass())
        {
          if (ls->GetCompassProcessMode() != location::ECompassFollow)
          {
            if (ls->IsCentered())
              ls->StartCompassFollowing();
            else
              ls->AnimateToPositionAndEnqueueFollowing();

            self.locationButton.selected = YES;
            [self.locationButton setImage:[UIImage imageNamed:@"LocationFollow"] forState:UIControlStateSelected];

            return;
          }
          else
          {
            anim::Controller *animController = f.GetAnimController();
            animController->Lock();

            f.GetInformationDisplay().locationState()->StopCompassFollowing();

            double startAngle = f.GetNavigator().Screen().GetAngle();
            double endAngle = 0;

            f.GetAnimator().RotateScreen(startAngle, endAngle);

            animController->Unlock();

            f.Invalidate();
          }
        }
      }
  }

  ls->OnStopLocation();

  [[MapsAppDelegate theApp] enableStandby];
  [[MapsAppDelegate theApp].m_locationManager stop:self];

  self.locationButton.selected = NO;
  [self.locationButton setImage:[UIImage imageNamed:@"LocationDefault"] forState:UIControlStateSelected];
}

- (IBAction)zoomInPressed:(id)sender
{
  GetFramework().Scale(2.0);
}

- (IBAction)zoomOutPressed:(id)sender
{
  GetFramework().Scale(0.5);
}

- (void)processMapClickAtPoint:(CGPoint)point longClick:(BOOL)isLongClick
{
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  m2::PointD pxClicked(point.x * scaleFactor, point.y * scaleFactor);
  GetFramework().GetBalloonManager().OnClick(m2::PointD(pxClicked.x, pxClicked.y), isLongClick);
}

- (void)onSingleTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:NO];
}

- (void)onLongTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:YES];
}

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController
{
  [self destroyPopover];
  [self invalidate];
}

- (void)showSearchResultAsBookmarkAtMercatorPoint:(const m2::PointD &)pt withInfo:(const search::AddressInfo &)info
{
  GetFramework().GetBalloonManager().ShowAddress(pt, info);
}

- (void)showBalloonWithCategoryIndex:(int)cat andBookmarkIndex:(int)bm
{
  GetFramework().GetBalloonManager().ShowBookmark(BookmarkAndCategory(cat, bm));
}
- (void)updatePointsFromEvent:(UIEvent *)event
{
	NSSet * allTouches = [event allTouches];

  UIView * v = self.view;
  CGFloat const scaleFactor = v.contentScaleFactor;

  // 0 touches are possible from touchesCancelled.
  switch ([allTouches count])
  {
    case 0:
      break;

    case 1:
    {
      CGPoint const pt = [[[allTouches allObjects] objectAtIndex:0] locationInView:v];
      m_Pt1 = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
      break;
    }

    default:
    {
      NSArray * sortedTouches = [[allTouches allObjects] sortedArrayUsingFunction:compareAddress context:NULL];
      CGPoint const pt1 = [[sortedTouches objectAtIndex:0] locationInView:v];
      CGPoint const pt2 = [[sortedTouches objectAtIndex:1] locationInView:v];

      m_Pt1 = m2::PointD(pt1.x * scaleFactor, pt1.y * scaleFactor);
      m_Pt2 = m2::PointD(pt2.x * scaleFactor, pt2.y * scaleFactor);
      break;
    }
  }
}

- (void)stopCurrentAction
{
	switch (m_CurrentAction)
	{
		case NOTHING:
			break;
		case DRAGGING:
			GetFramework().StopDrag(DragEvent(m_Pt1.x, m_Pt1.y));
			break;
		case SCALING:
			GetFramework().StopScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			break;
	}

	m_CurrentAction = NOTHING;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.sideToolbar.isMenuHidden)
    return;

  // To cancel single tap timer
  UITouch * theTouch = (UITouch *)[touches anyObject];
  if (theTouch.tapCount > 1)
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

	[self updatePointsFromEvent:event];

  Framework & f = GetFramework();

	if ([[event allTouches] count] == 1)
	{
    if (f.GetGuiController()->OnTapStarted(m_Pt1))
      return;

		f.StartDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		m_CurrentAction = DRAGGING;

    // Start long-tap timer
    [self performSelector:@selector(onLongTap:) withObject:[NSValue valueWithCGPoint:[theTouch locationInView:self.view]] afterDelay:1.0];
    // Temporary solution to filter long touch
    m_touchDownPoint = m_Pt1;
	}
	else
	{
		f.StartScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
		m_CurrentAction = SCALING;
	}

	m_isSticking = true;

  if (!self.sideToolbar.isMenuHidden)
    [self.sideToolbar setMenuHidden:YES animated:YES];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.sideToolbar.isMenuHidden)
    return;

  m2::PointD const TempPt1 = m_Pt1;
	m2::PointD const TempPt2 = m_Pt2;

	[self updatePointsFromEvent:event];

  // Cancel long-touch timer
  if (!m_touchDownPoint.EqualDxDy(m_Pt1, 9))
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

  Framework & f = GetFramework();

  if (f.GetGuiController()->OnTapMoved(m_Pt1))
    return;

	if (m_isSticking)
	{
		if ((TempPt1.Length(m_Pt1) > m_StickyThreshold) || (TempPt2.Length(m_Pt2) > m_StickyThreshold))
    {
			m_isSticking = false;
    }
		else
		{
			// Still stickying. Restoring old points and return.
			m_Pt1 = TempPt1;
			m_Pt2 = TempPt2;
			return;
		}
	}

	switch (m_CurrentAction)
	{
    case DRAGGING:
      f.DoDrag(DragEvent(m_Pt1.x, m_Pt1.y));
      //		needRedraw = true;
      break;
    case SCALING:
      if ([[event allTouches] count] < 2)
        [self stopCurrentAction];
      else
      {
        f.DoScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
        //			needRedraw = true;
      }
      break;
    case NOTHING:
      return;
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.sideToolbar.isMenuHidden)
  {
    [self.sideToolbar setMenuHidden:YES animated:YES];
    return;
  }

	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

  UITouch * theTouch = (UITouch *)[touches anyObject];
  NSUInteger const tapCount = theTouch.tapCount;
  NSUInteger const touchesCount = [[event allTouches] count];

  Framework & f = GetFramework();

  if (touchesCount == 1)
  {
    // Cancel long-touch timer
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // TapCount could be zero if it was a single long (or moving) tap.
    if (tapCount < 2)
    {
      if (f.GetGuiController()->OnTapEnded(m_Pt1))
        return;
    }

    if (tapCount == 1)
    {
      // Launch single tap timer
      if (m_isSticking)
        [self performSelector:@selector(onSingleTap:) withObject:[NSValue valueWithCGPoint:[theTouch locationInView:self.view]] afterDelay:0.3];
    }
    else if (tapCount == 2 && m_isSticking)
      f.ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2.0));
  }

  if (touchesCount == 2 && tapCount == 1 && m_isSticking)
  {
    f.Scale(0.5);
    if (!m_touchDownPoint.EqualDxDy(m_Pt1, 9))
      [NSObject cancelPreviousPerformRequestsWithTarget:self];
    m_isSticking = NO;
  }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  [self updatePointsFromEvent:event];
  [self stopCurrentAction];
}

#pragma mark - ViewController lifecycle

- (void)dealloc
{
  [self destroyPopover];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation)interfaceOrientation
{
	return YES; // We support all orientations
}

- (void)didRotateFromInterfaceOrientation: (UIInterfaceOrientation)fromInterfaceOrientation
{
  [self showPopover];
  [self invalidate];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  [self.topBannerAd rotateToOrientation:toInterfaceOrientation];
  [UIView animateWithDuration:duration animations:^{
    self.topBannerAd.midX = self.view.width / 2;
  }];
}

- (void)didReceiveMemoryWarning
{
	GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)onTerminate
{
  GetFramework().SaveState();
}

- (void)onEnterBackground
{
  // Save state and notify about entering background.

  Framework & f = GetFramework();
  f.SaveState();
  f.SetUpdatesEnabled(false);
  f.EnterBackground();
}

- (void)onEnterForeground
{
  // Notify about entering foreground (should be called on the first launch too).
  GetFramework().EnterForeground();

  if (self.isViewLoaded && self.view.window)
    [self invalidate]; // only invalidate when map is displayed on the screen

  Reachability * reachability = [Reachability reachabilityForInternetConnection];
  if ([reachability isReachable])
  {
    if (dlg_settings::ShouldShow(dlg_settings::AppStore))
      [self showAppStoreRatingMenu];
    else if (dlg_settings::ShouldShow(dlg_settings::FacebookDlg))
      [self showFacebookRatingMenu];
  }
  [self tryToShowInterstitial];
  [self tryToShowTopBanner];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];

  [self invalidate];

  [self tryToShowInterstitial];
  [self tryToShowTopBanner];
}

- (void)viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];

  [self.sideToolbar setMenuHidden:YES animated:NO];
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  bool zoomButtonsEnabled;
  if (!Settings::Get("ZoomButtonsEnabled", zoomButtonsEnabled))
    zoomButtonsEnabled = false;
  self.zoomButtonsView.hidden = !zoomButtonsEnabled;

  self.view.clipsToBounds = YES;

  [self.view addSubview:self.apiNavigationBar];
  [self.view addSubview:self.locationButton];
  [self.view addSubview:self.fadeView];
  [self.view addSubview:self.sideToolbar];

  [self.sideToolbar setMenuHidden:YES animated:NO];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
}

- (void)didEnterBackground:(NSNotification *)notification
{
#ifdef OMIM_LITE
  if ([self.presentedViewController isKindOfClass:[MPInterstitialViewController class]])
    [[Statistics instance] logEvent:@"Application closed while interstitial was being shown" withParameters:@{@"Country" : [AppInfo sharedInfo].countryCode}];
#endif
}

- (void)tryToShowTopBanner
{
#ifdef OMIM_LITE
  if (!self.topBannerAd && [[AppInfo sharedInfo] featureAvailable:AppFeatureBannerAd])
  {
    NSString * adUnitId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"MoPubTopBannerAdUnitId"];
    self.topBannerAd = [[MPAdView alloc] initWithAdUnitId:adUnitId size:MOPUB_BANNER_SIZE];
    self.topBannerAd.delegate = self;
    if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
      self.topBannerAd.minY = [UIApplication sharedApplication].statusBarFrame.size.height;
    self.topBannerAd.midX = self.view.width / 2;
    self.topBannerAd.hidden = YES;
    [self.topBannerAd startAutomaticallyRefreshingContents];
    [self.view addSubview:self.topBannerAd];
    [self.topBannerAd loadAd];
  }
#endif
}

- (void)tryToShowInterstitial
{
#ifdef OMIM_LITE
  AppInfo * info = [AppInfo sharedInfo];
  NSUserDefaults * userDefaults = [NSUserDefaults standardUserDefaults];
  NSDate * lastShowDate = [userDefaults objectForKey:@"BANNER_SHOW_TIME"];
  BOOL showTime = lastShowDate ? [[NSDate date] timeIntervalSinceDate:lastShowDate] > (1 * 60 * 60) : YES;
  if ([info featureAvailable:AppFeatureInterstitialAd] && info.launchCount >= 3 && showTime)
  {
    if (self.interstitialAd.ready)
    {
      [userDefaults setObject:[NSDate date] forKey:@"BANNER_SHOW_TIME"];
      [userDefaults synchronize];
      [self.interstitialAd showFromViewController:self];
    }
    else
    {
      NSString * adUnitId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"MoPubInterstitialAdUnitId"];
      self.interstitialAd = [MPInterstitialAdController interstitialAdControllerForAdUnitId:adUnitId];
      self.interstitialAd.delegate = self;
      [self.interstitialAd loadAd];
    }
  }
#endif
}

- (void)adViewDidLoadAd:(MPAdView *)view
{
  view.hidden = NO;
  view.midX = self.view.width / 2;
  [[Statistics instance] logEvent:@"Banner loaded" withParameters:@{@"Country" : [AppInfo sharedInfo].countryCode}];
}

- (void)adViewDidFailToLoadAd:(MPAdView *)view
{
  view.hidden = YES;
}

- (UIViewController *)viewControllerForPresentingModalView
{
  return self;
}

- (void)willPresentModalViewForAd:(MPAdView *)view
{
  [[Statistics instance] logEvent:@"Banner tapped" withParameters:@{@"Country" : [AppInfo sharedInfo].countryCode}];
}

- (void)interstitialDidLoadAd:(MPInterstitialAdController *)interstitial
{
  [interstitial showFromViewController:self];
  [[Statistics instance] logEvent:@"Interstitial showed" withParameters:@{@"Country" : [AppInfo sharedInfo].countryCode}];
}

- (void)interstitialDidDisappear:(MPInterstitialAdController *)interstitial
{
  [[Statistics instance] logEvent:@"Interstitial closed" withParameters:@{@"Country" : [AppInfo sharedInfo].countryCode}];
}

- (BOOL)prefersStatusBarHidden
{
  return !self.sideToolbar.isMenuHidden;
}

- (UIStatusBarAnimation)preferredStatusBarUpdateAnimation
{
  return UIStatusBarAnimationFade;
}

- (void)viewWillDisappear:(BOOL)animated
{
  GetFramework().SetUpdatesEnabled(false);

  [self.topBannerAd removeFromSuperview];
  self.topBannerAd.delegate = nil;
  self.topBannerAd = nil;

  [super viewWillDisappear:animated];
}

- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");

  if ((self = [super initWithCoder:coder]))
  {
    self.title = NSLocalizedString(@"back", @"Back button in nav bar to show the map");

    Framework & f = GetFramework();

    typedef void (*POSITIONBalloonFnT)(id,SEL, double, double);
    typedef void (*POIBalloonFnT)(id, SEL, m2::PointD const &, search::AddressInfo const &);
    typedef void (*APIPOINTBalloonFnT)(id,SEL, url_scheme::ApiPoint const &);
    typedef void (*BOOKMARKBalloonFnT)(id,SEL, BookmarkAndCategory const &);

    BalloonManager & manager = f.GetBalloonManager();

    SEL positionSel = @selector(positionBallonClickedLat:lon:);
    POSITIONBalloonFnT positionFn = (POSITIONBalloonFnT)[self methodForSelector:positionSel];
    manager.ConnectPositionListener(bind(positionFn, self, positionSel,_1,_2));

    SEL ballonPOIsel = @selector(poiBalloonClicked:info:);
    POIBalloonFnT balloonFn = (POIBalloonFnT)[self methodForSelector:ballonPOIsel];
    manager.ConnectPoiListener(bind(balloonFn, self, ballonPOIsel, _1, _2));

    SEL apiPointSel = @selector(apiBalloonClicked:);
    APIPOINTBalloonFnT apiFn = (APIPOINTBalloonFnT)[self methodForSelector:apiPointSel];
    manager.ConnectApiListener(bind(apiFn, self, apiPointSel, _1));

    SEL bookmarkSel = @selector(bookmarkBalloonClicked:);
    BOOKMARKBalloonFnT bookmarkFn = (BOOKMARKBalloonFnT)[self methodForSelector:bookmarkSel];
    manager.ConnectBookmarkListener(bind(bookmarkFn, self, bookmarkSel, _1));

    typedef void (*CompassStatusFnT)(id, SEL, int);
    SEL compassStatusSelector = @selector(onCompassStatusChanged:);
    CompassStatusFnT compassStatusFn = (CompassStatusFnT)[self methodForSelector:compassStatusSelector];

    shared_ptr<location::State> ls = f.GetLocationState();
    ls->AddCompassStatusListener(bind(compassStatusFn, self, compassStatusSelector, _1));

    m_StickyThreshold = 10;

    m_CurrentAction = NOTHING;

    EAGLView * v = (EAGLView *)self.view;
    [v initRenderPolicy];

    // restore previous screen position
    if (!f.LoadState())
      f.SetMaxWorldRect();

    _isApiMode = NO;

    f.Invalidate();
    f.LoadBookmarks();
  }

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

#pragma mark - Getters

- (LocationButton *)locationButton
{
  if (!_locationButton)
  {
    _locationButton = [[LocationButton alloc] initWithFrame:CGRectMake(0, 0, 60, 60)];
    _locationButton.center = CGPointMake(28, self.view.height - 28);
    _locationButton.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin;
    [_locationButton addTarget:self action:@selector(onMyPositionClicked:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _locationButton;
}

- (UIView *)fadeView
{
  if (!_fadeView)
  {
    _fadeView = [[UIView alloc] initWithFrame:self.view.bounds];
    _fadeView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
    _fadeView.alpha = 0;
    _fadeView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  }
  return _fadeView;
}

#define SLIDE_VIEW_DARK_PART_TAG 1

- (SideToolbar *)sideToolbar
{
  if (!_sideToolbar)
  {
    _sideToolbar = [[SideToolbar alloc] initWithFrame:CGRectMake(self.view.width, 0, 260, self.view.height)];
    _sideToolbar.delegate = self;

    UIButton * toolbarButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 50, 70)];
    toolbarButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin;
    toolbarButton.maxX = self.view.width;
    toolbarButton.midY = self.view.height - 35;
    [toolbarButton addTarget:self action:@selector(toolbarButtonPressed:) forControlEvents:UIControlEventTouchUpInside];

    CGFloat tailShift = 7;

    UIImageView * tailLight = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"SlideViewLight"]];
    tailLight.maxX = toolbarButton.width;
    tailLight.midY = toolbarButton.height / 2 + tailShift;
    [toolbarButton addSubview:tailLight];

    UIImageView * tailDark = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"SlideViewDark"]];
    tailDark.maxX = toolbarButton.width;
    tailDark.midY = toolbarButton.height / 2 + tailShift;
    tailDark.alpha = 0;
    tailDark.tag = SLIDE_VIEW_DARK_PART_TAG;
    [toolbarButton addSubview:tailDark];

    [self.view addSubview:toolbarButton];

    _sideToolbar.slideView = toolbarButton;

    [_sideToolbar addObserver:self forKeyPath:@"isMenuHidden" options:NSKeyValueObservingOptionNew context:nil];
  }
  return _sideToolbar;
}

- (UINavigationBar *)apiNavigationBar
{
  if (!_apiNavigationBar)
  {
    CGFloat height = SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 44 : 64;
    _apiNavigationBar = [[UINavigationBar alloc] initWithFrame:CGRectMake(0, 0, self.view.width, height)];
    UINavigationItem * item = [[UINavigationItem alloc] init];
    _apiNavigationBar.items = @[item];
    _apiNavigationBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _apiNavigationBar.topItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"back", nil) style: UIBarButtonItemStyleDone target:self action:@selector(returnToApiApp)];
    _apiNavigationBar.alpha = 0;
  }
  return _apiNavigationBar;
}

#pragma mark - SideToolbarDelegate

- (void)sideToolbar:(SideToolbar *)toolbar didPressButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == 0)
  {
    if (GetPlatform().IsPro())
    {
      SearchVC * vc = [[SearchVC alloc] init];
      [self.navigationController pushViewController:vc animated:YES];
    }
    else
    {
      [[UIApplication sharedApplication] openProVersion];
      [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"YES"];
    }
  }
  else if (buttonIndex == 1)
  {
    [[[MapsAppDelegate theApp] settingsManager] show:self];
  }
  else if (buttonIndex == 2)
  {
    if (GetPlatform().IsPro())
    {
      BookmarksRootVC * vc = [[BookmarksRootVC alloc] init];
      [self.navigationController pushViewController:vc animated:YES];
    }
    else
    {
      [[UIApplication sharedApplication] openProVersion];
      [[Statistics instance] logProposalReason:@"Bookmark Screen" withAnswer:@"YES"];
    }
  }
  else if (buttonIndex == 3)
  {
    SettingsViewController * vc = [self.mainStoryboard instantiateViewControllerWithIdentifier:NSStringFromClass([SettingsViewController class])];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else if (buttonIndex == 4)
  {
    CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
    if (location)
    {
      double gX = MercatorBounds::LonToX(location.coordinate.longitude);
      double gY = MercatorBounds::LatToY(location.coordinate.latitude);
      ShareInfo * info = [[ShareInfo alloc] initWithText:nil gX:gX gY:gY myPosition:YES];
      self.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self];
      [self.shareActionSheet show];
      [self.sideToolbar setMenuHidden:YES animated:YES];
    }
    else
    {
      [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"unknown_current_position", nil) message:nil delegate:nil cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil] show];
    }
  }
}

- (void)sideToolbarDidUpdateShift:(SideToolbar *)toolbar
{
  self.fadeView.alpha = toolbar.menuShift / (toolbar.maximumMenuShift - toolbar.minimumMenuShift);
}

- (void)sideToolbarDidPressBuyButton:(SideToolbar *)toolbar
{
  [[UIApplication sharedApplication] openProVersion];
}

#pragma mark - UIKitViews delegates

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  switch (alertView.tag)
  {
    case APPSTORE_ALERT_VIEW:
    {
      NSString * url = nil;
      if (GetPlatform().IsPro())
        url = [NSString stringWithFormat:ITUNES_URL, PRO_IDL];
      else
        url = [NSString stringWithFormat:ITUNES_URL, LITE_IDL];
      [self manageAlert:buttonIndex andUrl:[NSURL URLWithString: url] andDlgSetting:dlg_settings::AppStore];
      return;
    }
    case FACEBOOK_ALERT_VIEW:
    {
      NSString * url = [NSString stringWithFormat:FACEBOOK_SCHEME];
      if (![APP canOpenURL: [NSURL URLWithString: url]])
        url = [NSString stringWithFormat:FACEBOOK_URL];
      [self manageAlert:buttonIndex andUrl:[NSURL URLWithString: url] andDlgSetting:dlg_settings::FacebookDlg];
      return;
    }
    default:
      break;
  }
}

- (void)toolbarButtonPressed:(id)sender
{
  [self.sideToolbar setMenuHidden:!self.sideToolbar.isMenuHidden animated:YES];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if (object == self.sideToolbar && [keyPath isEqualToString:@"isMenuHidden"])
  {
    [UIView animateWithDuration:0.25 animations:^{
      if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)])
        [self setNeedsStatusBarAppearanceUpdate];
      self.fadeView.alpha = self.sideToolbar.isMenuHidden ? 0 : 1;
      UIView * darkTail = [self.sideToolbar.slideView viewWithTag:SLIDE_VIEW_DARK_PART_TAG];
      darkTail.alpha = self.sideToolbar.isMenuHidden ? 0 : 1;
    }];
  }
}

#pragma mark - Public methods

- (void)prepareForApi
{
  _isApiMode = YES;
  if ([self shouldShowNavBar])
  {
    self.apiNavigationBar.topItem.title = [NSString stringWithUTF8String:GetFramework().GetMapApiAppTitle().c_str()];
    [UIView animateWithDuration:0.3 animations:^{
      self.apiNavigationBar.alpha = 1;
    }];
    [self dismissPopover];
  }
}

- (void)clearApiMode
{
  _isApiMode = NO;
  [self invalidate];
  [UIView animateWithDuration:0.3 animations:^{
    self.apiNavigationBar.alpha = 0;
  }];
  Framework & f = GetFramework();
  f.ClearMapApiPoints();
  f.GetBalloonManager().Hide();
}

+ (NSURL *)getBackUrl
{
  return [NSURL URLWithString:[NSString stringWithUTF8String:GetFramework().GetMapApiBackUrl().c_str()]];
}

- (void)returnToApiApp
{
  [APP openURL:[MapViewController getBackUrl]];
  [self clearApiMode];
}

- (BOOL)shouldShowNavBar
{
  return (_isApiMode && [APP canOpenURL:[MapViewController getBackUrl]]);
}

- (void)setupMeasurementSystem
{
  GetFramework().SetupMeasurementSystem();
}

#pragma mark - Private methods

NSInteger compareAddress(id l, id r, void * context)
{
	return l < r;
}

- (void)invalidate
{
  Framework & f = GetFramework();
  if (!f.SetUpdatesEnabled(true))
    f.Invalidate();
}

- (void)pushViewController:(UIViewController *)vc
{
  if (isIPad)
  {
    NavigationController * navC = [[NavigationController alloc] init];
    m_popover = [[UIPopoverController alloc] initWithContentViewController:navC];
    m_popover.delegate = self;

    [navC pushViewController:vc animated:YES];
    navC.navigationBar.barStyle = UIBarStyleBlack;

    [m_popover setPopoverContentSize:CGSizeMake(320, 480)];
    [self showPopover];
  }
  else
  {
    [self.navigationController pushViewController:vc animated:YES];
  }
}

- (void)showAppStoreRatingMenu
{
  UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:@"App Store"
                                                       message:NSLocalizedString(@"appStore_message", nil)
                                                      delegate:self
                                             cancelButtonTitle:NSLocalizedString(@"no_thanks", nil)
                                             otherButtonTitles:NSLocalizedString(@"ok", nil), NSLocalizedString(@"remind_me_later", nil), nil];
  alertView.tag = APPSTORE_ALERT_VIEW;
  [alertView show];
}

- (void)showFacebookRatingMenu
{
  UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:@"Facebook"
                                                       message:NSLocalizedString(@"share_on_facebook_text", nil)
                                                      delegate:self
                                             cancelButtonTitle:NSLocalizedString(@"no_thanks", nil)
                                             otherButtonTitles:NSLocalizedString(@"ok", nil), NSLocalizedString(@"remind_me_later", nil),  nil];
  alertView.tag = FACEBOOK_ALERT_VIEW;
  [alertView show];
}

- (void)manageAlert:(NSInteger)buttonIndex andUrl:(NSURL *)url andDlgSetting:(dlg_settings::DialogT)set
{
  switch (buttonIndex)
  {
    case 0:
    {
      dlg_settings::SaveResult(set, dlg_settings::Never);
      break;
    }
    case 1:
    {
      dlg_settings::SaveResult(set, dlg_settings::OK);
      [APP openURL: url];
      break;
    }
    case 2:
    {
      dlg_settings::SaveResult(set, dlg_settings::Later);
      break;
    }
    default:
      break;
  }
}

- (void)destroyPopover
{
  m_popover = nil;
}

- (void)showPopover
{
  if (m_popover)
    GetFramework().GetBalloonManager().Hide();

  double const sf = self.view.contentScaleFactor;

  Framework & f = GetFramework();
  m2::PointD tmp = m2::PointD(f.GtoP(m2::PointD(m_popoverPos.x, m_popoverPos.y)));

  [m_popover presentPopoverFromRect:CGRectMake(tmp.x / sf, tmp.y / sf, 1, 1) inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
}

- (void)dismissPopover
{
  [m_popover dismissPopoverAnimated:YES];
  [self destroyPopover];
  [self invalidate];
}

@end

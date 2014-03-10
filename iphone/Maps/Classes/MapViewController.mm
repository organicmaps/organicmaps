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
#import "InAppMessagesManager.h"
#import "InterstitialView.h"
#import "MoreAppsVC.h"

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

@interface MapViewController () <SideToolbarDelegate>

@property (nonatomic) LocationButton * locationButton;
@property (nonatomic) UINavigationBar * apiNavigationBar;
@property (nonatomic) ShareActionSheet * shareActionSheet;
@property (nonatomic) UIButton * buyButton;

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

- (void)additionalLayer:(size_t)index
{
  Framework & framework = GetFramework();
  Bookmark const * bookmark = framework.AdditionalPoiLayerGetBookmark(index);

  [self.placePageView showBookmark:*bookmark];
  [self.placePageView setState:PlacePageStateBitShown animated:YES];
}

- (void)apiBalloonClicked:(url_scheme::ApiPoint const &)apiPoint
{
  [self.placePageView showApiPoint:apiPoint];
  [self.placePageView setState:PlacePageStateBitShown animated:YES];
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

}

- (void)showBalloonWithCategoryIndex:(int)cat andBookmarkIndex:(int)bm
{

}

- (void)poiBalloonDismissed
{

}

- (void)additionalLayer:(size_t)index
{
  
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
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];

  [self invalidate];

#ifdef OMIM_LITE
  InAppMessagesManager * manager = [InAppMessagesManager sharedManager];
  [manager registerController:self forMessage:InAppMessageInterstitial];
  [manager registerController:self forMessage:InAppMessageBanner];

  [manager triggerMessage:InAppMessageInterstitial];
  [manager triggerMessage:InAppMessageBanner];
#endif
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

#ifdef OMIM_LITE
  AppInfo * info = [AppInfo sharedInfo];
  if ([info featureAvailable:AppFeatureProButtonOnMap])
  {
    [self.view addSubview:self.buyButton];
    self.buyButton.minX = self.locationButton.maxX - 3;
    self.buyButton.maxY = self.view.height - 5;
    
    NSDictionary * texts = [info featureValue:AppFeatureProButtonOnMap forKey:@"Texts"];
    NSString * proText = texts[[[NSLocale preferredLanguages] firstObject]];
    if (!proText)
      proText = texts[@"*"];
    if (!proText)
      proText = [NSLocalizedString(@"become_a_pro", nil) uppercaseString];

    [self.buyButton setTitle:proText forState:UIControlStateNormal];
  }
#endif

  [self.view addSubview:self.apiNavigationBar];
  [self.view addSubview:self.locationButton];
  [self.view addSubview:self.fadeView];
  [self.view addSubview:self.sideToolbar];

  [self.sideToolbar setMenuHidden:YES animated:NO];
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

  [[InAppMessagesManager sharedManager] unregisterControllerFromAllMessages:self];

  [super viewWillDisappear:animated];
}

- (id)initWithCoder:(NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");

  if ((self = [super initWithCoder:coder]))
  {
    self.title = NSLocalizedString(@"back", @"Back button in nav bar to show the map");

    Framework & f = GetFramework();

    typedef void (*POSITIONBalloonFnT)(id, SEL, double, double);
    typedef void (*POIBalloonFnT)(id, SEL, m2::PointD const &, search::AddressInfo const &);
    typedef void (*APIPOINTBalloonFnT)(id, SEL, url_scheme::ApiPoint const &);
    typedef void (*BOOKMARKBalloonFnT)(id, SEL, BookmarkAndCategory const &);
    typedef void (*POIBalloonDismissedFnT)(id, SEL);
    typedef void (*ADDITIONALLayerFnT)(id, SEL, size_t);

    PinClickManager & manager = f.GetBalloonManager();

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

    SEL ballonPOIdismissedSel = @selector(poiBalloonDismissed);
    POIBalloonDismissedFnT balloonDismissedFn = (POIBalloonDismissedFnT)[self methodForSelector:ballonPOIdismissedSel];
    manager.ConnectDismissListener(bind(balloonDismissedFn, self, ballonPOIdismissedSel));

    SEL additionalLayerSel = @selector(additionalLayer:);
    ADDITIONALLayerFnT additionalLayerFn = (ADDITIONALLayerFnT)[self methodForSelector:additionalLayerSel];
    manager.ConnectAdditionalListener(bind(additionalLayerFn, self, additionalLayerSel, _1));

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

    f.Invalidate();
    f.LoadBookmarks();
  }

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

#pragma mark - Getters

- (PlacePageView *)placePageView
{
  if (!_placePageView)
  {
    _placePageView = [[PlacePageView alloc] initWithFrame:CGRectMake(0, 0, self.view.width, 0)];
    _placePageView.minY = self.view.height;
    _placePageView.delegate = self;
    _placePageView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    [_placePageView addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];
  }
  return _placePageView;
}

- (SearchView *)searchView
{
  if (!_searchView)
  {
    _searchView = [[SearchView alloc] initWithFrame:self.view.bounds];
    _searchView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [_searchView addObserver:self forKeyPath:@"active" options:NSKeyValueObservingOptionNew context:nil];
  }
  return _searchView;
}

#define LOCATION_BUTTON_MID_Y (self.view.height - 28)

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

- (UIButton *)buyButton
{
    if (!_buyButton)
    {
        UIImage * buyImage = [[UIImage imageNamed:@"ProVersionButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(14, 14, 14, 14)];
        _buyButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 115, 44.5)];
        _buyButton.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
        _buyButton.titleLabel.textAlignment = NSTextAlignmentCenter;
        _buyButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:16];
        _buyButton.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin;
        [_buyButton setBackgroundImage:buyImage forState:UIControlStateNormal];
        [_buyButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [_buyButton addTarget:self action:@selector(buyButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    }
    return _buyButton;
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

- (void)placePageView:(PlacePageView *)placePage willEditBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory
{
  PlacePageVC * vc = [[PlacePageVC alloc] initWithBookmark:bookmarkAndCategory];
  vc.delegate = self;
  vc.mode = PlacePageVCModeEditing;
  [self pushViewController:vc];
}

- (void)placePageView:(PlacePageView *)placePage willEditBookmarkWithInfo:(search::AddressInfo const &)addressInfo point:(m2::PointD const &)point
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

- (void)placePageView:(PlacePageView *)placePage willShareInfo:(search::AddressInfo const &)addressInfo point:(m2::PointD const &)point
{
  NSString * text = [NSString stringWithUTF8String:addressInfo.GetPinName().c_str()];
  ShareInfo * info = [[ShareInfo alloc] initWithText:text gX:point.x gY:point.y myPosition:NO];
  self.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self];
  [self.shareActionSheet show];
}

- (void)placePageVC:(PlacePageVC *)placePageVC didUpdateBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory
{
  [self.placePageView showBookmarkAndCategory:bookmarkAndCategory];
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
      [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"YES"];
      [[UIApplication sharedApplication] openProVersion];
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
      [[Statistics instance] logProposalReason:@"Bookmark Screen" withAnswer:@"YES"];
      [[UIApplication sharedApplication] openProVersion];
    }
  }
  else if (buttonIndex == 3)
  {
    SettingsViewController * vc = [self.mainStoryboard instantiateViewControllerWithIdentifier:[SettingsViewController className]];
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
  else if (buttonIndex == 5)
  {
    MoreAppsVC * vc = [[MoreAppsVC alloc] init];
    [self.navigationController pushViewController:vc animated:YES];
  }
}

- (void)sideToolbarDidUpdateShift:(SideToolbar *)toolbar
{
  self.fadeView.alpha = toolbar.menuShift / (toolbar.maximumMenuShift - toolbar.minimumMenuShift);
}

- (void)buyButtonPressed:(id)sender
{
  [[Statistics instance] logProposalReason:@"Pro button on map" withAnswer:@"YES"];
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
  else if (object == self.placePageView && [keyPath isEqualToString:@"state"])
  {
    [UIView animateWithDuration:0.5 delay:0 damping:0.8 initialVelocity:0 options:(UIViewAnimationOptionCurveEaseInOut | UIViewAnimationOptionAllowUserInteraction) animations:^{
      switch (self.placePageView.state)
      {
      case PlacePageStateHidden:
        self.sideToolbar.slideView.midY = SLIDE_VIEW_MID_Y;
        self.locationButton.midY = LOCATION_BUTTON_MID_Y;
        GetFramework().GetBalloonManager().RemovePin();
        break;
      case PlacePageStateBitShown:
        self.sideToolbar.slideView.midY = SLIDE_VIEW_MID_Y - (self.view.height - self.placePageView.minY);
        self.locationButton.midY = LOCATION_BUTTON_MID_Y - (self.view.height - self.placePageView.minY);
        break;
      case PlacePageStateOpened:
        Framework & f = GetFramework();
        CGFloat x = self.view.width / 2;
        CGFloat y = (self.searchView.searchBar.maxY + self.placePageView.minY) / 2;
        CGPoint const center = [(EAGLView *)self.view viewPoint2GlobalPoint:CGPointMake(x, y)];
        m2::PointD const offsetV = self.placePageView.pinPoint - m2::PointD(center.x, center.y);
        f.SetViewportCenterAnimated(f.GetViewportCenter() + offsetV);
        break;
      }
    } completion:nil];
  }
}

#pragma mark - Public methods

- (void)prepareForApi
{
  [self dismissPopover];
  self.searchView.searchBar.apiText = [NSString stringWithUTF8String:GetFramework().GetMapApiAppTitle().c_str()];
}

+ (NSURL *)getBackUrl
{
  return [NSURL URLWithString:[NSString stringWithUTF8String:GetFramework().GetMapApiBackUrl().c_str()]];
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
    self.popoverVC = [[UIPopoverController alloc] initWithContentViewController:navC];
    self.popoverVC.delegate = self;

    [navC pushViewController:vc animated:YES];
    navC.navigationBar.barStyle = UIBarStyleBlack;

    [self.popoverVC setPopoverContentSize:CGSizeMake(320, 480)];
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
  self.popoverVC = nil;
}

- (void)showPopover
{
  if (self.popoverVC)
    GetFramework().GetBalloonManager().Hide();

  double const sf = self.view.contentScaleFactor;

  Framework & f = GetFramework();
  m2::PointD tmp = m2::PointD(f.GtoP(m2::PointD(m_popoverPos.x, m_popoverPos.y)));

  [self.popoverVC presentPopoverFromRect:CGRectMake(tmp.x / sf, tmp.y / sf, 1, 1) inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
}

- (void)dismissPopover
{
  [self.popoverVC dismissPopoverAnimated:YES];
  [self destroyPopover];
  [self invalidate];
}

@end

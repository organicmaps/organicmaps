#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "BalloonView.h"
#import "BookmarksRootVC.h"
#import "PlacePageVC.h"
#import "GetActiveConnectionType.h"

#import "../Settings/SettingsManager.h"

#import "../../Common/CustomAlertView.h"

#include "../../../anim/controller.hpp"

#include "../../../gui/controller.hpp"

#include "../../../platform/platform.hpp"

#include "../Statistics/Statistics.h"

#include "RenderContext.hpp"

#include "../../../map/dialog_settings.hpp"

#define FACEBOOK_ALERT_VIEW 1
#define APPSTORE_ALERT_VIEW 2
#define BALLOON_PROPOSAL_ALERT_VIEW 11
#define ITUNES_URL @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?type=Purple+Software&id=%lld"
#define FACEBOOK_URL @"http://www.facebook.com/MapsWithMe"
#define FACEBOOK_SCHEME @"fb://profile/111923085594432"

const long long PRO_IDL = 510623322L;
const long long LITE_IDL = 431183278L;


@implementation MapViewController

@synthesize m_myPositionButton;


- (void) Invalidate
{
  Framework & f = GetFramework();
  if (!f.SetUpdatesEnabled(true))
    f.Invalidate();
}

//********************************************************************************************
//*********************** Callbacks from LocationManager *************************************
- (void) onLocationError:(location::TLocationError)errorCode
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
      [alert release];
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
      [alert release];
      [[MapsAppDelegate theApp].m_locationManager stop:self];
    }
    break;

  default:
    break;
  }
}

- (void) onLocationUpdate:(location::GpsInfo const &)info
{
  Framework & f = GetFramework();

  if (f.GetLocationState()->IsFirstPosition())
  {
    [m_myPositionButton setImage:[UIImage imageNamed:@"location-selected.png"] forState:UIControlStateSelected];
  }

  f.OnLocationUpdate(info);

  if (m_balloonView.isCurrentPosition && m_balloonView.isDisplayed)
  {
    m2::PointD newCenter(MercatorBounds::LonToX(info.m_longitude),
                         MercatorBounds::LatToY(info.m_latitude));
    m_balloonView.globalPosition = CGPointMake(newCenter.x, newCenter.y);
    [m_balloonView updatePosition:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
  }
  [self showPopoverFromBalloonData];
}

- (void) onCompassUpdate:(location::CompassInfo const &)info
{
  GetFramework().OnCompassUpdate(info);
}

- (void) onCompassStatusChanged:(int) newStatus
{
  Framework & f = GetFramework();
  shared_ptr<location::State> ls = f.GetLocationState();
  
  if (newStatus == location::ECompassFollow)
    [m_myPositionButton setImage:[UIImage imageNamed:@"location-follow.png"] forState:UIControlStateSelected];
  else
  {
    if (ls->HasPosition())
      [m_myPositionButton setImage:[UIImage imageNamed:@"location-selected.png"] forState:UIControlStateSelected];
    else
      [m_myPositionButton setImage:[UIImage imageNamed:@"location.png"] forState:UIControlStateSelected];
    
    m_myPositionButton.selected = YES;
  }
}

//fires when user taps on dot or arrow on the screen
-(void) onMyPosionClick:(m2::PointD const &) point
{
  Framework &f = GetFramework();
  BookmarkAndCategory const bmAndCat = f.GetBookmark(f.GtoP(point));
  if (IsValid(bmAndCat))
  {
    if (f.GetBmCategory(bmAndCat.first)->IsVisible())
    {
      [self onBookmarkClickWithBookmarkAndCategory:bmAndCat];
      return;
    }
  }
  [m_balloonView clear];
  m_balloonView.isCurrentPosition = YES;
  [m_balloonView setTitle:NSLocalizedString(@"my_position", nil)];
  m_balloonView.globalPosition = CGPointMake(point.x, point.y);
  [m_balloonView showInView:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
}

//********************************************************************************************
//********************************************************************************************

- (IBAction)OnMyPositionClicked:(id)sender
{
  Framework & f = GetFramework();
  shared_ptr<location::State> ls = f.GetLocationState();

  if (!ls->HasPosition())
  {
    if (!ls->IsFirstPosition())
    {
      m_myPositionButton.selected = YES;
      [m_myPositionButton setImage:[UIImage imageNamed:@"location-search.png"] forState:UIControlStateSelected];

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
      m_myPositionButton.selected = YES;
      return;
    }
    else
      if (GetPlatform().IsPro())
      {
        if (ls->HasCompass())
        {
          if (ls->GetCompassProcessMode() != location::ECompassFollow)
          {
            if (ls->IsCentered())
              ls->StartCompassFollowing();
            else
              ls->AnimateToPositionAndEnqueueFollowing();

            m_myPositionButton.selected = YES;
            [m_myPositionButton setImage:[UIImage imageNamed:@"location-follow.png"] forState:UIControlStateSelected];

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

  m_myPositionButton.selected = NO;
  [m_myPositionButton setImage:[UIImage imageNamed:@"location.png"] forState:UIControlStateSelected];
}

- (IBAction)OnSettingsClicked:(id)sender
{
  [[[MapsAppDelegate theApp] settingsManager] show:self];
}

- (IBAction)OnSearchClicked:(id)sender
{
  SearchVC * searchVC = [[SearchVC alloc] init];
  [self presentModalViewController:searchVC animated:YES];
  [searchVC release];
}

- (IBAction)OnBookmarksClicked:(id)sender
{
  BookmarksRootVC * bVC = [[BookmarksRootVC alloc] initWithBalloonView:m_balloonView];
  UINavigationController * navC = [[UINavigationController alloc] initWithRootViewController:bVC];
  [self presentModalViewController:navC animated:YES];
  [bVC release];
  [navC release];
}

- (void) onBalloonClicked
{
  // Disable bookmarks for free version
  if (!GetPlatform().IsPro())
  {
    // Display banner for paid version
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"bookmarks_in_pro_version", nil)
                                   message:nil
                                   delegate:self
                                   cancelButtonTitle:NSLocalizedString(@"cancel", nil)
                                   otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
    alert.tag = BALLOON_PROPOSAL_ALERT_VIEW;

    [alert show];
    [alert release];
  }
  else
  {
    PlacePageVC * placePageVC = nil;
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
      UINavigationController * navC = [[UINavigationController alloc] init];
      popover = [[UIPopoverController alloc] initWithContentViewController:navC];
      popover.delegate = self;

      placePageVC = [[PlacePageVC alloc] initWithBalloonView:m_balloonView];
      [navC pushViewController:placePageVC animated:YES];
      [navC release];

      [popover setPopoverContentSize:CGSizeMake(320, 480)];
      [self showPopoverFromBalloonData];
    }
    else
    {
      placePageVC = [[PlacePageVC alloc] initWithBalloonView:m_balloonView];
      [self.navigationController pushViewController:placePageVC animated:YES];
    }
    Framework & f = GetFramework();
    if (!IsValid(m_balloonView.editedBookmark))
    {
      int categoryPos = f.LastEditedCategory();
      [m_balloonView addBookmarkToCategory:categoryPos];
    }
    [m_balloonView hide];
    [placePageVC release];
  }
}

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController
{
  [m_balloonView addOrEditBookmark];
  [m_balloonView clear];
  [self destroyPopover];
  [self Invalidate];
}

- (void) updatePinTexts:(Framework::AddressInfo const &)info
{
  NSString * res = [NSString stringWithUTF8String:info.m_name.c_str()];

  char const * cType = info.GetBestType();
  if (cType)
  {
    NSString * type = [NSString stringWithUTF8String:cType];

    if (res.length == 0)
      res = [type capitalizedString];
    else
      res = [NSString stringWithFormat:@"%@ (%@)", res, type];
  }

  if (res.length == 0)
    res = NSLocalizedString(@"dropped_pin", nil);

  // Reset description BEFORE title, as title's setter also takes it into an account for Balloon text generation
  m_balloonView.notes = nil;
  m_balloonView.title = res;
  //m_balloonView.notes = [NSString stringWithUTF8String:info.FormatAddress().c_str()];
  //m_balloonView.type = [NSString stringWithUTF8String:info.FormatTypes().c_str()];
}


- (void) processMapClickAtPoint:(CGPoint)point longClick:(BOOL)isLongClick
{
  BOOL wasBalloonDisplayed;
  if (m_balloonView.isDisplayed)
  {
    [m_balloonView hide];
    [m_balloonView clear];
    wasBalloonDisplayed = YES;
  }
  else
    wasBalloonDisplayed = NO;

  m_balloonView.isCurrentPosition = NO;

  // Try to check if we've clicked on bookmark
  Framework & f = GetFramework();
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  // @TODO Refactor point transformation
  m2::PointD pxClicked(point.x * scaleFactor, point.y * scaleFactor);
  Framework::AddressInfo addrInfo;
  m2::PointD pxPivot;
  BookmarkAndCategory bmAndCat;
  switch (f.GetBookmarkOrPoi(pxClicked, pxPivot, addrInfo, bmAndCat))
  {
    case Framework::BOOKMARK:
      [self onBookmarkClickWithBookmarkAndCategory:bmAndCat];
      break;
    case Framework::POI:
      if (!wasBalloonDisplayed && f.GetVisiblePOI(pxClicked, pxPivot, addrInfo))
      {
        m2::PointD const gPivot = f.PtoG(pxPivot);
        m_balloonView.globalPosition = CGPointMake(gPivot.x, gPivot.y);
        [self updatePinTexts:addrInfo];
        [m_balloonView showInView:self.view atPoint:CGPointMake(pxPivot.x / scaleFactor, pxPivot.y / scaleFactor)];
      }
      break;
    default:
      if (isLongClick)
      {
        f.GetAddressInfo(pxClicked, addrInfo);
        // @TODO Refactor point transformation
        m_balloonView.globalPosition = [(EAGLView *)self.view viewPoint2GlobalPoint:point];
        [self updatePinTexts:addrInfo];
        [m_balloonView showInView:self.view atPoint:point];
      }
      break;
  }
}

- (void) onSingleTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:NO];
}

- (void) onLongTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:YES];
}

- (void)showSearchResultAsBookmarkAtMercatorPoint:(m2::PointD const &)pt withInfo:(Framework::AddressInfo const &)info
{
  [m_balloonView clear];
  m_balloonView.globalPosition = CGPointMake(pt.x, pt.y);
  m_balloonView.isCurrentPosition = NO;
  [self updatePinTexts:info];
  [m_balloonView showInView:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
}

- (void) dealloc
{
  [m_balloonView release];
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
  NSLog(@"MapViewController initWithCoder Started");

  if ((self = [super initWithCoder:coder]))
  {
    self.title = NSLocalizedString(@"back", @"Back button in nav bar to show the map");

    // Helper to display/hide pin on screen tap
    m_balloonView = [[BalloonView alloc] initWithTarget:self andSelector:@selector(onBalloonClicked)];

    // @TODO refactor cyclic dependence.
    // Here we're creating view and window handle in it, and later we should pass framework to the view.
    EAGLView * v = (EAGLView *)self.view;
    // @TODO implement balloon view in a cross-platform code
    v.balloonView = m_balloonView;

    Framework & f = GetFramework();
    
    typedef void (*onCompassStatusChangedFn)(id, SEL, int);
    SEL onCompassStatusChangedSel = @selector(onCompassStatusChanged:);
    onCompassStatusChangedFn onCompassStatusChangedImpl = (onCompassStatusChangedFn)[self methodForSelector:onCompassStatusChangedSel];

    typedef void (*onMyPositionClickedFn)(id, SEL, m2::PointD const &);
    SEL onMyPositionClickedSel = @selector(onMyPosionClick:);
    onMyPositionClickedFn onMyPosiionClickedImpl = (onMyPositionClickedFn)[self methodForSelector:onMyPositionClickedSel];

    shared_ptr<location::State> ls = f.GetLocationState();
    ls->AddCompassStatusListener(bind(onCompassStatusChangedImpl, self, onCompassStatusChangedSel, _1));
    ls->AddOnPositionClickListener(bind(onMyPosiionClickedImpl, self, onMyPositionClickedSel,_1));

    m_StickyThreshold = 10;

    m_CurrentAction = NOTHING;

    [v initRenderPolicy];

    // restore previous screen position
    if (!f.LoadState())
      f.SetMaxWorldRect();
    
    f.Invalidate();
  }

  NSLog(@"MapViewController initWithCoder Ended");
  return self;
}

NSInteger compareAddress(id l, id r, void * context)
{
	return l < r;
}

- (void) updatePointsFromEvent:(UIEvent*)event
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

- (void) stopCurrentAction
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

- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
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
}

- (void) touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
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

- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

  UITouch * theTouch = (UITouch*)[touches anyObject];
  int tapCount = theTouch.tapCount;
  int touchesCount = [[event allTouches] count];

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

- (void) touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  [self updatePointsFromEvent:event];
  [self stopCurrentAction];
}

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES; // We support all orientations
}

- (void) didReceiveMemoryWarning
{
	GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void) viewDidUnload
{
  // to correctly release view on memory warnings
  self.m_myPositionButton = nil;
  [super viewDidUnload];
}

- (void) OnTerminate
{
  GetFramework().SaveState();
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [[MapsAppDelegate theApp].m_locationManager setOrientation:self.interfaceOrientation];
  [self showPopoverFromBalloonData];
  [self Invalidate];
}

- (void) OnEnterBackground
{
  [[Statistics instance] stopSession];
  
  // Save state and notify about entering background.

  Framework & f = GetFramework();
  f.SaveState();
  f.SetUpdatesEnabled(false);
  f.EnterBackground();
}

- (void) OnEnterForeground
{
  // Notify about entering foreground (should be called on the first launch too).
  GetFramework().EnterForeground();

  if (self.isViewLoaded && self.view.window)
    [self Invalidate]; // only invalidate when map is displayed on the screen

  if (GetActiveConnectionType() != ENotConnected)
  {
    if (dlg_settings::ShouldShow(dlg_settings::AppStore))
      [self showAppStoreRatingMenu];
    else if (dlg_settings::ShouldShow(dlg_settings::FacebookDlg))
      [self showFacebookRatingMenu];
  }
}

- (void) viewWillAppear:(BOOL)animated
{
  [self Invalidate];
  [super viewWillAppear:animated];
}

- (void) viewWillDisappear:(BOOL)animated
{
  GetFramework().SetUpdatesEnabled(false);
  [super viewWillDisappear:animated];
}

- (void) SetupMeasurementSystem
{
  GetFramework().SetupMeasurementSystem();
}

-(void)onBookmarkClickWithBookmarkAndCategory:(BookmarkAndCategory const)bmAndCat
{
  Framework &f = GetFramework();
  // Already added bookmark was clicked
  BookmarkCategory * cat = f.GetBmCategory(bmAndCat.first);
  if (cat)
  {
    // Automatically reveal hidden bookmark on a click
    if (!cat->IsVisible())
    {
      // Category visibility will be autosaved after editing bookmark
      cat->SetVisible(true);
      [self Invalidate];
    }

    Bookmark const * bm = cat->GetBookmark(bmAndCat.second);
    if (bm)
    {
      m2::PointD const globalPos = bm->GetOrg();
      // Set it before changing balloon title to display different images in case of creating/editing Bookmark
      m_balloonView.editedBookmark = bmAndCat;
      m_balloonView.isCurrentPosition = NO;
      m_balloonView.globalPosition = CGPointMake(globalPos.x, globalPos.y);
      // Reset description BEFORE title, as title's setter also takes it into an account for Balloon text generation
      string const & descr = bm->GetDescription();
      if (!descr.empty())
        m_balloonView.notes = [NSString stringWithUTF8String:descr.c_str()];
      else
        m_balloonView.notes = nil;
      m_balloonView.title = [NSString stringWithUTF8String:bm->GetName().c_str()];
      m_balloonView.color = [NSString stringWithUTF8String:bm->GetType().c_str()];
      m_balloonView.setName = [NSString stringWithUTF8String:cat->GetName().c_str()];
      [m_balloonView showInView:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
    }
  }
}

- (void)showBalloonWithCategoryIndex:(int)index andBookmarkIndex:(int)bmIndex
{
  [self onBookmarkClickWithBookmarkAndCategory:BookmarkAndCategory(index, bmIndex)];
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
  [alertView release];
}

-(void)showFacebookRatingMenu
{
  UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:@"Facebook"
                                                      message:NSLocalizedString(@"share_on_facebook_text", nil)
                                                     delegate:self
                                            cancelButtonTitle:NSLocalizedString(@"no_thanks", nil)
                                            otherButtonTitles:NSLocalizedString(@"ok", nil), NSLocalizedString(@"remind_me_later", nil),  nil];
  alertView.tag = FACEBOOK_ALERT_VIEW;
  [alertView show];
  [alertView release];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  switch (alertView.tag)
  {
    case APPSTORE_ALERT_VIEW:
    {
      NSString * url = nil;
      if (GetPlatform().IsPro())
        url = [NSString stringWithFormat: ITUNES_URL, PRO_IDL];
      else
        url = [NSString stringWithFormat: ITUNES_URL, LITE_IDL];
      [self manageAlert:buttonIndex andUrl:[NSURL URLWithString: url] andDlgSetting:dlg_settings::AppStore];
      return;
    }
    case FACEBOOK_ALERT_VIEW:
    {
      NSString * url = [NSString stringWithFormat: FACEBOOK_SCHEME];
      if (![[UIApplication sharedApplication] canOpenURL: [NSURL URLWithString: url]])
        url = [NSString stringWithFormat: FACEBOOK_URL];
      [self manageAlert:buttonIndex andUrl:[NSURL URLWithString: url] andDlgSetting:dlg_settings::FacebookDlg];
      return;
    }
    default:
      break;
  }

  if (alertView.tag == BALLOON_PROPOSAL_ALERT_VIEW)
  {
    if (buttonIndex != alertView.cancelButtonIndex)
    {
      // Launch appstore
      [[UIApplication sharedApplication] openURL:[NSURL URLWithString:MAPSWITHME_PREMIUM_APPSTORE_URL]];
      [[Statistics instance] logProposalReason:@"Balloon Touch" withAnswer:@"YES"];
    }
    else
    {
      [[Statistics instance] logProposalReason:@"Balloon Touch" withAnswer:@"NO"];
    }
  }
}

-(void) manageAlert:(NSInteger)buttonIndex andUrl:(NSURL*)url andDlgSetting:(dlg_settings::DialogT)set
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
      [[UIApplication sharedApplication] openURL: url];
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

- (void)showBalloonWithText:(NSString *)text andGlobalPoint:(m2::PointD) point
{
  [m_balloonView clear];
  m_balloonView.globalPosition = CGPointMake(point.x, point.y);
  m_balloonView.title = text;
  m_balloonView.isCurrentPosition = NO;
  CGFloat const scaleFactor = self.view.contentScaleFactor;

  point = GetFramework().GtoP(point);
  [m_balloonView showInView:self.view atPoint:CGPointMake(point.x / scaleFactor, point.y / scaleFactor)];
}

- (void) destroyPopover
{
  [popover release];
  popover = nil;
}

//if save is NO, we need to delete bookmark
- (void)dismissPopoverAndSaveBookmark:(BOOL)save
{
  if (IsValid(m_balloonView.editedBookmark))
  {
    if (save)
      [m_balloonView addOrEditBookmark];
    else
      [m_balloonView deleteBookmark];
    [m_balloonView clear];
  }
  [popover dismissPopoverAnimated:YES];
  [self destroyPopover];
  [self Invalidate];
}

-(void)showPopoverFromBalloonData
{
  m2::PointD pt = GetFramework().GtoP(m2::PointD(m_balloonView.globalPosition.x, m_balloonView.globalPosition.y));
  if (IsValid(m_balloonView.editedBookmark))
    pt.y -= m_balloonView.pinImage.frame.size.height;
  //TODO We should always remember about scale factor, solve this problem
  double sf = self.view.contentScaleFactor;
  pt.x /= sf;
  pt.y /= sf;
  [popover presentPopoverFromRect:CGRectMake(pt.x, pt.y, 1, 1) inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
}

@end

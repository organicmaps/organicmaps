#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "BalloonView.h"
#import "BookmarksVC.h"
#import "PlacePageVC.h"
#import "../Settings/SettingsManager.h"
#import "../../Common/CustomAlertView.h"

#include "../../../gui/controller.hpp"

#include "Framework.h"
#include "RenderContext.hpp"

@implementation MapViewController

@synthesize m_myPositionButton;
@synthesize m_searchButton;
@synthesize m_downloadButton;
@synthesize m_bookmarksButton;

- (void) ZoomToRect: (m2::RectD const &) rect
{
  GetFramework().ShowRect(rect);
}

//********************************************************************************************
//*********************** Callbacks from LocationManager *************************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  GetFramework().OnLocationStatusChanged(newStatus);
  switch (newStatus)
  {
  case location::EDisabledByUser:
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
  case location::EFirstEvent:
      [m_myPositionButton setImage:[UIImage imageNamed:@"location-selected.png"] forState:UIControlStateSelected];
    break;
  default:
    break;
  }
}

- (void)onGpsUpdate:(location::GpsInfo const &)info
{
  GetFramework().OnGpsUpdate(info);
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  GetFramework().OnCompassUpdate(info);
}
//********************************************************************************************
//********************************************************************************************
- (IBAction)OnMyPositionClicked:(id)sender
{
  if (m_myPositionButton.isSelected == NO)
  {
    m_myPositionButton.selected = YES;
    [m_myPositionButton setImage:[UIImage imageNamed:@"location-search.png"] forState:UIControlStateSelected];
    [[MapsAppDelegate theApp] disableStandby];
    [[MapsAppDelegate theApp].m_locationManager start:self];
  }
  else
  {
    m_myPositionButton.selected = NO;
    [m_myPositionButton setImage:[UIImage imageNamed:@"location.png"] forState:UIControlStateSelected];
    [[MapsAppDelegate theApp] enableStandby];
    [[MapsAppDelegate theApp].m_locationManager stop:self];
  }
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
  BookmarksVC * bVC = [[BookmarksVC alloc] initWithBalloonView:m_bookmark];
  UINavigationController * navC = [[UINavigationController alloc] initWithRootViewController:bVC];
  [self presentModalViewController:navC animated:YES];
  [bVC release];
  [navC release];
}

- (void)onBookmarkClicked
{
  PlacePageVC * placePageVC = [[PlacePageVC alloc] initWithBalloonView:m_bookmark];
  [self.navigationController pushViewController:placePageVC animated:YES];
  [placePageVC release];
}

- (CGPoint) viewPoint2GlobalPoint:(CGPoint)pt
{
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  m2::PointD const ptG = GetFramework().PtoG(m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor));
  return CGPointMake(ptG.x, ptG.y);
}

- (CGPoint) globalPoint2ViewPoint:(CGPoint)pt
{
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  m2::PointD const ptP = GetFramework().GtoP(m2::PointD(pt.x, pt.y));
  return CGPointMake(ptP.x / scaleFactor, ptP.y / scaleFactor);
}

- (void)onSingleTap:(NSValue *)point
{
  // Try to check if we've clicked on bookmark
  CGPoint const pixelPos = [point CGPointValue];
  Bookmark const * bm = GetFramework().GetBookmark(m2::PointD(pixelPos.x, pixelPos.y));
  if (!bm)
  {
    if (m_bookmark.isDisplayed)
      [m_bookmark hide];
    else
    {
      CGPoint const globalPos = [self viewPoint2GlobalPoint:pixelPos];
      m_bookmark.globalPosition = globalPos;
      [m_bookmark showInView:self.view atPoint:pixelPos];
    }
  }
  else
  {
    // Already added bookmark was clicked
    m2::PointD const globalPos = bm->GetOrg();
    m_bookmark.globalPosition = CGPointMake(globalPos.x, globalPos.y);
    [m_bookmark showInView:self.view atPoint:[self globalPoint2ViewPoint:m_bookmark.globalPosition]];
  }
}

- (void) dealloc
{
  [m_bookmark release];
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
	if ((self = [super initWithCoder:coder]))
	{
    self.title = NSLocalizedString(@"back", @"Back button in nav bar to show the map");

    // Helper to display/hide pin on screen tap
    m_bookmark = [[BalloonView alloc] initWithTarget:self andSelector:@selector(onBookmarkClicked)];

    /// @TODO refactor cyclic dependence.
    /// Here we're creating view and window handle in it, and later we should pass framework to the view.
    EAGLView * v = (EAGLView *)self.view;

    Framework & f = GetFramework();

    f.AddString("country_status_added_to_queue", [NSLocalizedString(@"country_status_added_to_queue", @"Message to display at the center of the screen when the country is added to the downloading queue") UTF8String]);
    f.AddString("country_status_downloading", [NSLocalizedString(@"country_status_downloading", @"Message to display at the center of the screen when the country is downloading") UTF8String]);
    f.AddString("country_status_download", [NSLocalizedString(@"country_status_download", @"Button text for the button at the center of the screen when the country is not downloaded") UTF8String]);
    f.AddString("country_status_download_failed", [NSLocalizedString(@"country_status_download_failed", @"Message to display at the center of the screen when the country download has failed") UTF8String]);
    f.AddString("try_again", [NSLocalizedString(@"try_again", @"Button text for the button under the country_status_download_failed message") UTF8String]);

		m_StickyThreshold = 10;

		m_CurrentAction = NOTHING;

    // restore previous screen position
    if (!f.LoadState())
      f.SetMaxWorldRect();

    [v initRenderPolicy];

    f.Invalidate();
	}

	return self;
}

NSInteger compareAddress(id l, id r, void * context)
{
	return l < r;
}

- (void)updatePointsFromEvent:(UIEvent*)event
{
	NSSet * allTouches = [event allTouches];

  UIView * v = self.view;
  CGFloat const scaleFactor = v.contentScaleFactor;

	if ([allTouches count] == 1)
	{
		CGPoint const pt = [[[allTouches allObjects] objectAtIndex:0] locationInView:v];
		m_Pt1 = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
	}
	else
	{
		NSArray * sortedTouches = [[allTouches allObjects] sortedArrayUsingFunction:compareAddress context:NULL];
		CGPoint const pt1 = [[sortedTouches objectAtIndex:0] locationInView:v];
		CGPoint const pt2 = [[sortedTouches objectAtIndex:1] locationInView:v];

		m_Pt1 = m2::PointD(pt1.x * scaleFactor, pt1.y * scaleFactor);
	  m_Pt2 = m2::PointD(pt2.x * scaleFactor, pt2.y * scaleFactor);
	}
}

- (void)updateDataAfterScreenChanged
{
  if (m_bookmark.isDisplayed)
    [m_bookmark updatePosition:self.view atPoint:[self globalPoint2ViewPoint:m_bookmark.globalPosition]];
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

  [self updateDataAfterScreenChanged];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
  // To cancel single tap timer
  if (((UITouch *)[touches anyObject]).tapCount > 1)
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

	[self updatePointsFromEvent:event];

	if ([[event allTouches] count] == 1)
	{
    if (GetFramework().GetGuiController()->OnTapStarted(m_Pt1))
      return;
    
		GetFramework().StartDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		m_CurrentAction = DRAGGING;
	}
	else
	{
		GetFramework().StartScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
		m_CurrentAction = SCALING;
	}

	m_isSticking = true;
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	m2::PointD const TempPt1 = m_Pt1;
	m2::PointD const TempPt2 = m_Pt2;

	[self updatePointsFromEvent:event];

	bool needRedraw = false;

  if (GetFramework().GetGuiController()->OnTapMoved(m_Pt1))
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
		GetFramework().DoDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		needRedraw = true;
		break;
	case SCALING:
		if ([[event allTouches] count] < 2)
			[self stopCurrentAction];
		else
		{
			GetFramework().DoScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			needRedraw = true;
		}
		break;
	case NOTHING:
		return;
	}

  [self updateDataAfterScreenChanged];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

  UITouch * theTouch = (UITouch*)[touches anyObject];
  int tapCount = theTouch.tapCount;
  int touchesCount = [[event allTouches] count];

  if (touchesCount == 1 && tapCount == 1)
    if (GetFramework().GetGuiController()->OnTapEnded(m_Pt1))
      return;
  
  if (tapCount == 2 && touchesCount == 1 && m_isSticking)
    GetFramework().ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2.0));

  if (touchesCount == 2 && tapCount == 1 && m_isSticking)
    GetFramework().Scale(0.5);

  // Launch single tap timer
  if (touchesCount == 1 && tapCount == 1 && m_isSticking)
    [self performSelector:@selector(onSingleTap:) withObject:[NSValue valueWithCGPoint:[theTouch locationInView:self.view]] afterDelay:0.3];

  [self updateDataAfterScreenChanged];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];
}

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES; // We support all orientations
}

- (void)didReceiveMemoryWarning
{
	GetFramework().MemoryWarning();
  [super didReceiveMemoryWarning];
}

- (void)viewDidUnload
{
  // to correctly release view on memory warnings
  self.m_myPositionButton = nil;
  [super viewDidUnload];
}

- (void) OnTerminate
{
  GetFramework().SaveState();
}

- (void) Invalidate
{
  Framework & f = GetFramework();
  if (!f.SetUpdatesEnabled(true))
    f.Invalidate();
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [[MapsAppDelegate theApp].m_locationManager setOrientation:self.interfaceOrientation];
  [self Invalidate];
}

- (void) OnEnterBackground
{
  // save world rect for next launch
  Framework & f = GetFramework();
  f.SaveState();
  f.SetUpdatesEnabled(false);
  f.EnterBackground();
}

- (void) OnEnterForeground
{
  GetFramework().EnterForeground();
  if (self.isViewLoaded && self.view.window)
    [self Invalidate]; // only invalidate when map is displayed on the screen
}

- (void)viewWillAppear:(BOOL)animated
{
  [self Invalidate];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  GetFramework().SetUpdatesEnabled(false);
  [super viewWillDisappear:animated];
}

- (void) SetupMeasurementSystem
{
  GetFramework().SetupMeasurementSystem();
}

-(BOOL) OnProcessURL:(NSString*)url
{
  GetFramework().SetViewportByURL([url UTF8String]);
  return TRUE;
}

@end

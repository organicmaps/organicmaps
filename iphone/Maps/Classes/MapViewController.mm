#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "../Settings/SettingsManager.h"

#include "RenderContext.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/screen.hpp"
#include "../../map/drawer_yg.hpp"

typedef Framework<model::FeaturesFetcher> framework_t;

@implementation MapViewController

@synthesize m_myPositionButton;

// @TODO Make m_framework and m_storage MapsAppDelegate properties instead of global variables.
framework_t * m_framework = NULL;

- (void) ZoomToRect: (m2::RectD const &) rect
{
  if (m_framework)
    m_framework->ShowRect(rect);
}

//********************************************************************************************
//*********************** Callbacks from LocationManager *************************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  m_framework->OnLocationStatusChanged(newStatus);
  switch (newStatus)
  {
  case location::EDisabledByUser:
    {
      UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Location Services are disabled", @"Location services are disabled by user alert - title")
                                                       message:NSLocalizedString(@"You currently have all location services for this device or application disabled. Please, enable them in Settings Application->Location Services.", @"Location services are disabled by user alert - message")
                                                      delegate:nil 
                                             cancelButtonTitle:NSLocalizedString(@"Ok", @"Location services are disabled by user alert - close alert button")
                                             otherButtonTitles:nil];
      [alert show];
      [alert release];
      [[MapsAppDelegate theApp].m_locationManager stop:self];
    }
    break;
  case location::ENotSupported:
    {
      UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Location Services are not supported", @"Location Services are not available on the device alert - title")
                                                       message:NSLocalizedString(@"Your device doesn't support location services", @"Location Services are not available on the device alert - message")
                                                      delegate:nil
                                             cancelButtonTitle:NSLocalizedString(@"Ok", @"Location Services are not available on the device alert - close alert button")
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
  m_framework->OnGpsUpdate(info);
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  m_framework->OnCompassUpdate(info);
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
  [[[MapsAppDelegate theApp] settingsManager] Show:self WithStorage:&m_framework->Storage()];
}

- (IBAction)OnSearchClicked:(id)sender
{
  SearchVC * searchVC = [[SearchVC alloc] initWithFramework:m_framework andLocationManager:[MapsAppDelegate theApp].m_locationManager];
  [self presentModalViewController:searchVC animated:YES];
  [searchVC release];
}

- (void) dealloc
{
	delete m_framework;
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
	if ((self = [super initWithCoder:coder]))
	{
    // cyclic dependence, @TODO refactor.
    // Here we're creating view and window handle in it, and later we should pass framework to the view
    EAGLView * v = (EAGLView *)self.view;
        
    m_framework = FrameworkFactory<model::FeaturesFetcher>::CreateFramework();
    v.framework = m_framework;

		m_StickyThreshold = 10;

		m_CurrentAction = NOTHING;

    // restore previous screen position
    bool res = m_framework->LoadState();

    if (!res)
      m_framework->SetMaxWorldRect();

    [v initRenderPolicy];

    m_framework->Invalidate();
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

- (void)stopCurrentAction
{
	switch (m_CurrentAction)
	{
		case NOTHING:
			break;
		case DRAGGING:
			m_framework->StopDrag(DragEvent(m_Pt1.x, m_Pt1.y));
			break;
		case SCALING:
			m_framework->StopScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			break;
	}
	m_CurrentAction = NOTHING;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];

	if ([[event allTouches] count] == 1)
	{
		m_framework->StartDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		m_CurrentAction = DRAGGING;
	}
	else
	{
		m_framework->StartScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
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

	if (m_isSticking)
	{
		if ((TempPt1.Length(m_Pt1) > m_StickyThreshold) || (TempPt2.Length(m_Pt2) > m_StickyThreshold))
			m_isSticking = false;
		else
		{
			/// Still stickying. Restoring old points and return.
			m_Pt1 = TempPt1;
			m_Pt2 = TempPt2;
			return;
		}
	}

	switch (m_CurrentAction)
	{
	case DRAGGING:
		m_framework->DoDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		needRedraw = true;
		break;
	case SCALING:
		if ([[event allTouches] count] < 2)
			[self stopCurrentAction];
		else
		{
			m_framework->DoScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			needRedraw = true;
		}
		break;
	case NOTHING:
		return;
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

  int const tapCount = ((UITouch*)[touches anyObject]).tapCount;
  int const touchesCount = [[event allTouches] count];
  
  if (tapCount == 2 && (touchesCount == 1) && m_isSticking)
    m_framework->ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2.0));
  
  if ((touchesCount == 2) && (tapCount == 1) && m_isSticking)
    m_framework->Scale(0.5);
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

  int const tapCount = ((UITouch*)[touches anyObject]).tapCount;
  int const touchesCount = [[event allTouches] count];
  
  if (tapCount == 2 && (touchesCount == 1) && m_isSticking)
    m_framework->ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2.0));
  
  if ((touchesCount == 2) && (tapCount == 1) && m_isSticking)
    m_framework->Scale(0.5);
}

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES; // We support all orientations
}

- (void)didReceiveMemoryWarning
{
	m_framework->MemoryWarning();
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
  m_framework->SaveState();
}

- (void) Invalidate
{
  if (!m_framework->SetUpdatesEnabled(true))
    m_framework->Invalidate();
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [[MapsAppDelegate theApp].m_locationManager setOrientation:self.interfaceOrientation];
  [self Invalidate];
}

- (void) OnEnterBackground
{
  // save world rect for next launch
  m_framework->SaveState();
  m_framework->SetUpdatesEnabled(false);
  m_framework->EnterBackground();
}

- (void) OnEnterForeground
{
  m_framework->EnterForeground();
  if (self.isViewLoaded && self.view.window)
    [self Invalidate]; // only invalidate when map is displayed on the screen
}

- (void)viewWillAppear:(BOOL)animated
{
  // needed to correctly handle startup landscape orientation
  // and orientation changes when mapVC is not visible
  [self.view layoutSubviews];

  [self Invalidate];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  m_framework->SetUpdatesEnabled(false);
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  [super viewWillDisappear:animated];
}

- (void) SetupMeasurementSystem
{
  m_framework->SetupMeasurementSystem();
}

@end

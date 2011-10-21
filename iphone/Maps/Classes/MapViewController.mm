#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "WindowHandle.h"
#import "../Settings/SettingsManager.h"

#include "RenderContext.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/screen.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../storage/storage.hpp"

typedef Framework<model::FeaturesFetcher> framework_t;

@implementation MapViewController

@synthesize m_myPositionButton;

// @TODO Make m_framework and m_storage MapsAppDelegate properties instead of global variables.
framework_t * m_framework = NULL;
storage::Storage m_storage;

- (void) ZoomToRect: (m2::RectD const &) rect
{
  if (m_framework)
    m_framework->ShowRect(rect);
}

- (void)startLocation
{
  typedef void (*OnLocationUpdatedFunc)(id, SEL, location::TLocationStatus);
  SEL onLocUpdatedSel = @selector(OnLocationUpdated:);
  OnLocationUpdatedFunc locUpdatedImpl = (OnLocationUpdatedFunc)[self methodForSelector:onLocUpdatedSel];

  m_myPositionButton.selected = YES;
  [m_myPositionButton setImage:[UIImage imageNamed:@"location-search.png"] forState:UIControlStateSelected];
  [[MapsAppDelegate theApp] disableStandby];

  m_framework->StartLocationService(bind(locUpdatedImpl, self, onLocUpdatedSel, _1));
}

- (void)stopLocation
{
  m_myPositionButton.selected = NO;
  [m_myPositionButton setImage:[UIImage imageNamed:@"location.png"] forState:UIControlStateSelected];
  [[MapsAppDelegate theApp] enableStandby];

  m_framework->StopLocationService();
}

- (IBAction)OnMyPositionClicked:(id)sender
{
  if (m_myPositionButton.isSelected == NO)
    [self startLocation];
  else
    [self stopLocation];
}

- (void)OnLocationUpdated:(location::TLocationStatus) status
{
  switch (status)
  {
  case location::EDisabledByUser:
    {
      UIAlertView * alert = [[[UIAlertView alloc] initWithTitle:@"Location Services are disabled"
                                                      message:@"You currently have all location services for this device or application disabled. Please, enable them in Settings->Location Services."
                                                     delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease];
      [alert show];
      [self stopLocation];
    }
    break;
  case location::ENotSupported:
    {
      UIAlertView * alert = [[[UIAlertView alloc] initWithTitle:@"Location Services are not supported"
                                                      message:@"Your device doesn't support location services"
                                                     delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease];
      [alert show];
      [self stopLocation];
    }
    break;
  case location::ERoughMode:
  case location::EAccurateMode:
    [m_myPositionButton setImage:[UIImage imageNamed:@"location-selected.png"] forState:UIControlStateSelected];
    break;
  }
}

- (IBAction)OnSettingsClicked:(id)sender
{
  [[[MapsAppDelegate theApp] settingsManager] Show:self WithStorage:&m_storage];
}

- (IBAction)OnSearchClicked:(id)sender
{
  SearchVC * searchVC = [[[SearchVC alloc] initWithFramework:m_framework] autorelease];
  [self presentModalViewController:searchVC animated:YES];
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
    m_mapIsVisible = false;

    [(EAGLView*)self.view setController : self];

		shared_ptr<iphone::WindowHandle> windowHandle = [(EAGLView*)self.view windowHandle];
		shared_ptr<yg::ResourceManager> resourceManager = [(EAGLView*)self.view resourceManager];
        
    m_framework = FrameworkFactory<model::FeaturesFetcher>::CreateFramework(windowHandle, 40);
		m_framework->InitStorage(m_storage);
		m_StickyThreshold = 10;

		m_CurrentAction = NOTHING;

    // restore previous screen position
    bool res = m_framework->LoadState();

    if (!res)
      m_framework->SetMaxWorldRect();

    m_framework->InitializeGL([(EAGLView*)self.view renderContext], resourceManager);

    m_framework->Invalidate();
	}

	return self;
}

- (void)onResize:(GLint) width withHeight:(GLint) height
{
	m_framework->OnSize(width, height);
}

NSInteger compareAddress(id l, id r, void * context)
{
	return l < r;
}

- (void)updatePointsFromEvent:(UIEvent*)event
{
	NSSet * allTouches = [event allTouches];

  CGFloat const scaleFactor = self.view.contentScaleFactor;

	if ([allTouches count] == 1)
	{
		CGPoint const pt = [[[allTouches allObjects] objectAtIndex:0] locationInView:nil];
		m_Pt1 = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
	}
	else
	{
		NSArray * sortedTouches = [[allTouches allObjects] sortedArrayUsingFunction:compareAddress context:NULL];
		CGPoint const pt1 = [[sortedTouches objectAtIndex:0] locationInView:nil];
		CGPoint const pt2 = [[sortedTouches objectAtIndex:1] locationInView:nil];

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

- (void)drawFrame
{
  EAGLView * v = (EAGLView*)self.view;
  boost::shared_ptr<WindowHandle> wh = [v windowHandle];
  boost::shared_ptr<iphone::RenderBuffer> rb = [v renderBuffer];
  shared_ptr<DrawerYG> drawer = [v drawer];
	shared_ptr<PaintEvent> pe(new PaintEvent(drawer.get()));
  
  if (wh->needRedraw())
  {
    wh->setNeedRedraw(false);
    m_framework->BeginPaint(pe);
    m_framework->DoPaint(pe);
    rb->present();
    m_framework->EndPaint(pe);
  }
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

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
	EOrientation newOrientation = EOrientation0;
	switch (self.interfaceOrientation)
  {
		case UIInterfaceOrientationPortrait: newOrientation = EOrientation0;break;
		case UIInterfaceOrientationPortraitUpsideDown: newOrientation = EOrientation180; break;
		case UIInterfaceOrientationLandscapeLeft: newOrientation = EOrientation90; break;
		case UIInterfaceOrientationLandscapeRight: newOrientation = EOrientation270; break;
  }
	m_framework->SetOrientation(newOrientation);
}

- (void) OnTerminate
{
  if (m_framework)
    m_framework->SaveState();
}

- (void) Invalidate
{
  if (!m_framework->SetUpdatesEnabled(true))
    m_framework->Invalidate();
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
  if (m_mapIsVisible)
    [self Invalidate];
}

- (void)viewWillAppear:(BOOL)animated
{
  // needed to correctly handle startup landscape orientation
  // and orientation changes when mapVC is not visible
  [self.view layoutSubviews];

  m_mapIsVisible = true;
  [self Invalidate];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  m_mapIsVisible = false;
  m_framework->SetUpdatesEnabled(false);
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  [super viewWillDisappear:animated];
}

- (void) SetupMeasurementSystem
{
  m_framework->SetupMeasurementSystem();
}

@end

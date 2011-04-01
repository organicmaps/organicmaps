#import "MapViewController.h"
#import "GuideViewController.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "WindowHandle.h"
#import "../Settings/SettingsManager.h"
#import "IPhoneLocator.h"

#include "RenderContext.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/screen.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../storage/storage.hpp"

typedef FrameWork<model::FeaturesFetcher, Navigator> framework_t;

@implementation MapViewController

  // Make m_framework and m_storage MapsAppDelegate properties instead of global variables.
  framework_t * m_framework = NULL;
  shared_ptr<Locator> m_locator;
  storage::Storage m_storage;

- (IBAction)OnMyPositionClicked:(id)sender
{
	if (m_locator->mode() == Locator::EPreciseMode)
  {
    m_locator->setMode(Locator::ERoughMode);
    ((UIBarButtonItem *)sender).style = UIBarButtonItemStyleBordered;
  }
  else
  {
    m_locator->setMode(Locator::EPreciseMode);
    m_isDirtyPosition = true;

    ((UIBarButtonItem *)sender).style = UIBarButtonItemStyleDone;
    /// TODO : change button icon to progress indicator

    //UIActivityIndicatorView * indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];

    //((UIBarButtonItem *)sender).customView = indicator;
    //[indicator release];
  }
}

- (void) OnChangeLocatorMode:(Locator::EMode) oldMode
                 withNewMode:(Locator::EMode) newMode
{
  if (newMode == Locator::ERoughMode)
  {
    /// TODO : change button icon to "rough mode"(UIBarButtonItemStyleBordered);
  }
}

- (void) OnUpdateLocation: (m2::PointD const &) mercatorPoint
          withErrorRadius: (double) errorRadius
         withLocTimeStamp: (double) locTimeStamp
         withCurTimeStamp: (double) curTimeStamp
{
  if (m_isDirtyPosition)
  {
    /// TODO : change button icon to "precise" mode(UIBarButtonItemStyleDone).
  }
}

- (IBAction)OnSettingsClicked:(id)sender
{
  m_framework->SetUpdatesEnabled(false);
  [[[MapsAppDelegate theApp] settingsManager] Show:self WithStorage:&m_storage];
}

- (IBAction)OnGuideClicked:(id)sender
{
  m_framework->SetUpdatesEnabled(false);
  UIView * guideView = [MapsAppDelegate theApp].guideViewController.view;
  [guideView setFrame:self.view.frame];
  [UIView transitionFromView:self.view
                      toView:guideView
                    duration:0
                     options:UIViewAnimationOptionTransitionNone
                  completion:nil];
  [(UISegmentedControl *)sender setSelectedSegmentIndex:0];
}

- (void) dealloc
{
  m_locator.reset();
	delete m_framework;
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
	if ((self = [super initWithCoder:coder]))
	{
		[(EAGLView*)self.view setController : self];

		shared_ptr<iphone::WindowHandle> windowHandle = [(EAGLView*)self.view windowHandle];
		shared_ptr<yg::ResourceManager> resourceManager = [(EAGLView*)self.view resourceManager];
		
    m_locator = shared_ptr<Locator>(new iphone::Locator());
    
    // tricky boost::bind for objC class methods
		typedef void (*TUpdateLocationFunc)(id, SEL, m2::PointD const &, double, double, double);
		SEL updateLocationSel = @selector(OnUpdateLocation:withErrorRadius:withLocTimeStamp:withCurTimeStamp:);
		TUpdateLocationFunc updateLocationImpl = (TUpdateLocationFunc)[self methodForSelector:updateLocationSel];
    
    typedef void (*TChangeModeFunc)(id, SEL, Locator::EMode, Locator::EMode);
    SEL changeModeSel = @selector(OnChangeLocatorMode:withNewMode:);
    TChangeModeFunc changeModeImpl = (TChangeModeFunc)[self methodForSelector:changeModeSel];

    m_locator->addOnUpdateLocationFn(boost::bind(updateLocationImpl, self, updateLocationSel, _1, _2, _3, _4));
    m_locator->addOnChangeModeFn(boost::bind(changeModeImpl, self, changeModeSel, _1, _2));
    
    m_framework = new framework_t(windowHandle, 40);
		m_framework->InitStorage(m_storage);
    m_framework->InitLocator(m_locator);
		m_StickyThreshold = 10;

		m_CurrentAction = NOTHING;
    m_isDirtyPosition = false;

		// initialize with currently active screen orientation
    [self didRotateFromInterfaceOrientation: self.interfaceOrientation];

    // to perform a proper resize
    [(EAGLView*)self.view layoutSubviews];
    // restore previous screen position
    bool res = m_framework->LoadState();

    m_framework->initializeGL([(EAGLView*)self.view renderContext], resourceManager);

    if (!res)
      m_framework->ShowAll();
    else
      m_framework->UpdateNow();
	}

	return self;
}

- (void)onResize:(GLint) width withHeight:(GLint) height
{
	UIInterfaceOrientation orientation = [self interfaceOrientation];
	if ((orientation == UIInterfaceOrientationLandscapeLeft)
		||(orientation == UIInterfaceOrientationLandscapeRight))
		std::swap(width, height);
	NSLog(@"onResize: %d, %d", width, height);
	m_framework->OnSize(width, height);
}

NSInteger compareAddress(UITouch * l, UITouch * r, void * context)
{
	return l < r;
}

- (void)updatePointsFromEvent:(UIEvent*)event
{
	NSSet * allTouches = [event allTouches];
	int touchCount = [allTouches count];

  CGFloat scaleFactor = 1.0;
  if ([self.view respondsToSelector:@selector(contentScaleFactor)])
  	scaleFactor = self.view.contentScaleFactor;

	if (touchCount == 1)
	{
		CGPoint pt = [[[allTouches allObjects] objectAtIndex:0] locationInView:nil];
		m_Pt1 = m2::PointD(pt.x * scaleFactor, pt.y * scaleFactor);
	}
	else
	{
		NSArray * sortedTouches = [[allTouches allObjects] sortedArrayUsingFunction:compareAddress context:NULL];
		CGPoint pt1 = [[sortedTouches objectAtIndex:0] locationInView:nil];
		CGPoint pt2 = [[sortedTouches objectAtIndex:1] locationInView:nil];

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
	int touchCount = [[event allTouches] count];
	// NSLog(@"touchesBeg %i", touchCount);
	if (touchCount == 1)
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
	m2::PointD TempPt1 = m_Pt1;
	m2::PointD TempPt2 = m_Pt2;

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

	if ((((UITouch*)[touches anyObject]).tapCount == 2) && ([[event allTouches] count] < 2))
		m_framework->ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2));
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];

	if ((((UITouch*)[touches anyObject]).tapCount == 2) && ([[event allTouches] count] < 2))
		m_framework->ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2));
}

- (void)onPaint
{
	shared_ptr<iphone::WindowHandle> windowHandle = [(EAGLView*)self.view windowHandle];
	shared_ptr<PaintEvent> paintEvent(new PaintEvent(windowHandle->drawer()));
	m_framework->Paint(paintEvent);
}


/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
  [super viewDidLoad];
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES;
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];

	m_framework->MemoryWarning();
//	m_framework->Repaint();

	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload
{
  // Release any retained subviews of the main view.
  // e.g. self.myOutlet = nil;
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
	if (m_framework)
	{
        if (!m_framework->SetUpdatesEnabled(true))
            m_framework->Invalidate();
	}
}

- (void) OnEnterBackground
{
	if (m_framework)
  {	// save world rect for next launch
  	m_framework->SetUpdatesEnabled(false);
    m_framework->SaveState();
    m_framework->EnterBackground();
  }
}

- (void) OnEnterForeground
{
  if (m_framework)
  {
    m_framework->EnterForeground();
    if (!m_framework->SetUpdatesEnabled(true))
      m_framework->Invalidate();
  }
}


@end

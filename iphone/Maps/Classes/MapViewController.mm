#import "MapViewController.h"
#import "SearchVC.h"
#import "MapsAppDelegate.h"
#import "EAGLView.h"
#import "BalloonView.h"
#import "BookmarksRootVC.h"
#import "PlacePageVC.h"

#import "../Settings/SettingsManager.h"

#import "../../Common/CustomAlertView.h"

#include "../../../anim/controller.hpp"

#include "../../../gui/controller.hpp"

#include "../../../platform/platform.hpp"

#include "RenderContext.hpp"

@implementation MapViewController

@synthesize m_myPositionButton;


- (void) Invalidate
{
  Framework & f = GetFramework();
  if (!f.SetUpdatesEnabled(true))
    f.Invalidate();
  [self addGestures];
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

// Banner dialog handler
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    // Launch appstore
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:MAPSWITHME_PREMIUM_APPSTORE_URL]];
  }
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

    [alert show];
    [alert release];
  }
  else
  {
    PlacePageVC * placePageVC = [[PlacePageVC alloc] initWithBalloonView:m_balloonView];
    [self.navigationController pushViewController:placePageVC animated:YES];
    [placePageVC release];
  }
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

  m_balloonView.title = res;
  //m_balloonView.description = [NSString stringWithUTF8String:info.FormatAddress().c_str()];
  //m_balloonView.type = [NSString stringWithUTF8String:info.FormatTypes().c_str()];
}


- (void) processMapClickAtPoint:(CGPoint)point longClick:(BOOL)isLongClick
{
  if (m_balloonView.isDisplayed)
  {
    [m_balloonView hide];
//    if (!isLongClick)
//      return;
  }

  // Try to check if we've clicked on bookmark
  Framework & f = GetFramework();
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  // @TODO Refactor point transformation
  m2::PointD pxClicked(point.x * scaleFactor, point.y * scaleFactor);

  BookmarkAndCategory const bmAndCat = f.GetBookmark(pxClicked);
  if (IsValid(bmAndCat))
  {
    [self onBookmarkClickWithBookmarkAndCategory:bmAndCat];
  }
  else
  {
    // Check if we've clicked on visible POI
    Framework::AddressInfo addrInfo;
    m2::PointD pxPivot;
    if (f.GetVisiblePOI(pxClicked, pxPivot, addrInfo))
    {
      m2::PointD const gPivot = f.PtoG(pxPivot);
      m_balloonView.globalPosition = CGPointMake(gPivot.x, gPivot.y);
      [self updatePinTexts:addrInfo];
      m_balloonView.isCurrentPosition = NO;
      [m_balloonView showInView:self.view atPoint:CGPointMake(pxPivot.x / scaleFactor, pxPivot.y / scaleFactor)];
    }
    else
    {
      // Just a click somewhere on a map
      if (isLongClick)
      {
        f.GetAddressInfo(pxClicked, addrInfo);
        // @TODO Refactor point transformation
        m_balloonView.globalPosition = [(EAGLView *)self.view viewPoint2GlobalPoint:point];
        [self updatePinTexts:addrInfo];
        [m_balloonView showInView:self.view atPoint:point];
      }
    }
  }
}

- (void)showSearchResultAsBookmarkAtMercatorPoint:(m2::PointD const &)pt withInfo:(Framework::AddressInfo const &)info
{
  m_balloonView.globalPosition = CGPointMake(pt.x, pt.y);
  m_balloonView.isCurrentPosition = NO;
  [self updatePinTexts:info];
  [m_balloonView showInView:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
}

- (void) onSingleTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:NO];
}

- (void) onLongTap:(NSValue *)point
{
  [self processMapClickAtPoint:[point CGPointValue] longClick:YES];
}

- (void) dealloc
{
  [m_balloonView release];
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
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

    [v initRenderPolicy];

    // restore previous screen position
    if (!f.LoadState())
      f.SetMaxWorldRect();
    
    f.Invalidate();
	}

	return self;
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
  [self Invalidate];
}

- (void) OnEnterBackground
{
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

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
  return YES;
}

-(void)addGestures
{
  lastRotateTime = 0;
  startedScaling = NO;
  UITapGestureRecognizer *singleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
  [singleTap setNumberOfTouchesRequired:1];
  [singleTap setNumberOfTapsRequired:1];

  UITapGestureRecognizer *twoTouches = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoSingleTouches:)];
  [twoTouches setNumberOfTouchesRequired:2];
  [twoTouches setNumberOfTapsRequired:1];

  UITapGestureRecognizer *twoTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoTaps:)];
  [twoTap setNumberOfTouchesRequired:1];
  [twoTap setNumberOfTapsRequired:2];

  UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
  [longPress setMinimumPressDuration:1.0];

  UIPanGestureRecognizer * pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
  [pan setMaximumNumberOfTouches:1];
  [pan setDelegate:self];

  UIRotationGestureRecognizer * rotate = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotate:)];
  [rotate setDelegate:self];
  UIPinchGestureRecognizer * scale = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotate:)];
  [scale setDelegate:self];

  [singleTap requireGestureRecognizerToFail:twoTap];
  [singleTap requireGestureRecognizerToFail:longPress];

  [self.view addGestureRecognizer:singleTap];
  [self.view addGestureRecognizer:twoTouches];
  [self.view addGestureRecognizer:twoTap];
  [self.view addGestureRecognizer:longPress];
  [self.view addGestureRecognizer:pan];
  [self.view addGestureRecognizer:rotate];
  [self.view addGestureRecognizer:scale];

  [singleTap release];
  [twoTouches release];
  [twoTap release];
  [longPress release];
  [pan release];
  [rotate release];
  [scale release];
}

-(void)handleSingleTap:(UITapGestureRecognizer *)recognizer
{
  if (CFAbsoluteTimeGetCurrent() - lastRotateTime < 0.1)
    return;
  Framework & f = GetFramework();

  CGPoint location = [self pointFromRecognizerWithScaleFactor:recognizer];
  m2::PointD point = m2::PointD(location.x, location.y);

  if (f.GetGuiController()->OnTapStarted(point))
  {
    f.GetGuiController()->OnTapStarted(point);
    f.GetGuiController()->OnTapMoved(point);
    f.GetGuiController()->OnTapEnded(point);
    return;
  }
  [self processMapClickAtPoint:[recognizer locationInView:self.view] longClick:NO];
}

-(void)handleTwoSingleTouches:(UITapGestureRecognizer *)recognizer
{
  Framework &f = GetFramework();
  if (CFAbsoluteTimeGetCurrent() - lastRotateTime > 0.1)
  {
    f.Scale(0.5);
  }
}

-(void)handleTwoTaps:(UITapGestureRecognizer*)recognizer
{
  if (recognizer.state == UIGestureRecognizerStateEnded)
  {
    Framework &f = GetFramework();
    CGPoint location = [self pointFromRecognizerWithScaleFactor:recognizer];
    f.ScaleToPoint(ScaleToPointEvent(location.x, location.y, 2.0));
  }
}

-(void)handleLongPress:(UILongPressGestureRecognizer*)recognizer
{
  if (recognizer.state == UIGestureRecognizerStateBegan)
  {
    [self processMapClickAtPoint:[recognizer locationInView:self.view] longClick:YES];
  }
}

-(void)handlePan:(UIPanGestureRecognizer *)recognizer
{
  if (CFAbsoluteTimeGetCurrent() - lastRotateTime < 0.1 ||
      (recognizer.numberOfTouches == 0 && recognizer.state != UIGestureRecognizerStateEnded))
    return;
  CGPoint p = [self pointFromRecognizerWithScaleFactor:recognizer];
  Framework &f = GetFramework();
  if (recognizer.state == UIGestureRecognizerStateBegan)
  {
    f.StartDrag(DragEvent(p.x, p.y));
  }
  else if (recognizer.state == UIGestureRecognizerStateChanged)
  {
    f.DoDrag(DragEvent(p.x, p.y));
  }
  else
  {
    f.StopDrag(DragEvent(p.x, p.y));
  }
}

-(void)handleRotate:(UIRotationGestureRecognizer *)recognizer
{
  Framework &f = GetFramework();
  if (recognizer.numberOfTouches < 2)
  {
    if (recognizer.state == UIGestureRecognizerStateChanged)
    {
        if (startedScaling)
        {
          f.StopScale(ScaleEvent(p1.x, p1.y, p2.x, p2.y));
          startedScaling = NO;
        }
    }
    if (recognizer.state != UIGestureRecognizerStateEnded)
    {
      return;
    }
  }

  //Work of Scale Recognizer
  [self adjustPoints:recognizer];
  if (recognizer.state == UIGestureRecognizerStateBegan)
  {
    [self checkForScalingAndStart];
  }
  else if (recognizer.state == UIGestureRecognizerStateChanged)
  {
    [self checkForScalingAndStart];
    f.DoScale(ScaleEvent(p1.x, p1.y, p2.x, p2.y));
  }
  else
  {
    if (startedScaling)
    {
      startedScaling = NO;
      f.StopScale(ScaleEvent(p1.x, p1.y, p2.x, p2.y));
    }
  }
  lastRotateTime = CFAbsoluteTimeGetCurrent();
}

-(void)checkForScalingAndStart
{
  if (!startedScaling)
  {
    startedScaling = YES;
    GetFramework().StartScale(ScaleEvent(p1.x, p1.y, p2.x, p2.y));
  }
}

-(void)adjustPoints:(UIGestureRecognizer *)recognizer
{
  if (recognizer.numberOfTouches >1 )
  {
    p1 = [recognizer locationOfTouch:0 inView:self.view];
    p2 = [recognizer locationOfTouch:1 inView:self.view];

    CGFloat const scaleFactor = self.view.contentScaleFactor;
    p1.x *= scaleFactor;
    p1.y *= scaleFactor;
    p2.x *= scaleFactor;
    p2.y *= scaleFactor;
  }
}

-(CGPoint)pointFromRecognizerWithScaleFactor:(UIGestureRecognizer *)recognizer
{
  CGPoint point = [recognizer locationInView:self.view];
  CGFloat const scaleFactor = self.view.contentScaleFactor;
  point.x *= scaleFactor;
  point.y *= scaleFactor;

  return point;
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
      m_balloonView.title = [NSString stringWithUTF8String:bm->GetName().c_str()];
      m_balloonView.color = [NSString stringWithUTF8String:bm->GetType().c_str()];
      m_balloonView.setName = [NSString stringWithUTF8String:cat->GetName().c_str()];
      [m_balloonView showInView:self.view atPoint:[(EAGLView *)self.view globalPoint2ViewPoint:m_balloonView.globalPosition]];
    }
  }
}

@end

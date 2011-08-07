#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "GuideViewController.h"
#import "SettingsManager.h"
#import "../../Sloynik/Shared/global.hpp"

@implementation MapsAppDelegate

@synthesize window;
@synthesize mapViewController;

+ (MapsAppDelegate *) theApp
{
  return [[UIApplication sharedApplication] delegate];
}

// here we're
- (void) applicationWillTerminate: (UIApplication *) application
{
	[mapViewController OnTerminate];
}

- (void) applicationDidEnterBackground: (UIApplication *) application
{
	[mapViewController OnEnterBackground];
}

- (void) applicationWillEnterForeground: (UIApplication *) application
{
  [mapViewController OnEnterForeground];
}

- (void) onSloynikEngineInitialized: (void *) pEngine
{
  SetSloynikEngine(static_cast<sl::SloynikEngine *>(pEngine));
  if (m_guideViewController)
    [m_guideViewController OnSloynikEngineInitialized];
}

- (void) applicationDidFinishLaunching: (UIApplication *) application
{
  // Initialize Sloynik engine.
  // It takes long for the first time, so we do it while startup image is visible.
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
    sl::SloynikEngine * pEngine = CreateSloynikEngine();
    dispatch_async(dispatch_get_main_queue(), ^{
      [[MapsAppDelegate theApp] onSloynikEngineInitialized:pEngine];
    });
  });


  // Add the tab bar controller's current view as a subview of the window
  [window addSubview:mapViewController.view];
  [window makeKeyAndVisible];
}

- (GuideViewController *)guideViewController
{
  if (!m_guideViewController)
  {
    m_guideViewController = [GuideViewController alloc];
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
      m_guideViewController = [m_guideViewController initWithNibName:@"GuideView-iPad" bundle:nil];
    else
      m_guideViewController = [m_guideViewController initWithNibName:@"GuideView" bundle:nil];
  }
  return m_guideViewController;
}

- (SettingsManager *)settingsManager
{
  if (!m_settingsManager)
    m_settingsManager = [[SettingsManager alloc] init];
  return m_settingsManager;
}

- (void) dealloc
{
  [m_guideViewController release];
  [m_settingsManager release];
  mapViewController = nil;
  window = nil;
  [super dealloc];
}

- (void) disableStandby
{
  ++m_standbyCounter;
  [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void) enableStandby
{
  --m_standbyCounter;
  if (m_standbyCounter == 0)
    [UIApplication sharedApplication].idleTimerDisabled = NO;
}

@end


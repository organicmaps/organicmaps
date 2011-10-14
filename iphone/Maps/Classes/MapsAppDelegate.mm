#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "SettingsManager.h"
#import "Preferences.h"

@implementation MapsAppDelegate

@synthesize m_navigationController;
@synthesize m_window;
@synthesize m_mapViewController;

+ (MapsAppDelegate *) theApp
{
  return [[UIApplication sharedApplication] delegate];
}

- (void) applicationWillTerminate: (UIApplication *) application
{
	[m_mapViewController OnTerminate];
}

- (void) applicationDidEnterBackground: (UIApplication *) application
{
	[m_mapViewController OnEnterBackground];
}

- (void) applicationWillEnterForeground: (UIApplication *) application
{
  [m_mapViewController OnEnterForeground];
}

- (void) applicationDidFinishLaunching: (UIApplication *) application
{
  [Preferences setup];

  [m_window addSubview:m_navigationController.view];
  [m_window makeKeyAndVisible];
}

- (SettingsManager *)settingsManager
{
  if (!m_settingsManager)
    m_settingsManager = [[SettingsManager alloc] init];
  return m_settingsManager;
}

- (void) dealloc
{
  [m_settingsManager release];
  m_mapViewController = nil;
  m_window = nil;
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

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
  NSLog(@"applicationDidReceiveMemoryWarning");
}

@end

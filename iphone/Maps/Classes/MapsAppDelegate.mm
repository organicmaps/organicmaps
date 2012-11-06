#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "SettingsManager.h"
#import "Preferences.h"
#import "LocationManager.h"

#include <sys/xattr.h>

#include "Framework.h"
#include "../../../platform/settings.hpp"

@implementation MapsAppDelegate

@synthesize m_mapViewController;
@synthesize m_locationManager;


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

- (SettingsManager *)settingsManager
{
  if (!m_settingsManager)
    m_settingsManager = [[SettingsManager alloc] init];
  return m_settingsManager;
}

- (void) dealloc
{
  [m_locationManager release];
  [m_settingsManager release];
  self.m_mapViewController = nil;
  [m_navController release];
  [m_window release];
  [super dealloc];

  // Global cleanup
  DeleteFramework();
}

- (void) disableStandby
{
  ++m_standbyCounter;
  [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void) enableStandby
{
  --m_standbyCounter;
  if (m_standbyCounter <= 0)
  {
    [UIApplication sharedApplication].idleTimerDisabled = NO;
    m_standbyCounter = 0;
  }
}

- (void) disableDownloadIndicator
{
  --m_activeDownloadsCounter;
  if (m_activeDownloadsCounter <= 0)
  {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    m_activeDownloadsCounter = 0;
  }
}

- (void) enableDownloadIndicator
{
  ++m_activeDownloadsCounter;
  [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}

// We process memory warnings in MapsViewController
//- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
//{
//  NSLog(@"applicationDidReceiveMemoryWarning");
//}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  [m_mapViewController OnEnterForeground];

  [Preferences setup:m_mapViewController];
  m_locationManager = [[LocationManager alloc] init];

  m_navController = [[UINavigationController alloc] initWithRootViewController:m_mapViewController];
  m_navController.navigationBarHidden = YES;
  m_window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  m_window.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  m_window.clearsContextBeforeDrawing = NO;
  m_window.multipleTouchEnabled = YES;
  [m_window setRootViewController:m_navController];
  [m_window makeKeyAndVisible];

  return [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey] != nil;
}

// handleOpenURL is deprecaed now.
// http://stackoverflow.com/questions/3612460/lauching-app-with-url-via-uiapplicationdelegates-handleopenurl-working-under
/*
- (BOOL)application:(UIApplication *)application handleOpenURL:(NSURL *)url
{
  NSString * text = [[url absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  return [m_mapViewController OnProcessURL:text];
}
*/

@end

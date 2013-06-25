#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "SettingsManager.h"
#import "Preferences.h"
#import "LocationManager.h"
#import "Statistics.h"

#include <sys/xattr.h>

#include "Framework.h"
#include "../../../platform/settings.hpp"
#include "../../../platform/platform.hpp"

/// Adds needed localized strings to C++ code
/// @TODO Refactor localization mechanism to make it simpler
void InitLocalizedStrings()
{
  Framework & f = GetFramework();
  // Texts on the map screen when map is not downloaded or is downloading
  f.AddString("country_status_added_to_queue", [NSLocalizedString(@"country_status_added_to_queue", @"Message to display at the center of the screen when the country is added to the downloading queue") UTF8String]);
  f.AddString("country_status_downloading", [NSLocalizedString(@"country_status_downloading", @"Message to display at the center of the screen when the country is downloading") UTF8String]);
  f.AddString("country_status_download", [NSLocalizedString(@"country_status_download", @"Button text for the button at the center of the screen when the country is not downloaded") UTF8String]);
  f.AddString("country_status_download_failed", [NSLocalizedString(@"country_status_download_failed", @"Message to display at the center of the screen when the country download has failed") UTF8String]);
  f.AddString("try_again", [NSLocalizedString(@"try_again", @"Button text for the button under the country_status_download_failed message") UTF8String]);
  // Default texts for bookmarks added in C++ code (by URL Scheme API)
  f.AddString("dropped_pin", [NSLocalizedString(@"dropped_pin", nil) UTF8String]);
  f.AddString("my_places", [NSLocalizedString(@"my_places", nil) UTF8String]);
}

@implementation MapsAppDelegate

@synthesize m_mapViewController;
@synthesize m_locationManager;


+ (MapsAppDelegate *) theApp
{
  return (MapsAppDelegate *)[[UIApplication sharedApplication] delegate];
}

- (void) applicationWillTerminate: (UIApplication *) application
{
	[m_mapViewController OnTerminate];
}

- (void) applicationDidEnterBackground: (UIApplication *) application
{
	[m_mapViewController OnEnterBackground];
  if(m_activeDownloadsCounter)
  {
    m_backgroundTask = [application beginBackgroundTaskWithExpirationHandler:^{
      [application endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }];
  }
}

- (void) applicationWillEnterForeground: (UIApplication *) application
{
  [m_mapViewController OnEnterForeground];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  UIPasteboard * pasteboard = [UIPasteboard generalPasteboard];
  if (GetPlatform().IsPro() && !m_didOpenedWithUrl)
  {
    NSString * url = [pasteboard.string stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    if ([url length])
    {
      url_scheme::ApiPoint apiPoint;
      if (GetFramework().SetViewportByURL([url UTF8String], apiPoint))
      {
        [self showParsedBookmarkOnMap:apiPoint];
        pasteboard.string = @"";
      }
    }
  }
  m_didOpenedWithUrl = NO;
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
    if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateBackground)
    {
      [[UIApplication sharedApplication] endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }
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
  NSLog(@"application didFinishLaunchingWithOptions");
  [[Statistics instance] startSession];

  InitLocalizedStrings();

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

  m_didOpenedWithUrl = NO;

  return [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey] != nil;
}

// We don't support HandleOpenUrl as it's deprecated from iOS 4.2
- (BOOL)application:(UIApplication *)application
            openURL:(NSURL *)url
  sourceApplication:(NSString *)sourceApplication
         annotation:(id)annotation
{
  NSString * scheme = url.scheme;
  // geo scheme support, see http://tools.ietf.org/html/rfc5870
  if ([scheme isEqualToString:@"geo"] || [scheme isEqualToString:@"ge0"])
  {
    url_scheme::ApiPoint apiPoint;
    if (GetFramework().SetViewportByURL([[url.absoluteString stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String], apiPoint))
    {
      [self showParsedBookmarkOnMap:apiPoint];
      m_didOpenedWithUrl = YES;
      if ([scheme isEqualToString:@"geo"])
        [[Statistics instance] logEvent:@"geo Import"];
      if ([scheme isEqualToString:@"ge0"])
        [[Statistics instance] logEvent:@"ge0(zero) Import"];
      return YES;
    }
  }
  if ([scheme isEqualToString:@"mapswithme"])
  {
    url_scheme::ApiPoint apiPoint;
    if (GetFramework().SetViewportByURL([[url.absoluteString stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String], apiPoint));
    {
      [self showMap];
      [m_mapViewController prepareForApi];
      return YES;
    }
  }

  if ([scheme isEqualToString:@"file"])
  {
     if (!GetFramework().AddBookmarksFile([[url relativePath] UTF8String]))
     {
       [self showLoadFileAlertIsSuccessful:NO];
       return NO;
     }
     [[NSNotificationCenter defaultCenter] postNotificationName:@"KML file added" object:nil];
     [self showLoadFileAlertIsSuccessful:YES];
     m_didOpenedWithUrl = YES;
     [[Statistics instance] logEvent:@"KML Import"];
     return YES;
  }
  NSLog(@"Scheme %@ is not supported", scheme);
  return NO;
}

-(void)showLoadFileAlertIsSuccessful:(BOOL) successful
{
  m_loadingAlertView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"load_kmz_title", nil)
                                                  message:
                        (successful ? NSLocalizedString(@"load_kmz_successful", nil) : NSLocalizedString(@"load_kmz_failed", nil))
                                                 delegate:nil
                                      cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil];
  m_loadingAlertView.delegate = self;
  [m_loadingAlertView show];
  [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(dismissAlert) userInfo:nil repeats:NO];
  [m_loadingAlertView release];
}

-(void)dismissAlert
{
  if (m_loadingAlertView)
    [m_loadingAlertView dismissWithClickedButtonIndex:0 animated:YES];
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  m_loadingAlertView = nil;
}

-(void)showMap
{
  [m_navController popToRootViewControllerAnimated:YES];
  if (![m_navController.visibleViewController isMemberOfClass:NSClassFromString(@"MapViewController")])
    [m_mapViewController dismissModalViewControllerAnimated:YES];
  [m_mapViewController dismissPopoverAndSaveBookmark:YES];
  m_navController.navigationBarHidden = YES;
}

-(void) showParsedBookmarkOnMap:(url_scheme::ApiPoint const &)apiPoint
{
  [self showMap];
  m2::PointD const globalPoint(MercatorBounds::LonToX(apiPoint.m_lon), MercatorBounds::LatToY(apiPoint.m_lat));
  NSString * name = [NSString stringWithUTF8String:apiPoint.m_name.c_str()];
  [m_mapViewController showBalloonWithText:name andGlobalPoint:globalPoint];
}

@end

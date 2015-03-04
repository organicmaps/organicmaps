#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "Preferences.h"
#import "LocationManager.h"
#import "Statistics.h"
#import "UIKitCategories.h"
#import "AppInfo.h"
#import "LocalNotificationManager.h"

#include <sys/xattr.h>

#import <MRGService/MRGService.h>
#import <FacebookSDK/FacebookSDK.h>

#include "../../../storage/storage_defines.hpp"

#include "../../../platform/settings.hpp"
#include "../../../platform/platform.hpp"
#include "../../../platform/preferred_languages.hpp"

#import "../../../3party/Alohalytics/src/alohalytics_objc.h"

NSString * const MapsStatusChangedNotification = @"MapsStatusChangedNotification";

/// Adds needed localized strings to C++ code
/// @TODO Refactor localization mechanism to make it simpler
void InitLocalizedStrings()
{
  Framework & f = GetFramework();
  // Texts on the map screen when map is not downloaded or is downloading
  f.AddString("country_status_added_to_queue", [L(@"country_status_added_to_queue") UTF8String]);
  f.AddString("country_status_downloading", [L(@"country_status_downloading") UTF8String]);
  f.AddString("country_status_download_routing", [L(@"country_status_download_routing") UTF8String]);
  f.AddString("country_status_download", [L(@"country_status_download") UTF8String]);
  f.AddString("country_status_download_failed", [L(@"country_status_download_failed") UTF8String]);
  f.AddString("try_again", [L(@"try_again") UTF8String]);
  // Default texts for bookmarks added in C++ code (by URL Scheme API)
  f.AddString("dropped_pin", [L(@"dropped_pin") UTF8String]);
  f.AddString("my_places", [L(@"my_places") UTF8String]);
  f.AddString("my_position", [L(@"my_position") UTF8String]);
  f.AddString("routes", [L(@"routes") UTF8String]);

  f.AddString("routing_failed_unknown_my_position", [L(@"routing_failed_unknown_my_position") UTF8String]);
  f.AddString("routing_failed_has_no_routing_file", [L(@"routing_failed_has_no_routing_file") UTF8String]);
  f.AddString("routing_failed_start_point_not_found", [L(@"routing_failed_start_point_not_found") UTF8String]);
  f.AddString("routing_failed_dst_point_not_found", [L(@"routing_failed_dst_point_not_found") UTF8String]);
  f.AddString("routing_failed_cross_mwm_building", [L(@"routing_failed_cross_mwm_building") UTF8String]);
  f.AddString("routing_failed_route_not_found", [L(@"routing_failed_route_not_found") UTF8String]);
  f.AddString("routing_failed_internal_error", [L(@"routing_failed_internal_error") UTF8String]);
}

@implementation MapsAppDelegate
{
  NSString * m_geoURL;
  NSString * m_mwmURL;
  NSString * m_fileURL;

  NSString * m_scheme;
  NSString * m_sourceApplication;
  ActiveMapsObserver * m_mapsObserver;
}

+ (MapsAppDelegate *)theApp
{
  return (MapsAppDelegate *)[UIApplication sharedApplication].delegate;
}

+ (BOOL)isFirstAppLaunch
{
  // TODO: check if possible when user reinstall the app
  return [[NSUserDefaults standardUserDefaults] boolForKey:FIRST_LAUNCH_KEY];
}

- (void)initMRGService
{
  NSInteger appId = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"MRGServiceAppID"] integerValue];
  NSString * secret = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"MRGServiceClientKey"];
  
  // MRGService settings
  MRGServiceParams * mrgsParams = [[MRGServiceParams alloc] initWithAppId:appId andSecret:secret];
#ifdef DEBUG
  mrgsParams.debug = YES;
#endif
  mrgsParams.shouldResetBadge = NO;
  mrgsParams.crashReportEnabled = YES;
  mrgsParams.allowPushNotificationHooks = YES;
  
  NSString * appleAppId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"MRGServiceAppleAppID"];
  MRGSAppsFlyerParams * appsFlyerParams = [[MRGSAppsFlyerParams alloc] initWithDevKey:@"***REMOVED***" andAppleAppId:appleAppId];
#ifdef DEBUG
  appsFlyerParams.debug = YES;
#endif

  NSArray * externalParams = @[appsFlyerParams];
  
  [MRGServiceInit startWithServiceParams:mrgsParams externalSDKParams:externalParams delegate:nil];
  [[MRGSApplication currentApplication] markAsUpdatedWithRegistrationDate:[NSDate date]];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  // Initialize Alohalytics statistics engine.
#ifndef OMIM_PRODUCTION
  [Alohalytics setDebugMode:YES];
  NSString * serverUrl = @"http://localhost:8080/dev";
#else
  NSString * serverUrl = @"http://localhost:8080/2";
#endif
  [Alohalytics setup:serverUrl andFirstLaunch:[MapsAppDelegate isFirstAppLaunch] withLaunchOptions:launchOptions];

  [[Statistics instance] startSessionWithLaunchOptions:launchOptions];

  [AppInfo sharedInfo]; // we call it to init -firstLaunchDate
  if ([AppInfo sharedInfo].advertisingId)
    [[Statistics instance] logEvent:@"Device Info" withParameters:@{@"IFA" : [AppInfo sharedInfo].advertisingId, @"Country" : [AppInfo sharedInfo].countryCode}];

  InitLocalizedStrings();

  [self.m_mapViewController onEnterForeground];

  [Preferences setup:self.m_mapViewController];
  _m_locationManager = [[LocationManager alloc] init];

  m_navController = [[NavigationController alloc] initWithRootViewController:self.m_mapViewController];
  m_navController.navigationBarHidden = YES;
  m_window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  m_window.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  m_window.clearsContextBeforeDrawing = NO;
  m_window.multipleTouchEnabled = YES;
  [m_window setRootViewController:m_navController];
  [m_window makeKeyAndVisible];

  [self subscribeToStorage];

  [self customizeAppearance];

  [self initMRGService];

  if ([application respondsToSelector:@selector(setMinimumBackgroundFetchInterval:)])
    [application setMinimumBackgroundFetchInterval:(6 * 60 * 60)];

  if ([application respondsToSelector:@selector(registerUserNotificationSettings:)])
  {
    UIUserNotificationSettings * settings = [UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeBadge | UIUserNotificationTypeSound | UIUserNotificationTypeAlert) categories:nil];
    [[UIApplication sharedApplication] registerUserNotificationSettings:settings];
  }
  LocalNotificationManager * notificationManager = [LocalNotificationManager sharedManager];
  if (launchOptions[UIApplicationLaunchOptionsLocalNotificationKey])
    [notificationManager processNotification:launchOptions[UIApplicationLaunchOptionsLocalNotificationKey] onLaunch:YES];
  [notificationManager updateLocalNotifications];

  [UIApplication sharedApplication].applicationIconBadgeNumber = GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount();

  return [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey] != nil;
}

- (void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  [[LocalNotificationManager sharedManager] showDownloadMapNotificationIfNeeded:completionHandler];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  [[Statistics instance] applicationWillTerminate];

	[self.m_mapViewController onTerminate];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  [[Statistics instance] applicationDidEnterBackground];

	[self.m_mapViewController onEnterBackground];
  if (m_activeDownloadsCounter)
  {
    m_backgroundTask = [application beginBackgroundTaskWithExpirationHandler:^{
      [application endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }];
  }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
  [[Statistics instance] applicationWillResignActive];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  [[Statistics instance] applicationWillEnterForeground];

  [self.m_locationManager orientationChanged];
  [self.m_mapViewController onEnterForeground];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  [[Statistics instance] applicationDidBecomeActive];

  Framework & f = GetFramework();
  if (m_geoURL)
  {
    if (f.ShowMapForURL([m_geoURL UTF8String]))
    {
      if ([m_scheme isEqualToString:@"geo"])
        [[Statistics instance] logEvent:@"geo Import"];
      if ([m_scheme isEqualToString:@"ge0"])
        [[Statistics instance] logEvent:@"ge0(zero) Import"];

      [self showMap];
    }
  }
  else if (m_mwmURL)
  {
    if (f.ShowMapForURL([m_mwmURL UTF8String]));
    {
      [self.m_mapViewController setApiMode:YES animated:NO];
      [[Statistics instance] logApiUsage:m_sourceApplication];
      [self showMap];
    }
  }
  else if (m_fileURL)
  {
    if (!f.AddBookmarksFile([m_fileURL UTF8String]))
      [self showLoadFileAlertIsSuccessful:NO];

    [[NSNotificationCenter defaultCenter] postNotificationName:@"KML file added" object:nil];
    [self showLoadFileAlertIsSuccessful:YES];
    [[Statistics instance] logEvent:@"KML Import"];
  }
  else
  {
    UIPasteboard * pasteboard = [UIPasteboard generalPasteboard];
    if ([pasteboard.string length])
    {
      if (f.ShowMapForURL([pasteboard.string UTF8String]))
      {
        [self showMap];
        pasteboard.string = @"";
      }
    }
  }
  m_geoURL = nil;
  m_mwmURL = nil;
  m_fileURL = nil;

  [FBSettings setDefaultAppID:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FacebookAppID"]];
  [FBAppEvents activateApp];

  f.GetLocationState()->InvalidatePosition();
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  // Global cleanup
  DeleteFramework();
}

- (void)disableStandby
{
  ++m_standbyCounter;
  [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void)enableStandby
{
  --m_standbyCounter;
  if (m_standbyCounter <= 0)
  {
    [UIApplication sharedApplication].idleTimerDisabled = NO;
    m_standbyCounter = 0;
  }
}

- (void)disableDownloadIndicator
{
  --m_activeDownloadsCounter;
  if (m_activeDownloadsCounter <= 0)
  {
    [UIApplication sharedApplication].networkActivityIndicatorVisible = NO;
    m_activeDownloadsCounter = 0;
    if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground)
    {
      [[UIApplication sharedApplication] endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }
  }
}

- (void)enableDownloadIndicator
{
  ++m_activeDownloadsCounter;
  [UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}

- (void)customizeAppearance
{
  NSMutableDictionary * attributes = [[NSMutableDictionary alloc] init];
  attributes[UITextAttributeTextColor] = [UIColor whiteColor];
  attributes[UITextAttributeTextShadowColor] = [UIColor clearColor];
  [[UINavigationBar appearanceWhenContainedIn:[NavigationController class], nil] setTintColor:[UIColor colorWithColorCode:@"15c584"]];

  if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
  {
    [[UIBarButtonItem appearance] setTitleTextAttributes:attributes forState:UIControlStateNormal];

    [[UINavigationBar appearanceWhenContainedIn:[NavigationController class], nil] setBackgroundImage:[UIImage imageNamed:@"NavigationBarBackground7"] forBarMetrics:UIBarMetricsDefault];

    attributes[UITextAttributeFont] = [UIFont fontWithName:@"HelveticaNeue" size:17.5];
  }

  if ([UINavigationBar instancesRespondToSelector:@selector(setShadowImage:)])
    [[UINavigationBar appearanceWhenContainedIn:[NavigationController class], nil] setShadowImage:[[UIImage alloc] init]];

  [[UINavigationBar appearanceWhenContainedIn:[NavigationController class], nil] setTitleTextAttributes:attributes];
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification
{
  [[LocalNotificationManager sharedManager] processNotification:notification onLaunch:NO];
}

// We don't support HandleOpenUrl as it's deprecated from iOS 4.2
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
  NSString * scheme = url.scheme;

  m_scheme = scheme;
  m_sourceApplication = sourceApplication;

  // geo scheme support, see http://tools.ietf.org/html/rfc5870
  if ([scheme isEqualToString:@"geo"] || [scheme isEqualToString:@"ge0"])
  {
    m_geoURL = [url absoluteString];
    return YES;
  }
  else if ([scheme isEqualToString:@"mapswithme"] || [scheme isEqualToString:@"mwm"])
  {
    m_mwmURL = [url absoluteString];
    return YES;
  }
  else if ([scheme isEqualToString:@"file"])
  {
    m_fileURL = [url relativePath];
    return YES;
  }
  NSLog(@"Scheme %@ is not supported", scheme);

  return NO;
}

- (void)showLoadFileAlertIsSuccessful:(BOOL)successful
{
  m_loadingAlertView = [[UIAlertView alloc] initWithTitle:L(@"load_kmz_title")
                                                  message:
                        (successful ? L(@"load_kmz_successful") : L(@"load_kmz_failed"))
                                                 delegate:nil
                                        cancelButtonTitle:L(@"ok") otherButtonTitles:nil];
  m_loadingAlertView.delegate = self;
  [m_loadingAlertView show];
  [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(dismissAlert) userInfo:nil repeats:NO];
}

- (void)dismissAlert
{
  if (m_loadingAlertView)
    [m_loadingAlertView dismissWithClickedButtonIndex:0 animated:YES];
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  m_loadingAlertView = nil;
}

- (void)showMap
{
  [m_navController popToRootViewControllerAnimated:YES];
  [self.m_mapViewController dismissPopover];
}

- (void)subscribeToStorage
{
  __weak MapsAppDelegate * weakSelf = self;
  m_mapsObserver = new ActiveMapsObserver(weakSelf);
  GetFramework().GetCountryTree().GetActiveMapLayout().AddListener(m_mapsObserver);

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(outOfDateCountriesCountChanged:) name:MapsStatusChangedNotification object:nil];
}

- (void)countryStatusChangedAtPosition:(int)position inGroup:(storage::ActiveMapsLayout::TGroup const &)group
{
  ActiveMapsLayout & l = GetFramework().GetCountryTree().GetActiveMapLayout();
  int const outOfDateCount = l.GetOutOfDateCount();
  [[NSNotificationCenter defaultCenter] postNotificationName:MapsStatusChangedNotification object:nil userInfo:@{@"OutOfDate" : @(outOfDateCount)}];
}

- (void)outOfDateCountriesCountChanged:(NSNotification *)notification
{
  [UIApplication sharedApplication].applicationIconBadgeNumber = [[notification userInfo][@"OutOfDate"] integerValue];
}

@end

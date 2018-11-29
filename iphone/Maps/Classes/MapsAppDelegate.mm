#import "MapsAppDelegate.h"

#import "EAGLView.h"
#import "LocalNotificationManager.h"
#import "MWMAuthorizationCommon.h"
#import "MWMCommon.h"
#import "MWMCoreRouterType.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPushNotifications.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearch+CoreSpotlight.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#import <CoreSpotlight/CoreSpotlight.h>
#import <FBSDKCoreKit/FBSDKCoreKit.h>
#import <UserNotifications/UserNotifications.h>

#ifdef OMIM_PRODUCTION

#import <AppsFlyerTracker/AppsFlyerTracker.h>
#import <Crashlytics/Crashlytics.h>
#import <Fabric/Fabric.h>

#endif

#include "Framework.h"

#include "map/framework_light.hpp"
#include "map/gps_tracker.hpp"

#include "platform/http_thread_apple.h"
#include "platform/local_country_file_utils.hpp"

#include "private.h"
// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.

extern NSString * const MapsStatusChangedNotification = @"MapsStatusChangedNotification";
// Alert keys.
extern NSString * const kUDAlreadyRatedKey = @"UserAlreadyRatedApp";
extern NSString * const kUDAlreadySharedKey = @"UserAlreadyShared";

namespace
{
NSString * const kUDLastLaunchDateKey = @"LastLaunchDate";
NSString * const kUDSessionsCountKey = @"SessionsCount";
NSString * const kUDFirstVersionKey = @"FirstVersion";
NSString * const kUDLastRateRequestDate = @"LastRateRequestDate";
NSString * const kUDLastShareRequstDate = @"LastShareRequestDate";
NSString * const kUDAutoNightModeOff = @"AutoNightModeOff";
NSString * const kIOSIDFA = @"IFA";
NSString * const kBundleVersion = @"BundleVersion";

/// Adds needed localized strings to C++ code
/// @TODO Refactor localization mechanism to make it simpler
void InitLocalizedStrings()
{
  Framework & f = GetFramework();

  f.AddString("core_entrance", L(@"core_entrance").UTF8String);
  f.AddString("core_exit", L(@"core_exit").UTF8String);
  f.AddString("core_my_places", L(@"core_my_places").UTF8String);
  f.AddString("core_my_position", L(@"core_my_position").UTF8String);
  f.AddString("core_placepage_unknown_place", L(@"core_placepage_unknown_place").UTF8String);
  f.AddString("wifi", L(@"wifi").UTF8String);
}

void InitCrashTrackers()
{
#ifdef OMIM_PRODUCTION
  if ([MWMSettings crashReportingDisabled])
    return;

  NSString * fabricKey = @(CRASHLYTICS_IOS_KEY);
  if (fabricKey.length != 0)
  {
    // Initialize Fabric/Crashlytics SDK.
    [Fabric with:@[ [Crashlytics class] ]];
  }
#endif
}

void ConfigCrashTrackers()
{
#ifdef OMIM_PRODUCTION
  [[Crashlytics sharedInstance] setObjectValue:[Alohalytics installationId]
                                        forKey:@"AlohalyticsInstallationId"];
#endif
}

void OverrideUserAgent()
{
  [NSUserDefaults.standardUserDefaults registerDefaults:@{
    @"UserAgent" : @"Mozilla/5.0 (iPhone; CPU iPhone OS 10_3 like Mac OS X) AppleWebKit/603.1.30 "
                   @"(KHTML, like Gecko) Version/10.0 Mobile/14E269 Safari/602.1"
  }];
}
  
void InitMarketingTrackers()
{
#ifdef OMIM_PRODUCTION
  NSString * appsFlyerDevKey = @(APPSFLYER_KEY);
  NSString * appsFlyerAppIdKey = @(APPSFLYER_APP_ID_IOS);
  if (appsFlyerDevKey.length != 0 && appsFlyerAppIdKey.length != 0)
  {
    [AppsFlyerTracker sharedTracker].appsFlyerDevKey = appsFlyerDevKey;
    [AppsFlyerTracker sharedTracker].appleAppID = appsFlyerAppIdKey;
  }
#endif
}

void TrackMarketingAppLaunch()
{
#ifdef OMIM_PRODUCTION
  [[AppsFlyerTracker sharedTracker] trackAppLaunch];
#endif
}
}  // namespace

using namespace osm_auth_ios;

@interface MapsAppDelegate ()<MWMFrameworkStorageObserver, UNUserNotificationCenterDelegate>

@property(nonatomic) NSInteger standbyCounter;
@property(nonatomic) MWMBackgroundFetchScheduler * backgroundFetchScheduler;
@property(nonatomic) id<IPendingTransactionsHandler> pendingTransactionHandler;

@end

@implementation MapsAppDelegate
{
  NSString * m_geoURL;
  NSString * m_mwmURL;
  NSString * m_fileURL;

  NSString * m_scheme;
  NSString * m_sourceApplication;
}

+ (MapsAppDelegate *)theApp
{
  return (MapsAppDelegate *)UIApplication.sharedApplication.delegate;
}

#pragma mark - Notifications

// system push notification registration success callback, delegate to pushManager
- (void)application:(UIApplication *)application
    didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
  [MWMPushNotifications application:application
      didRegisterForRemoteNotificationsWithDeviceToken:deviceToken];
}

// system push notification registration error callback, delegate to pushManager
- (void)application:(UIApplication *)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
  [MWMPushNotifications application:application
      didFailToRegisterForRemoteNotificationsWithError:error];
}

// system push notifications callback, delegate to pushManager
- (void)application:(UIApplication *)application
    didReceiveRemoteNotification:(NSDictionary *)userInfo
          fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  [MWMPushNotifications application:application
       didReceiveRemoteNotification:userInfo
             fetchCompletionHandler:completionHandler];
}

- (BOOL)isDrapeEngineCreated
{
  return ((EAGLView *)self.mapViewController.view).drapeEngineCreated;
}

- (BOOL)hasApiURL { return m_geoURL || m_mwmURL; }
- (void)handleURLs
{
  self.mapViewController.launchByDeepLink = self.hasApiURL;

  if (!self.isDrapeEngineCreated)
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self handleURLs];
    });
    return;
  }

  Framework & f = GetFramework();
  if (m_geoURL)
  {
    if (f.ShowMapForURL(m_geoURL.UTF8String))
    {
      [Statistics logEvent:kStatEventName(kStatApplication, kStatImport)
            withParameters:@{kStatValue : m_scheme}];
      [self showMap];
    }
  }
  else if (m_mwmURL)
  {
    using namespace url_scheme;

    string const url = m_mwmURL.UTF8String;
    auto const parsingType = f.ParseAndSetApiURL(url);
    NSLog(@"Started by url: %@", m_mwmURL);
    switch (parsingType)
    {
    case ParsedMapApi::ParsingResult::Incorrect:
      LOG(LWARNING, ("Incorrect parsing result for url:", url));
      break;
    case ParsedMapApi::ParsingResult::Route:
    {
      auto const parsedData = f.GetParsedRoutingData();
      auto const points = parsedData.m_points;
      if (points.size() == 2)
      {
        auto p1 = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.front()
                                                                type:MWMRoutePointTypeStart
                                                   intermediateIndex:0];
        auto p2 = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.back()
                                                                type:MWMRoutePointTypeFinish
                                                   intermediateIndex:0];
        [MWMRouter buildApiRouteWithType:routerType(parsedData.m_type)
                              startPoint:p1
                             finishPoint:p2];
      }
      else
      {
#ifdef OMIM_PRODUCTION
        auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                              code:5
                                          userInfo:@{
                                            @"Description" : @"Invalid number of route points",
                                            @"URL" : m_mwmURL
                                          }];
        [[Crashlytics sharedInstance] recordError:err];
#endif
      }

      [self showMap];
      break;
    }
    case ParsedMapApi::ParsingResult::Map:
      if (f.ShowMapForURL(url))
        [self showMap];
      break;
    case ParsedMapApi::ParsingResult::Search:
    {
      auto const & request = f.GetParsedSearchRequest();
      auto manager = [MWMMapViewControlsManager manager];

      auto query = [@((request.m_query + " ").c_str()) stringByRemovingPercentEncoding];
      auto locale = @(request.m_locale.c_str());

      if (request.m_isSearchOnMap)
        [manager searchTextOnMap:query forInputLocale:locale];
      else
        [manager searchText:query forInputLocale:locale];

      break;
    }
    case ParsedMapApi::ParsingResult::Catalogue:
      [self.mapViewController openCatalogDeeplink:[[NSURL alloc] initWithString:m_mwmURL] animated:NO];
      break;
    case ParsedMapApi::ParsingResult::Lead: break;
    }
  }
  else if (m_fileURL)
  {
    f.AddBookmarksFile(m_fileURL.UTF8String, false /* isTemporaryFile */);
  }
  else
  {
    // Take a copy of pasteboard string since it can accidentally become nil while we still use it.
    NSString * pasteboard = [[UIPasteboard generalPasteboard].string copy];
    if (pasteboard && pasteboard.length)
    {
      if (f.ShowMapForURL(pasteboard.UTF8String))
      {
        [self showMap];
        [UIPasteboard generalPasteboard].string = @"";
      }
    }
  }
  m_geoURL = nil;
  m_mwmURL = nil;
  m_fileURL = nil;
}

- (NSURL *)convertUniversalLink:(NSURL *)universalLink
{
  auto deeplink = [NSString stringWithFormat:@"mapsme://%@?%@", universalLink.path, universalLink.query];
  return [NSURL URLWithString:deeplink];
}

- (void)searchText:(NSString *)searchString
{
  if (!self.isDrapeEngineCreated)
  {
    dispatch_async(dispatch_get_main_queue(), ^{ [self searchText:searchString]; });
    return;
  }

  [[MWMMapViewControlsManager manager] searchText:[searchString stringByAppendingString:@" "]
                                   forInputLocale:[MWMSettings spotlightLocaleLanguageId]];
}

- (void)incrementSessionsCountAndCheckForAlert
{
  [self incrementSessionCount];
  [self showAlertIfRequired];
}

- (void)commonInit
{
  [HttpThread setDownloadIndicatorProtocol:self];
  InitLocalizedStrings();
  GetFramework().SetupMeasurementSystem();
  [MWMFrameworkListener addObserver:self];
  [MapsAppDelegate customizeAppearance];

  self.standbyCounter = 0;
  NSTimeInterval const minimumBackgroundFetchIntervalInSeconds = 6 * 60 * 60;
  [UIApplication.sharedApplication
      setMinimumBackgroundFetchInterval:minimumBackgroundFetchIntervalInSeconds];
  [MWMMyTarget startAdServerForbiddenCheckTimer];
  [self updateApplicationIconBadgeNumber];
}

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  OverrideUserAgent();

  InitCrashTrackers();
  
  InitMarketingTrackers();

  // Initialize all 3party engines.
  [self initStatistics:application didFinishLaunchingWithOptions:launchOptions];

  // We send Alohalytics installation id to Fabric.
  // To make sure id is created, ConfigCrashTrackers must be called after Statistics initialization.
  ConfigCrashTrackers();

  [HttpThread setDownloadIndicatorProtocol:self];

  InitLocalizedStrings();
  [MWMThemeManager invalidate];

  [self commonInit];

  LocalNotificationManager * notificationManager = [LocalNotificationManager sharedManager];
  if (launchOptions[UIApplicationLaunchOptionsLocalNotificationKey])
  {
    NSNotification * notification = launchOptions[UIApplicationLaunchOptionsLocalNotificationKey];
    [notificationManager processNotification:notification.userInfo onLaunch:YES];
  }

  if ([Alohalytics isFirstSession])
  {
    [self firstLaunchSetup];
  }
  else
  {
    if ([MWMSettings statisticsEnabled])
      [Alohalytics enable];
    else
      [Alohalytics disable];
    [self incrementSessionsCountAndCheckForAlert];

    //For first launch setup is called by FirstLaunchController
    [MWMPushNotifications setup:launchOptions];
  }
  [self enableTTSForTheFirstTime];

  [MWMRouter restoreRouteIfNeeded];

  [GIDSignIn sharedInstance].clientID =
      [[NSBundle mainBundle] loadWithPlist:@"GoogleService-Info"][@"CLIENT_ID"];

  if (@available(iOS 10, *))
    [UNUserNotificationCenter currentNotificationCenter].delegate = self;

  if ([MWMFrameworkHelper canUseNetwork]) {
    [[SubscriptionManager shared] validate];
    self.pendingTransactionHandler = [InAppPurchase pendingTransactionsHandler];
    __weak __typeof(self) ws = self;
    [self.pendingTransactionHandler handlePendingTransactions:^(PendingTransactionsStatus) {
      ws.pendingTransactionHandler = nil;
    }];
  }

  if ([MoPubKit shouldShowConsentDialog])
    [MoPubKit grantConsent];
  
  return YES;
}

- (void)application:(UIApplication *)application
    performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
               completionHandler:(void (^)(BOOL))completionHandler
{
  [self.mapViewController performAction:shortcutItem.type];
  completionHandler(YES);
}

- (void)runBackgroundTasks:(NSArray<BackgroundFetchTask *> * _Nonnull)tasks
         completionHandler:(void (^_Nullable)(UIBackgroundFetchResult))completionHandler
{
  auto completion = ^(UIBackgroundFetchResult result) {
    if (completionHandler)
      completionHandler(result);
  };
  self.backgroundFetchScheduler =
      [[MWMBackgroundFetchScheduler alloc] initWithTasks:tasks completionHandler:completion];
  [self.backgroundFetchScheduler run];
}

- (void)application:(UIApplication *)application
    performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  auto onTap = ^{
    MapViewController * mapViewController = [MapViewController sharedController];
    [mapViewController.navigationController popToRootViewControllerAnimated:NO];
    [mapViewController showUGCAuth];
  };

  if ([LocalNotificationManager.sharedManager showUGCNotificationIfNeeded:onTap])
  {
    completionHandler(UIBackgroundFetchResultNewData);
    return;
  }

  lightweight::Framework const framework(lightweight::REQUEST_TYPE_NOTIFICATION);
  auto const notificationCandidate = framework.GetNotification();
  if (notificationCandidate)
  {
    auto const notification = notificationCandidate.get();
    if (notification.m_type == notifications::NotificationCandidate::Type::UgcReview &&
        notification.m_mapObject)
    {
      [LocalNotificationManager.sharedManager
       showReviewNotificationForPlace:@(notification.m_mapObject->GetReadableName().c_str())
       onTap:^{
         [Statistics logEvent:kStatUGCReviewNotificationClicked];
         place_page::Info info;
         if (GetFramework().MakePlacePageInfo(*notification.m_mapObject, info))
           [[MapViewController sharedController].controlsManager showPlacePageReview:info];
     }];
    }
  }

  auto tasks = @[
    [[MWMBackgroundStatisticsUpload alloc] init], [[MWMBackgroundEditsUpload alloc] init],
    [[MWMBackgroundUGCUpload alloc] init], [[MWMBackgroundDownloadMapNotification alloc] init]
  ];
  [self runBackgroundTasks:tasks completionHandler:completionHandler];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
#ifdef OMIM_PRODUCTION
  auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                        code:1
                                    userInfo:@{
                                      @"Description" : @"applicationDidReceiveMemoryWarning"
                                    }];
  [[Crashlytics sharedInstance] recordError:err];
#endif
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  [self.mapViewController onTerminate];

#ifdef OMIM_PRODUCTION
  auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                        code:2
                                    userInfo:@{
                                      @"Description" : @"applicationWillTerminate"
                                    }];
  [[Crashlytics sharedInstance] recordError:err];
#endif

  // Global cleanup
  DeleteFramework();
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  LOG(LINFO, ("applicationDidEnterBackground - begin"));
  if (m_activeDownloadsCounter)
  {
    m_backgroundTask = [application beginBackgroundTaskWithExpirationHandler:^{
      [application endBackgroundTask:self->m_backgroundTask];
      self->m_backgroundTask = UIBackgroundTaskInvalid;
    }];
  }

  auto tasks = @[[[MWMBackgroundEditsUpload alloc] init], [[MWMBackgroundUGCUpload alloc] init]];
  [self runBackgroundTasks:tasks completionHandler:nil];

  [MWMRouter saveRouteIfNeeded];
  LOG(LINFO, ("applicationDidEnterBackground - end"));
}

- (void)applicationWillResignActive:(UIApplication *)application
{
  LOG(LINFO, ("applicationWillResignActive - begin"));
  [self.mapViewController onGetFocus:NO];
  self.mapViewController.launchByDeepLink = NO;
  auto & f = GetFramework();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103)
  {
    f.SetRenderingDisabled(true);
    f.OnDestroyGLContext();
  }
  else
  {
    f.SetRenderingDisabled(false);
  }
  [MWMLocationManager applicationWillResignActive];
  f.EnterBackground();
  LOG(LINFO, ("applicationWillResignActive - end"));
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  LOG(LINFO, ("applicationWillEnterForeground - begin"));
  if (!GpsTracker::Instance().IsEnabled())
    return;

  MWMViewController * topVc = static_cast<MWMViewController *>(
      self.mapViewController.navigationController.topViewController);
  if (![topVc isKindOfClass:[MWMViewController class]])
    return;

  if ([MWMSettings isTrackWarningAlertShown])
    return;

  [topVc.alertController presentTrackWarningAlertWithCancelBlock:^{
    GpsTracker::Instance().SetEnabled(false);
  }];

  [MWMSettings setTrackWarningAlertShown:YES];
  LOG(LINFO, ("applicationWillEnterForeground - end"));
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  LOG(LINFO, ("applicationDidBecomeActive - begin"));
  
  TrackMarketingAppLaunch();
  
  auto & f = GetFramework();
  f.EnterForeground();
  [self.mapViewController onGetFocus:YES];
  [[Statistics instance] applicationDidBecomeActive];
  f.SetRenderingEnabled();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103)
  {
    m2::PointU const size = ((EAGLView *)self.mapViewController.view).pixelSize;
    f.OnRecoverGLContext(static_cast<int>(size.x), static_cast<int>(size.y));
  }
  [MWMLocationManager applicationDidBecomeActive];
  [MWMSearch addCategoriesToSpotlight];
  [MWMKeyboard applicationDidBecomeActive];
  [MWMTextToSpeech applicationDidBecomeActive];
  LOG(LINFO, ("applicationDidBecomeActive - end"));
}

- (BOOL)application:(UIApplication *)application
continueUserActivity:(NSUserActivity *)userActivity
 restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler {
  if ([userActivity.activityType isEqualToString:CSSearchableItemActionType])
  {
    NSString * searchStringKey = userActivity.userInfo[CSSearchableItemActivityIdentifier];
    NSString * searchString = L(searchStringKey);
    if (searchString)
    {
      [self searchText:searchString];
      return YES;
    }
  }
  else if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb])
  {
    auto link = userActivity.webpageURL;
    if ([self checkLaunchURL:[self convertUniversalLink:link]])
    {
      [self handleURLs];
      return YES;
    }
  }

  return NO;
}

- (BOOL)initStatistics:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  Statistics * statistics = [Statistics instance];
  BOOL returnValue =
      [statistics application:application didFinishLaunchingWithOptions:launchOptions];

  NSString * connectionType;
  NSString * network = [Statistics connectionTypeString];
  switch (Platform::ConnectionStatus())
  {
  case Platform::EConnectionType::CONNECTION_NONE: break;
  case Platform::EConnectionType::CONNECTION_WIFI:
    connectionType = @"Wi-Fi";
    break;
  case Platform::EConnectionType::CONNECTION_WWAN:
    connectionType = [[CTTelephonyNetworkInfo alloc] init].currentRadioAccessTechnology;
    break;
  }
  if (!connectionType)
    connectionType = @"Offline";
  [Statistics logEvent:kStatDeviceInfo
        withParameters:@{
          kStatCountry : [AppInfo sharedInfo].countryCode,
          kStatConnection : connectionType
        }];

  auto device = UIDevice.currentDevice;
  device.batteryMonitoringEnabled = YES;
  auto charging = kStatUnknown;
  auto const state = device.batteryState;
  if (state == UIDeviceBatteryStateCharging || state == UIDeviceBatteryStateFull)
    charging = kStatOn;
  else if (state == UIDeviceBatteryStateUnplugged)
    charging = kStatOff;

  [Statistics logEvent:kStatApplicationColdStartupInfo
        withParameters:@{
                         kStatBattery : @(UIDevice.currentDevice.batteryLevel * 100),
                         kStatCharging : charging,
                         kStatNetwork : network
                         }];

  return returnValue;
}

- (void)disableDownloadIndicator
{
  --m_activeDownloadsCounter;
  if (m_activeDownloadsCounter <= 0)
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      UIApplication.sharedApplication.networkActivityIndicatorVisible = NO;
    });
    m_activeDownloadsCounter = 0;
    if (UIApplication.sharedApplication.applicationState == UIApplicationStateBackground)
    {
      [UIApplication.sharedApplication endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }
  }
}

- (void)enableDownloadIndicator
{
  ++m_activeDownloadsCounter;
  dispatch_async(dispatch_get_main_queue(), ^{
    UIApplication.sharedApplication.networkActivityIndicatorVisible = YES;
  });
}

+ (NSDictionary *)navigationBarTextAttributes
{
  return @{
    NSForegroundColorAttributeName : [UIColor whitePrimaryText],
    NSFontAttributeName : [UIFont regular18]
  };
}

+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar
{
  navigationBar.tintColor = [UIColor primary];
  navigationBar.barTintColor = [UIColor primary];
  navigationBar.titleTextAttributes = [self navigationBarTextAttributes];
  navigationBar.translucent = NO;
  [navigationBar setBackgroundImage:[UIImage new] forBarMetrics:UIBarMetricsDefault];
  navigationBar.shadowImage = [UIImage new];
  auto backImage = [[UIImage imageNamed:@"ic_nav_bar_back_sys"]
                    imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
  navigationBar.backIndicatorImage = backImage;
  navigationBar.backIndicatorTransitionMaskImage = backImage;
}

+ (void)customizeAppearance
{
  [UIButton appearance].exclusiveTouch = YES;

  [self customizeAppearanceForNavigationBar:[UINavigationBar appearance]];

  UIBarButtonItem * barBtn = [UIBarButtonItem appearance];
  [barBtn setTitleTextAttributes:[self navigationBarTextAttributes] forState:UIControlStateNormal];
  [barBtn setTitleTextAttributes:@{
    NSForegroundColorAttributeName : [UIColor lightGrayColor],
  }
                        forState:UIControlStateDisabled];
  [UIBarButtonItem appearanceWhenContainedInInstancesOfClasses:@[[UINavigationBar class]]].tintColor = [UIColor whitePrimaryText];

  UIPageControl * pageControl = [UIPageControl appearance];
  pageControl.pageIndicatorTintColor = [UIColor blackHintText];
  pageControl.currentPageIndicatorTintColor = [UIColor blackSecondaryText];
  pageControl.backgroundColor = [UIColor white];

  UITextField * textField = [UITextField appearance];
  textField.keyboardAppearance =
      [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;

  UISearchBar * searchBar = [UISearchBar appearance];
  searchBar.barTintColor = [UIColor primary];
  UITextField * textFieldInSearchBar =
      [UITextField appearanceWhenContainedInInstancesOfClasses:@[[UISearchBar class]]];

  textField.backgroundColor = [UIColor white];
  textFieldInSearchBar.defaultTextAttributes = @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont regular14]
  };
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
  if ([LocalNotificationManager isLocalNotification:notification])
    completionHandler(UNNotificationPresentationOptionAlert | UNNotificationPresentationOptionSound);
  else
    [MWMPushNotifications userNotificationCenter:center
                         willPresentNotification:notification
                           withCompletionHandler:completionHandler];
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void(^)(void))completionHandler
{
  if ([LocalNotificationManager isLocalNotification:response.notification])
  {
    if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier])
    {
      auto userInfo = response.notification.request.content.userInfo;
      [[LocalNotificationManager sharedManager] processNotification:userInfo onLaunch:NO];
    }
    completionHandler();
  }
  else
  {
    [MWMPushNotifications userNotificationCenter:center
                  didReceiveNotificationResponse:response
                           withCompletionHandler:completionHandler];
  }
}

- (void)application:(UIApplication *)application
    didReceiveLocalNotification:(UILocalNotification *)notification
{
  [[LocalNotificationManager sharedManager] processNotification:notification.userInfo onLaunch:NO];
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options
{
  m_sourceApplication = options[UIApplicationOpenURLOptionsSourceApplicationKey];

  BOOL isGoogleURL = [[GIDSignIn sharedInstance] handleURL:url
                                         sourceApplication:m_sourceApplication
                                                annotation:options[UIApplicationOpenURLOptionsAnnotationKey]];
  if (isGoogleURL)
    return YES;

  BOOL isFBURL = [[FBSDKApplicationDelegate sharedInstance] application:app openURL:url options:options];
  if (isFBURL)
    return YES;

  for (auto host in @[@"dlink.maps.me", @"dlink.mapsme.devmail.ru"])
  {
    if ([self checkLaunchURL:(url.host.length > 0 && [url.host rangeOfString:host].location != NSNotFound)
         ? [self convertUniversalLink:url] : url])
    {
      [self handleURLs];
      return YES;
    }
  }

  return NO;
}

- (BOOL)checkLaunchURL:(NSURL *)url
{
  NSString * scheme = url.scheme;
  m_scheme = scheme;
  if ([scheme isEqualToString:@"geo"] || [scheme isEqualToString:@"ge0"])
  {
    m_geoURL = [url absoluteString];
    return YES;
  }
  else if ([scheme isEqualToString:@"mapswithme"] || [scheme isEqualToString:@"mwm"] ||
           [scheme isEqualToString:@"mapsme"])
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

- (void)showMap
{
  [(UINavigationController *)self.window.rootViewController popToRootViewControllerAnimated:YES];
}

- (void)updateApplicationIconBadgeNumber
{
  auto const number = [self badgeNumber];
  UIApplication.sharedApplication.applicationIconBadgeNumber = number;
  [[MWMBottomMenuViewController controller] updateBadgeVisible:number != 0];
}

- (NSUInteger)badgeNumber
{
  auto & s = GetFramework().GetStorage();
  storage::Storage::UpdateInfo updateInfo{};
  s.GetUpdateInfo(s.GetRootId(), updateInfo);
  return updateInfo.m_numberOfMwmFilesToUpdate + (platform::migrate::NeedMigrate() ? 1 : 0);
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(storage::TCountryId const &)countryId
{
  // Dispatch this method after delay since there are too many events for group mwms download.
  // We do not need to update badge frequently.
  // Update after 1 second delay (after last country event) is sure enough for app badge.
  SEL const updateBadge = @selector(updateApplicationIconBadgeNumber);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:updateBadge object:nil];
  [self performSelector:updateBadge withObject:nil afterDelay:1.0];
}

#pragma mark - Properties

- (MapViewController *)mapViewController
{
  return [(UINavigationController *)self.window.rootViewController viewControllers].firstObject;
}

#pragma mark - TTS

- (void)enableTTSForTheFirstTime
{
  if (![MWMTextToSpeech savedLanguage].length)
    [MWMTextToSpeech setTTSEnabled:YES];
}

#pragma mark - Standby

- (void)enableStandby { self.standbyCounter--; }
- (void)disableStandby { self.standbyCounter++; }
- (void)setStandbyCounter:(NSInteger)standbyCounter
{
  _standbyCounter = MAX(0, standbyCounter);
  dispatch_async(dispatch_get_main_queue(), ^{
    [UIApplication sharedApplication].idleTimerDisabled = (self.standbyCounter != 0);
  });
}

#pragma mark - Alert logic

- (void)firstLaunchSetup
{
  [MWMSettings setStatisticsEnabled:YES];
  NSString * currentVersion =
      [NSBundle.mainBundle objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSUserDefaults * standartDefaults = NSUserDefaults.standardUserDefaults;
  [standartDefaults setObject:currentVersion forKey:kUDFirstVersionKey];
  [standartDefaults setInteger:1 forKey:kUDSessionsCountKey];
  [standartDefaults setObject:NSDate.date forKey:kUDLastLaunchDateKey];
  [standartDefaults synchronize];
  
  GetPlatform().GetMarketingService().ProcessFirstLaunch();
}

- (void)incrementSessionCount
{
  NSUserDefaults * standartDefaults = NSUserDefaults.standardUserDefaults;
  NSUInteger sessionCount = [standartDefaults integerForKey:kUDSessionsCountKey];
  NSUInteger const kMaximumSessionCountForShowingShareAlert = 50;
  if (sessionCount > kMaximumSessionCountForShowingShareAlert)
    return;

  NSDate * lastLaunchDate = [standartDefaults objectForKey:kUDLastLaunchDateKey];
  NSUInteger daysFromLastLaunch = [self.class daysBetweenNowAndDate:lastLaunchDate];
  if (daysFromLastLaunch > 0)
  {
    sessionCount++;
    [standartDefaults setInteger:sessionCount forKey:kUDSessionsCountKey];
    [standartDefaults setObject:NSDate.date forKey:kUDLastLaunchDateKey];
    [standartDefaults synchronize];
  }
}

- (void)showAlertIfRequired
{
  if ([self shouldShowRateAlert])
  {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showRateAlert) object:nil];
    [self performSelector:@selector(showRateAlert) withObject:nil afterDelay:30.0];
  }
  else if ([self shouldShowFacebookAlert])
  {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showFacebookAlert) object:nil];
    [self performSelector:@selector(showFacebookAlert) withObject:nil afterDelay:30.0];
  }
}

- (void)showAlert:(BOOL)isRate
{
  if (!Platform::IsConnected() || [MWMRouter isRoutingActive])
    return;

  if (isRate)
    [[MWMAlertViewController activeAlertController] presentRateAlert];
  else
    [[MWMAlertViewController activeAlertController] presentFacebookAlert];
  [NSUserDefaults.standardUserDefaults
      setObject:NSDate.date
         forKey:isRate ? kUDLastRateRequestDate : kUDLastShareRequstDate];
}

#pragma mark - Facebook

- (void)showFacebookAlert { [self showAlert:NO]; }
- (BOOL)shouldShowFacebookAlert
{
  NSUInteger const kMaximumSessionCountForShowingShareAlert = 50;
  NSUserDefaults const * const standartDefaults = NSUserDefaults.standardUserDefaults;
  if ([standartDefaults boolForKey:kUDAlreadySharedKey])
    return NO;

  NSUInteger const sessionCount = [standartDefaults integerForKey:kUDSessionsCountKey];
  if (sessionCount > kMaximumSessionCountForShowingShareAlert)
    return NO;

  NSDate * const lastShareRequestDate = [standartDefaults objectForKey:kUDLastShareRequstDate];
  NSUInteger const daysFromLastShareRequest =
      [MapsAppDelegate daysBetweenNowAndDate:lastShareRequestDate];
  if (lastShareRequestDate != nil && daysFromLastShareRequest == 0)
    return NO;

  if (sessionCount == 30 || sessionCount == kMaximumSessionCountForShowingShareAlert)
    return YES;

  if (self.userIsNew)
  {
    if (sessionCount == 12)
      return YES;
  }
  else
  {
    if (sessionCount == 5)
      return YES;
  }
  return NO;
}

#pragma mark - Rate

- (void)showRateAlert { [self showAlert:YES]; }
- (BOOL)shouldShowRateAlert
{
  NSUInteger const kMaximumSessionCountForShowingAlert = 21;
  NSUserDefaults const * const standartDefaults = NSUserDefaults.standardUserDefaults;
  if ([standartDefaults boolForKey:kUDAlreadyRatedKey])
    return NO;

  NSUInteger const sessionCount = [standartDefaults integerForKey:kUDSessionsCountKey];
  if (sessionCount > kMaximumSessionCountForShowingAlert)
    return NO;

  NSDate * const lastRateRequestDate = [standartDefaults objectForKey:kUDLastRateRequestDate];
  NSUInteger const daysFromLastRateRequest =
      [MapsAppDelegate daysBetweenNowAndDate:lastRateRequestDate];
  // Do not show more than one alert per day.
  if (lastRateRequestDate != nil && daysFromLastRateRequest == 0)
    return NO;

  if (self.userIsNew)
  {
    // It's new user.
    if (sessionCount == 3 || sessionCount == 10 ||
        sessionCount == kMaximumSessionCountForShowingAlert)
      return YES;
  }
  else
  {
    // User just got updated. Show alert, if it first session or if 90 days spent.
    if (daysFromLastRateRequest >= 90 || daysFromLastRateRequest == 0)
      return YES;
  }
  return NO;
}

- (BOOL)userIsNew
{
  NSString * currentVersion =
      [NSBundle.mainBundle objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSString * firstVersion = [NSUserDefaults.standardUserDefaults stringForKey:kUDFirstVersionKey];
  if (!firstVersion.length || firstVersionIsLessThanSecond(firstVersion, currentVersion))
    return NO;

  return YES;
}

+ (NSInteger)daysBetweenNowAndDate:(NSDate *)fromDate
{
  if (!fromDate)
    return 0;

  NSDate * now = NSDate.date;
  NSCalendar * calendar = NSCalendar.currentCalendar;
  [calendar rangeOfUnit:NSCalendarUnitDay startDate:&fromDate interval:NULL forDate:fromDate];
  [calendar rangeOfUnit:NSCalendarUnitDay startDate:&now interval:NULL forDate:now];
  NSDateComponents * difference =
      [calendar components:NSCalendarUnitDay fromDate:fromDate toDate:now options:0];
  return difference.day;
}

#pragma mark - Showcase

- (MWMMyTarget *)myTarget
{
  if (![ASIdentifierManager sharedManager].advertisingTrackingEnabled)
    return nil;
  
  if (!_myTarget)
    _myTarget = [[MWMMyTarget alloc] init];
  return _myTarget;
}

@end

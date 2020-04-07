#import "MapsAppDelegate.h"

#import "CoreNotificationWrapper+Core.h"
#import "EAGLView.h"
#import "LocalNotificationManager.h"
#import "MWMAuthorizationCommon.h"
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
#import "NSDate+TimeDistance.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#import <CarPlay/CarPlay.h>
#import <CoreSpotlight/CoreSpotlight.h>
#import <FBSDKCoreKit/FBSDKCoreKit.h>
#import <UserNotifications/UserNotifications.h>

#import <AppsFlyerLib/AppsFlyerTracker.h>
#import <Firebase/Firebase.h>

#include <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>

#include "map/framework_light.hpp"
#include "map/gps_tracker.hpp"

#include "partners_api/ads/mopub_ads.hpp"

#include "platform/background_downloader_ios.h"
#include "platform/http_thread_apple.h"
#include "platform/local_country_file_utils.hpp"

#include "base/assert.hpp"

#include "private.h"
// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.

// Alert keys.
extern NSString *const kUDAlreadyRatedKey = @"UserAlreadyRatedApp";

namespace {
NSString *const kUDLastLaunchDateKey = @"LastLaunchDate";
NSString *const kUDSessionsCountKey = @"SessionsCount";
NSString *const kUDFirstVersionKey = @"FirstVersion";
NSString *const kUDLastRateRequestDate = @"LastRateRequestDate";
NSString *const kUDLastShareRequstDate = @"LastShareRequestDate";
NSString *const kUDAutoNightModeOff = @"AutoNightModeOff";
NSString *const kIOSIDFA = @"IFA";
NSString *const kBundleVersion = @"BundleVersion";

/// Adds needed localized strings to C++ code
/// @TODO Refactor localization mechanism to make it simpler
void InitLocalizedStrings() {
  Framework &f = GetFramework();

  f.AddString("core_entrance", L(@"core_entrance").UTF8String);
  f.AddString("core_exit", L(@"core_exit").UTF8String);
  f.AddString("core_my_places", L(@"core_my_places").UTF8String);
  f.AddString("core_my_position", L(@"core_my_position").UTF8String);
  f.AddString("core_placepage_unknown_place", L(@"core_placepage_unknown_place").UTF8String);
  f.AddString("postal_code", L(@"postal_code").UTF8String);
  f.AddString("wifi", L(@"wifi").UTF8String);
}

void InitCrashTrackers() {
#ifdef OMIM_PRODUCTION
  if ([MWMSettings crashReportingDisabled])
    return;

  NSString *googleConfig = [[NSBundle mainBundle] pathForResource:@"GoogleService-Info" ofType:@"plist"];
  if ([[NSFileManager defaultManager] fileExistsAtPath:googleConfig]) {
    [FIRApp configure];
  }
#endif
}

void ConfigCrashTrackers() {
#ifdef OMIM_PRODUCTION
  [[FIRCrashlytics crashlytics] setUserID:[Alohalytics installationId]];
#endif
}

void OverrideUserAgent() {
  [NSUserDefaults.standardUserDefaults registerDefaults:@{
    @"UserAgent": @"Mozilla/5.0 (iPhone; CPU iPhone OS 10_3 like Mac OS X) AppleWebKit/603.1.30 "
                  @"(KHTML, like Gecko) Version/10.0 Mobile/14E269 Safari/602.1"
  }];
}
}  // namespace

using namespace osm_auth_ios;

@interface MapsAppDelegate () <MWMStorageObserver,
                               NotificationManagerDelegate,
                               AppsFlyerTrackerDelegate,
                               CPApplicationDelegate>

@property(nonatomic) NSInteger standbyCounter;
@property(nonatomic) MWMBackgroundFetchScheduler *backgroundFetchScheduler;
@property(nonatomic) id<IPendingTransactionsHandler> pendingTransactionHandler;
@property(nonatomic) NotificationManager *notificationManager;

@end

@implementation MapsAppDelegate

+ (MapsAppDelegate *)theApp {
  return (MapsAppDelegate *)UIApplication.sharedApplication.delegate;
}

#pragma mark - Notifications

// system push notification registration success callback, delegate to pushManager
- (void)application:(UIApplication *)application
  didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
  [MWMPushNotifications application:application didRegisterForRemoteNotificationsWithDeviceToken:deviceToken];
}

// system push notification registration error callback, delegate to pushManager
- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
  [MWMPushNotifications application:application didFailToRegisterForRemoteNotificationsWithError:error];
}

// system push notifications callback, delegate to pushManager
- (void)application:(UIApplication *)application
  didReceiveRemoteNotification:(NSDictionary *)userInfo
        fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
  [MWMPushNotifications application:application
       didReceiveRemoteNotification:userInfo
             fetchCompletionHandler:completionHandler];
}

- (BOOL)isDrapeEngineCreated {
  return self.mapViewController.mapView.drapeEngineCreated;
}

- (BOOL)isGraphicContextInitialized {
  return self.mapViewController.mapView.graphicContextInitialized;
}

- (void)searchText:(NSString *)searchString {
  if (!self.isDrapeEngineCreated) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self searchText:searchString];
    });
    return;
  }

  [[MWMMapViewControlsManager manager] searchText:[searchString stringByAppendingString:@" "]
                                   forInputLocale:[MWMSettings spotlightLocaleLanguageId]];
}

- (void)incrementSessionsCountAndCheckForAlert {
  [self incrementSessionCount];
  [self showAlertIfRequired];
}

- (void)commonInit {
  [HttpThreadImpl setDownloadIndicatorProtocol:self];
  InitLocalizedStrings();
  GetFramework().SetupMeasurementSystem();
  [[MWMStorage sharedStorage] addObserver:self];
  [MapsAppDelegate customizeAppearance];

  self.standbyCounter = 0;
  NSTimeInterval const minimumBackgroundFetchIntervalInSeconds = 6 * 60 * 60;
  [UIApplication.sharedApplication setMinimumBackgroundFetchInterval:minimumBackgroundFetchIntervalInSeconds];
  [MWMMyTarget startAdServerForbiddenCheckTimer];
  [self updateApplicationIconBadgeNumber];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  NSLog(@"deeplinking: launchOptions %@", launchOptions);
  OverrideUserAgent();

  InitCrashTrackers();

  [self initMarketingTrackers];

  // Initialize all 3party engines.
  [self initStatistics:application didFinishLaunchingWithOptions:launchOptions];

  // We send Alohalytics installation id to Fabric.
  // To make sure id is created, ConfigCrashTrackers must be called after Statistics initialization.
  ConfigCrashTrackers();

  [HttpThreadImpl setDownloadIndicatorProtocol:self];

  InitLocalizedStrings();
  [MWMThemeManager invalidate];

  [self commonInit];

  if ([Alohalytics isFirstSession]) {
    [self firstLaunchSetup];
  } else {
    if ([MWMSettings statisticsEnabled])
      [Alohalytics enable];
    else
      [Alohalytics disable];
    [self incrementSessionsCountAndCheckForAlert];

    // For first launch setup is called by FirstLaunchController
    [MWMPushNotifications setup];
  }
  [self enableTTSForTheFirstTime];

  [GIDSignIn sharedInstance].clientID = @(GOOGLE_WEB_CLIENT_ID);

  self.notificationManager = [[NotificationManager alloc] init];
  self.notificationManager.delegate = self;
  [UNUserNotificationCenter currentNotificationCenter].delegate = self.notificationManager;

  if ([MWMFrameworkHelper isWiFiConnected]) {
    [[InAppPurchase bookmarksSubscriptionManager] validateWithCompletion:^(MWMValidationResult result, BOOL isTrial) {
      if (result == MWMValidationResultNotValid) {
        [[InAppPurchase bookmarksSubscriptionManager] setSubscriptionActive:NO isTrial:NO];
      }
    }];
    [[InAppPurchase allPassSubscriptionManager] validateWithCompletion:^(MWMValidationResult result, BOOL isTrial) {
      if (result == MWMValidationResultNotValid) {
        [[InAppPurchase allPassSubscriptionManager] setSubscriptionActive:NO isTrial:NO];
      }
    }];
    [[InAppPurchase adsRemovalSubscriptionManager] validateWithCompletion:^(MWMValidationResult result, BOOL isTrial) {
      [[InAppPurchase adsRemovalSubscriptionManager] setSubscriptionActive:result != MWMValidationResultNotValid
                                                                   isTrial:NO];
    }];
    self.pendingTransactionHandler = [InAppPurchase pendingTransactionsHandler];
    __weak __typeof(self) ws = self;
    [self.pendingTransactionHandler handlePendingTransactions:^(PendingTransactionsStatus) {
      ws.pendingTransactionHandler = nil;
    }];
  }

  MPMoPubConfiguration *sdkConfig =
    [[MPMoPubConfiguration alloc] initWithAdUnitIdForAppInitialization:@(ads::Mopub::InitializationBannerId().c_str())];
  NSDictionary *facebookConfig = @{@"native_banner": @true};
  NSMutableDictionary *config = [@{@"FacebookAdapterConfiguration": facebookConfig} mutableCopy];
  sdkConfig.mediatedNetworkConfigurations = config;
  sdkConfig.loggingLevel = MPBLogLevelDebug;
  [[MoPub sharedInstance] initializeSdkWithConfiguration:sdkConfig completion:nil];

  if ([MoPubKit shouldShowConsentDialog])
    [MoPubKit grantConsent];

  [[DeepLinkHandler shared] applicationDidFinishLaunching:launchOptions];
  if (@available(iOS 13, *)) {
    [MWMUser verifyAppleId];
  }
  return YES;
}

- (void)application:(UIApplication *)application
  performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
             completionHandler:(void (^)(BOOL))completionHandler {
  [self.mapViewController performAction:shortcutItem.type];
  completionHandler(YES);
}

- (void)runBackgroundTasks:(NSArray<BackgroundFetchTask *> *_Nonnull)tasks
         completionHandler:(void (^_Nullable)(UIBackgroundFetchResult))completionHandler {
  self.backgroundFetchScheduler = [[MWMBackgroundFetchScheduler alloc] initWithTasks:tasks
                                                                   completionHandler:^(UIBackgroundFetchResult result) {
                                                                     if (completionHandler)
                                                                       completionHandler(result);
                                                                   }];
  [self.backgroundFetchScheduler run];
}

- (void)application:(UIApplication *)application
  performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
  if ([LocalNotificationManager shouldShowAuthNotification]) {
    AuthNotification *n = [[AuthNotification alloc] initWithTitle:L(@"notification_unsent_reviews_title")
                                                             text:L(@"notification_unsent_reviews_message")];
    [self.notificationManager showNotification:n];
    [LocalNotificationManager authNotificationWasShown];
    completionHandler(UIBackgroundFetchResultNewData);
    return;
  }

  CoreNotificationWrapper *reviewNotification = [LocalNotificationManager reviewNotificationWrapper];
  if (reviewNotification) {
    NSString *text =
      reviewNotification.address.length > 0
        ? [NSString stringWithFormat:@"%@, %@", reviewNotification.readableName, reviewNotification.address]
        : reviewNotification.readableName;
    ReviewNotification *n = [[ReviewNotification alloc] initWithTitle:L(@"notification_leave_review_v2_title")
                                                                 text:text
                                                  notificationWrapper:reviewNotification];
    [self.notificationManager showNotification:n];
    [LocalNotificationManager reviewNotificationWasShown];
  }

  [self runBackgroundTasks:@ [[MWMBackgroundStatisticsUpload new], [MWMBackgroundEditsUpload new],
                              [MWMBackgroundUGCUpload new]]
    completionHandler:completionHandler];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
#ifdef OMIM_PRODUCTION
  auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                        code:1
                                    userInfo:@{@"Description": @"applicationDidReceiveMemoryWarning"}];
  [[FIRCrashlytics crashlytics] recordError:err];
#endif
}

- (void)applicationWillTerminate:(UIApplication *)application {
  [self.mapViewController onTerminate];

#ifdef OMIM_PRODUCTION
  auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                        code:2
                                    userInfo:@{@"Description": @"applicationWillTerminate"}];
  [[FIRCrashlytics crashlytics] recordError:err];
#endif

  // Global cleanup
  DeleteFramework();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
  LOG(LINFO, ("applicationDidEnterBackground - begin"));
  [DeepLinkHandler.shared reset];
  if (m_activeDownloadsCounter) {
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

- (void)applicationWillResignActive:(UIApplication *)application {
  LOG(LINFO, ("applicationWillResignActive - begin"));
  [self.mapViewController onGetFocus:NO];
  auto &f = GetFramework();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103) {
    f.SetRenderingDisabled(true);
    f.OnDestroySurface();
  } else {
    f.SetRenderingDisabled(false);
  }
  [MWMLocationManager applicationWillResignActive];
  f.EnterBackground();
  LOG(LINFO, ("applicationWillResignActive - end"));
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
  LOG(LINFO, ("applicationWillEnterForeground - begin"));
  if (!GpsTracker::Instance().IsEnabled())
    return;

  MWMViewController *topVc =
    static_cast<MWMViewController *>(self.mapViewController.navigationController.topViewController);
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

- (void)applicationDidBecomeActive:(UIApplication *)application {
  LOG(LINFO, ("applicationDidBecomeActive - begin"));

  [self trackMarketingAppLaunch];

  auto &f = GetFramework();
  f.EnterForeground();
  [self.mapViewController onGetFocus:YES];
  [[Statistics instance] applicationDidBecomeActive];
  f.SetRenderingEnabled();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103) {
    CGSize const objcSize = self.mapViewController.mapView.pixelSize;
    f.OnRecoverSurface(static_cast<int>(objcSize.width), static_cast<int>(objcSize.height),
                       true /* recreateContextDependentResources */);
  }
  [MWMLocationManager applicationDidBecomeActive];
  [MWMSearch addCategoriesToSpotlight];
  [MWMKeyboard applicationDidBecomeActive];
  [MWMTextToSpeech applicationDidBecomeActive];
  LOG(LINFO, ("applicationDidBecomeActive - end"));
}

- (BOOL)application:(UIApplication *)application
  continueUserActivity:(NSUserActivity *)userActivity
    restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> *_Nullable))restorationHandler {
  if ([userActivity.activityType isEqualToString:CSSearchableItemActionType]) {
    NSString *searchStringKey = userActivity.userInfo[CSSearchableItemActivityIdentifier];
    NSString *searchString = L(searchStringKey);
    if (searchString) {
      [self searchText:searchString];
      return YES;
    }
  } else if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb] &&
             userActivity.webpageURL != nil) {
    return [DeepLinkHandler.shared applicationDidReceiveUniversalLink:userActivity.webpageURL];
  }

  return NO;
}

- (void)initMarketingTrackers {
  NSString *appsFlyerDevKey = @(APPSFLYER_KEY);
  NSString *appsFlyerAppIdKey = @(APPSFLYER_APP_ID_IOS);
  if (appsFlyerDevKey.length != 0 && appsFlyerAppIdKey.length != 0) {
    [AppsFlyerTracker sharedTracker].appsFlyerDevKey = appsFlyerDevKey;
    [AppsFlyerTracker sharedTracker].appleAppID = appsFlyerAppIdKey;
    [AppsFlyerTracker sharedTracker].delegate = self;
#if DEBUG
    [AppsFlyerTracker sharedTracker].isDebug = YES;
#endif
  }
}

- (void)trackMarketingAppLaunch {
  [[AppsFlyerTracker sharedTracker] trackAppLaunch];
}

- (BOOL)initStatistics:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  Statistics *statistics = [Statistics instance];
  BOOL returnValue = [statistics application:application didFinishLaunchingWithOptions:launchOptions];

  NSString *connectionType;
  NSString *network = [Statistics connectionTypeString];
  switch (Platform::ConnectionStatus()) {
    case Platform::EConnectionType::CONNECTION_NONE:
      break;
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
        withParameters:@{kStatCountry: [AppInfo sharedInfo].countryCode, kStatConnection: connectionType}];

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
          kStatBattery: @(UIDevice.currentDevice.batteryLevel * 100),
          kStatCharging: charging,
          kStatNetwork: network
        }];

  return returnValue;
}

- (void)disableDownloadIndicator {
  --m_activeDownloadsCounter;
  if (m_activeDownloadsCounter <= 0) {
    dispatch_async(dispatch_get_main_queue(), ^{
      UIApplication.sharedApplication.networkActivityIndicatorVisible = NO;
    });
    m_activeDownloadsCounter = 0;
    if (UIApplication.sharedApplication.applicationState == UIApplicationStateBackground) {
      [UIApplication.sharedApplication endBackgroundTask:m_backgroundTask];
      m_backgroundTask = UIBackgroundTaskInvalid;
    }
  }
}

- (void)enableDownloadIndicator {
  ++m_activeDownloadsCounter;
  dispatch_async(dispatch_get_main_queue(), ^{
    UIApplication.sharedApplication.networkActivityIndicatorVisible = YES;
  });
}

+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar {
  auto backImage =
    [[UIImage imageNamed:@"ic_nav_bar_back_sys"] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
  navigationBar.backIndicatorImage = backImage;
  navigationBar.backIndicatorTransitionMaskImage = backImage;
}

+ (void)customizeAppearance {
  [UIButton appearance].exclusiveTouch = YES;

  [self customizeAppearanceForNavigationBar:[UINavigationBar appearance]];

  UITextField *textField = [UITextField appearance];
  textField.keyboardAppearance = [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
}

- (BOOL)application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
  BOOL isGoogleURL = [[GIDSignIn sharedInstance] handleURL:url
                                         sourceApplication:options[UIApplicationOpenURLOptionsSourceApplicationKey]
                                                annotation:options[UIApplicationOpenURLOptionsAnnotationKey]];
  if (isGoogleURL)
    return YES;

  BOOL isFBURL = [[FBSDKApplicationDelegate sharedInstance] application:app openURL:url options:options];
  if (isFBURL)
    return YES;

  return [DeepLinkHandler.shared applicationDidOpenUrl:url];
}

- (void)showMap {
  [(UINavigationController *)self.window.rootViewController popToRootViewControllerAnimated:YES];
}

- (void)updateApplicationIconBadgeNumber {
  auto const number = [self badgeNumber];
  UIApplication.sharedApplication.applicationIconBadgeNumber = number;
  BottomTabBarViewController.controller.isApplicationBadgeHidden = number == 0;
}

- (NSUInteger)badgeNumber {
  auto &s = GetFramework().GetStorage();
  storage::Storage::UpdateInfo updateInfo{};
  s.GetUpdateInfo(s.GetRootId(), updateInfo);
  return updateInfo.m_numberOfMwmFilesToUpdate;
}

- (void)application:(UIApplication *)application
  handleEventsForBackgroundURLSession:(NSString *)identifier
                    completionHandler:(void (^)())completionHandler {
  [BackgroundDownloader sharedBackgroundMapDownloader].backgroundCompletionHandler = completionHandler;
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId {
  // Dispatch this method after delay since there are too many events for group mwms download.
  // We do not need to update badge frequently.
  // Update after 1 second delay (after last country event) is sure enough for app badge.
  SEL const updateBadge = @selector(updateApplicationIconBadgeNumber);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:updateBadge object:nil];
  [self performSelector:updateBadge withObject:nil afterDelay:1.0];
}

#pragma mark - Properties

- (MapViewController *)mapViewController {
  return [(UINavigationController *)self.window.rootViewController viewControllers].firstObject;
}

- (MWMCarPlayService *)carplayService {
  return [MWMCarPlayService shared];
}

#pragma mark - TTS

- (void)enableTTSForTheFirstTime {
  if (![MWMTextToSpeech savedLanguage].length)
    [MWMTextToSpeech setTTSEnabled:YES];
}

#pragma mark - Standby

- (void)enableStandby {
  self.standbyCounter--;
}
- (void)disableStandby {
  self.standbyCounter++;
}
- (void)setStandbyCounter:(NSInteger)standbyCounter {
  _standbyCounter = MAX(0, standbyCounter);
  dispatch_async(dispatch_get_main_queue(), ^{
    [UIApplication sharedApplication].idleTimerDisabled = (self.standbyCounter != 0);
  });
}

#pragma mark - Alert logic

- (void)firstLaunchSetup {
  [MWMSettings setStatisticsEnabled:YES];
  NSString *currentVersion = [NSBundle.mainBundle objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSUserDefaults *standartDefaults = NSUserDefaults.standardUserDefaults;
  [standartDefaults setObject:currentVersion forKey:kUDFirstVersionKey];
  [standartDefaults setInteger:1 forKey:kUDSessionsCountKey];
  [standartDefaults setObject:NSDate.date forKey:kUDLastLaunchDateKey];
  [standartDefaults synchronize];

  GetPlatform().GetMarketingService().ProcessFirstLaunch();
}

- (void)incrementSessionCount {
  NSUserDefaults *standartDefaults = NSUserDefaults.standardUserDefaults;
  NSUInteger sessionCount = [standartDefaults integerForKey:kUDSessionsCountKey];
  NSUInteger const kMaximumSessionCountForShowingShareAlert = 50;
  if (sessionCount > kMaximumSessionCountForShowingShareAlert)
    return;

  NSDate *lastLaunchDate = [standartDefaults objectForKey:kUDLastLaunchDateKey];
  if (lastLaunchDate.daysToNow > 0) {
    sessionCount++;
    [standartDefaults setInteger:sessionCount forKey:kUDSessionsCountKey];
    [standartDefaults setObject:NSDate.date forKey:kUDLastLaunchDateKey];
    [standartDefaults synchronize];
  }
}

- (void)showAlertIfRequired {
  if ([self shouldShowRateAlert]) {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showRateAlert) object:nil];
    [self performSelector:@selector(showRateAlert) withObject:nil afterDelay:30.0];
  }
}

#pragma mark - Rate

- (void)showRateAlert {
  if (!Platform::IsConnected() || [MWMRouter isRoutingActive])
    return;

  [[MWMAlertViewController activeAlertController] presentRateAlert];
  [NSUserDefaults.standardUserDefaults setObject:NSDate.date forKey:kUDLastRateRequestDate];
}

- (BOOL)shouldShowRateAlert {
  NSUInteger const kMaximumSessionCountForShowingAlert = 21;
  NSUserDefaults const *const standartDefaults = NSUserDefaults.standardUserDefaults;
  if ([standartDefaults boolForKey:kUDAlreadyRatedKey])
    return NO;

  NSUInteger const sessionCount = [standartDefaults integerForKey:kUDSessionsCountKey];
  if (sessionCount > kMaximumSessionCountForShowingAlert)
    return NO;

  NSDate *const lastRateRequestDate = [standartDefaults objectForKey:kUDLastRateRequestDate];
  NSInteger const daysFromLastRateRequest = lastRateRequestDate.daysToNow;
  // Do not show more than one alert per day.
  if (lastRateRequestDate != nil && daysFromLastRateRequest == 0)
    return NO;

  if (self.userIsNew) {
    // It's new user.
    if (sessionCount == 3 || sessionCount == 10 || sessionCount == kMaximumSessionCountForShowingAlert)
      return YES;
  } else {
    // User just got updated. Show alert, if it first session or if 90 days spent.
    if (daysFromLastRateRequest >= 90 || daysFromLastRateRequest == 0)
      return YES;
  }
  return NO;
}

- (BOOL)userIsNew {
  NSString *currentVersion = [NSBundle.mainBundle objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSString *firstVersion = [NSUserDefaults.standardUserDefaults stringForKey:kUDFirstVersionKey];
  if (!firstVersion.length || firstVersionIsLessThanSecond(firstVersion, currentVersion))
    return NO;

  return YES;
}

#pragma mark - Showcase

- (MWMMyTarget *)myTarget {
  if (![ASIdentifierManager sharedManager].advertisingTrackingEnabled)
    return nil;

  if (!_myTarget)
    _myTarget = [[MWMMyTarget alloc] init];
  return _myTarget;
}

#pragma mark - NotificationManagerDelegate

- (void)didOpenNotification:(Notification *)notification {
  if (notification.class == ReviewNotification.class) {
    [Statistics logEvent:kStatUGCReviewNotificationClicked];
    ReviewNotification *reviewNotification = (ReviewNotification *)notification;
    if (GetFramework().MakePlacePageForNotification(reviewNotification.notificationWrapper.notificationCandidate))
      [[MapViewController sharedController].controlsManager showPlacePageReview];
  } else if (notification.class == AuthNotification.class) {
    [Statistics logEvent:@"UGC_UnsentNotification_clicked"];
    MapViewController *mapViewController = [MapViewController sharedController];
    [mapViewController.navigationController popToRootViewControllerAnimated:NO];
    [mapViewController showUGCAuth];
  }
}

#pragma mark - AppsFlyerTrackerDelegate

- (void)onConversionDataReceived:(NSDictionary *)installData {
  if ([installData[@"is_first_launch"] boolValue]) {
    NSString *deeplink = installData[@"af_dp"];
    NSURL *deeplinkUrl = [NSURL URLWithString:deeplink];
    if (deeplinkUrl != nil) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [[DeepLinkHandler shared] applicationDidReceiveUniversalLink:deeplinkUrl provider:DeepLinkProviderAppsflyer];
      });
    }
  }
}

- (void)onConversionDataRequestFailure:(NSError *)error {
  dispatch_async(dispatch_get_main_queue(), ^{
    [[FIRCrashlytics crashlytics] recordError:error];
  });
}

#pragma mark - CPApplicationDelegate implementation

- (void)application:(UIApplication *)application
  didConnectCarInterfaceController:(CPInterfaceController *)interfaceController
                          toWindow:(CPWindow *)window API_AVAILABLE(ios(12.0)) {
  [self.carplayService setupWithWindow:window interfaceController:interfaceController];
  if (@available(iOS 13.0, *)) {
    window.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
  }
  [self updateAppearanceFromWindow:self.window toWindow:window isCarplayActivated:YES];

  [Statistics logEvent:kStatCarplayActivated];
}

- (void)application:(UIApplication *)application
  didDisconnectCarInterfaceController:(CPInterfaceController *)interfaceController
                           fromWindow:(CPWindow *)window API_AVAILABLE(ios(12.0)) {
  [self.carplayService destroy];
  [self updateAppearanceFromWindow:window toWindow:self.window isCarplayActivated:NO];

  [Statistics logEvent:kStatCarplayDeactivated];
}

- (void)updateAppearanceFromWindow:(UIWindow *)sourceWindow
                          toWindow:(UIWindow *)destinationWindow
                isCarplayActivated:(BOOL)isCarplayActivated {
  CGFloat sourceContentScale = sourceWindow.screen.scale;
  CGFloat destinationContentScale = destinationWindow.screen.scale;
  if (ABS(sourceContentScale - destinationContentScale) > 0.1) {
    if (isCarplayActivated) {
      [self updateVisualScale:destinationContentScale];
    } else {
      [self updateVisualScaleToMain];
    }
  }
}

- (void)updateVisualScale:(CGFloat)scale {
  if ([self isGraphicContextInitialized]) {
    [self.mapViewController.mapView updateVisualScaleTo:scale];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateVisualScale:scale];
    });
  }
}

- (void)updateVisualScaleToMain {
  if ([self isGraphicContextInitialized]) {
    [self.mapViewController.mapView updateVisualScaleToMain];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateVisualScaleToMain];
    });
  }
}

@end

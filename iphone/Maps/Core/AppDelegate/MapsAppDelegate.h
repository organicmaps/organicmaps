#import "DownloadIndicatorProtocol.h"
#import "MWMNavigationController.h"

@class MapViewController;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder <UIApplicationDelegate, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
}

@property(nonatomic) UIWindow * window;

@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;
- (void)completeOAuth2Authorization;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

// UIApplicationDelegate callbacks forwarded by MainSceneDelegate once UIScene is adopted.
// Re-declared here so Swift sees them on the concrete MapsAppDelegate type.
- (void)applicationDidBecomeActive:(UIApplication *)application;
- (void)applicationWillResignActive:(UIApplication *)application;
- (void)applicationDidEnterBackground:(UIApplication *)application;
- (void)applicationWillEnterForeground:(UIApplication *)application;
- (BOOL)application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options;
- (BOOL)application:(UIApplication *)application
    continueUserActivity:(NSUserActivity *)userActivity
      restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler;
- (void)application:(UIApplication *)application
    performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
               completionHandler:(void (^)(BOOL))completionHandler;

@end

NS_ASSUME_NONNULL_END

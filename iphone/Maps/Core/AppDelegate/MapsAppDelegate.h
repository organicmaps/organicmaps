#import "MWMNavigationController.h"

@class MapViewController;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder <UIApplicationDelegate>

@property(nonatomic, strong, nullable) UIWindow * window;

// The connected main window, or nil before MainSceneDelegate attaches it (e.g. a CarPlay-first
// cold launch, where only the CarPlay scene exists). Use this instead of `window` when the phone
// scene may not have connected yet.
@property(nonatomic, readonly, nullable) UIWindow * connectedWindow;

// The Main storyboard's root navigation controller. Lazily instantiated so a single shared
// MapViewController exists even before the phone window scene connects.
@property(nonatomic, readonly) UINavigationController * mainNavigationController;
@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (void)enableStandby;
- (void)disableStandby;
- (void)completeOAuth2Authorization;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (BOOL)handleOpenURL:(NSURL *)url openInPlace:(BOOL)openInPlace NS_SWIFT_NAME(handleOpenURL(_:openInPlace:));
- (BOOL)handleUserActivity:(NSUserActivity *)userActivity NS_SWIFT_NAME(handleUserActivity(_:));
- (void)handleShortcutItem:(UIApplicationShortcutItem *)shortcutItem
         completionHandler:(void (^)(BOOL))completionHandler NS_SWIFT_NAME(handleShortcutItem(_:completionHandler:));

// Aggregate phone and CarPlay scene transitions into application-wide lifecycle events.
- (void)sceneDidBecomeActive:(UIScene *)scene NS_SWIFT_NAME(sceneDidBecomeActive(_:));
- (void)sceneWillResignActive:(UIScene *)scene NS_SWIFT_NAME(sceneWillResignActive(_:));
- (void)sceneWillEnterForeground:(UIScene *)scene NS_SWIFT_NAME(sceneWillEnterForeground(_:));
- (void)sceneDidEnterBackground:(UIScene *)scene NS_SWIFT_NAME(sceneDidEnterBackground(_:));

- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

@end

NS_ASSUME_NONNULL_END
